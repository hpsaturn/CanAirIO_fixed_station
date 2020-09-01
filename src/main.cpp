#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include <FS.h>           // this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <ESP32Ping.h>

#ifdef ESP32
#include <SPIFFS.h>
#endif

WiFiManager wm;

char influx_server[40];  // influxdb server IP
char influx_port[6];     // influxdb server port
char influx_db[32];      // influxdb database name
char devicename[32];     // CanAirIO station name

WiFiManagerParameter custom_influx_server;
WiFiManagerParameter custom_influx_port;
WiFiManagerParameter custom_influx_db;
WiFiManagerParameter custom_devicename;

//flag for saving data
bool shouldSaveConfig = false;

#define GPIO_LED_GREEN 22  // Led on TTGO board (black)


void blinkOnboardLed() {
    digitalWrite(GPIO_LED_GREEN, LOW);
    delay(50);  // fast blink for low power
    digitalWrite(GPIO_LED_GREEN, HIGH);
}

void setupSpiffs() {
    //clean FS, for testing
    //SPIFFS.format();

    //read configuration from FS json
    Serial.println(">WM: mounting FS..");

    if (SPIFFS.begin()) {
        Serial.println(">WM: mounted file system.");
        if (SPIFFS.exists("/config.json")) {
            //file exists, reading and loading
            Serial.println(">WM: reading config file..");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                Serial.println(">WM: opened config file.");
                // Use arduinojson.org/v6/assistant to compute the capacity.
                StaticJsonDocument<512> json;
                // Deserialize the JSON document
                DeserializationError error = deserializeJson(json, configFile);
                if (error)
                    Serial.println(F("-WM: Failed to read config file, using default configuration"));

                Serial.println(">WM: parsed json.");

                strlcpy(influx_server, json["hostname"] | influx_server, sizeof(influx_server));
                strlcpy(influx_port, json["port"] | influx_port, sizeof(influx_port));
                strlcpy(influx_db, json["influxdb"] | influx_db, sizeof(influx_db));
                strlcpy(devicename, json["devicename"] | "", sizeof(devicename));

                custom_influx_server.setValue(influx_server, sizeof(influx_server));
                custom_influx_port.setValue(influx_port, sizeof(influx_port));
                custom_influx_db.setValue(influx_db, sizeof(influx_db));
                custom_devicename.setValue(devicename, sizeof(devicename));

                configFile.close();

            } else {
                Serial.println("-WM: failed to open json config!");
            }
        } else {
            Serial.println("-WM: config file not found!");
        }
    } else {
        Serial.println("-WM: failed to mount FS!");
    }
}

void readCurrentValues(){
    //read updated parameters
    strcpy(influx_server, custom_influx_server.getValue());
    strcpy(influx_port, custom_influx_port.getValue());
    strcpy(influx_db, custom_influx_db.getValue());
    strcpy(devicename, custom_devicename.getValue());
}

void writeConfigFile() {
    Serial.println(">WM: saving config");
    StaticJsonDocument<512> json;
    json["devicename"] = devicename;
    json["hostname"] = influx_server;
    json["port"] = influx_port;
    json["influxdb"] = influx_db;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println(">WM: failed to open config file for writing");
    }
    // Serialize JSON to file
    if (serializeJson(json, configFile) == 0) {
        Serial.println(">WM: Failed to write to file");
    }
    configFile.close();
    shouldSaveConfig = false;
}

//callback notifying us of the need to save config
void saveConfigCallback() {
    Serial.println(">WM: Should save config");
    readCurrentValues();
    writeConfigFile();
}

void runPingTest() {
    Serial.print(">VD: Pinging:\t");
    Serial.print(influx_server);

    if (Ping.ping(influx_server)) {
        blinkOnboardLed();
        Serial.println(" -> Success!!");
    } else {
        Serial.println(" -> Error :(");
    }
}

void printConfigValues() {
    Serial.print(">WM: IP:\t");
    Serial.println(WiFi.localIP());
    Serial.print(">WM: SSID:\t");
    Serial.println(WiFi.SSID());
    Serial.print(">WM: GW:\t");
    Serial.println(WiFi.gatewayIP());
    Serial.print(">WM: MAC:\t");
    Serial.println(WiFi.macAddress());
    Serial.print(">WM: RSSI:\t");
    Serial.println(WiFi.RSSI());
    Serial.print(">WM: device:\t");
    Serial.println(devicename);
    Serial.print(">WM: server:\t");
    Serial.println(influx_server);
    Serial.print(">WM: port:\t");
    Serial.println(influx_port);
    Serial.print(">WM: database:\t");
    Serial.println(influx_db);
}

void setupWifiManager() {
    // setup custom parameters
    // id/name  placeholder/prompt  default  length

    //adding custom parameters for CanAirIO device configuration
    wm.addParameter(&custom_devicename);
    wm.addParameter(&custom_influx_server);
    wm.addParameter(&custom_influx_db);
    wm.addParameter(&custom_influx_port);
    //reset settings - wipe credentials for testing
    // wm.resetSettings();
    wm.setConfigPortalBlocking(false);
    //set config save notify callback
    wm.setSaveConfigCallback(saveConfigCallback);
    
    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect("CanAirIO_ConfigMe!")){
        Serial.println(">WM: connected! :)");
        WiFi.setHostname("CanAirIO");
        readCurrentValues();
        printConfigValues();
    }
    else {
        Serial.println(">WM: Configportal running");
    }
    
    // always start configportal for a little while
    // wm.setConfigPortalTimeout(60);
    // wm.startConfigPortal("CanAirIO Config", "CanAirIO");

}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println();

    // GPIO setup
    pinMode(GPIO_LED_GREEN, OUTPUT);
    digitalWrite(GPIO_LED_GREEN, HIGH);

    setupSpiffs();
    setupWifiManager();
    Serial.println(">VD: setup ready!");
    
}

bool isPortalRunning;

void loop() {
    wm.process();
    if(WiFi.isConnected())runPingTest();
}
