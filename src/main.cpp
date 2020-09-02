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

// setup custom parameters
// id/name  placeholder/prompt  default  length
WiFiManagerParameter custom_influx_server("server", "influx server", influx_server, 40);
WiFiManagerParameter custom_influx_port("port", "influx port", influx_port, 6);
WiFiManagerParameter custom_influx_db("influxdb", "database", influx_db, 32);
WiFiManagerParameter custom_devicename("devicename", "device name", devicename, 32);

//flag for saving data
bool shouldSaveConfig = false;
bool isPortalRunning = true;

#define GPIO_LED_GREEN          22  // Led on TTGO board (black)
#define PORTAL_TIMEOUT          60  // Config portal timeout in seconds
#define APP_REFRESH_TIME        10  // polling time for check the app


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

                custom_influx_server.setValue(influx_server, 40);
                custom_influx_port.setValue(influx_port, 6);
                custom_influx_db.setValue(influx_db, 32);
                custom_devicename.setValue(devicename, 32);

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

//callback notifying us of the need to save config
void saveConfigCallback() {
    Serial.println(">WM: saving new config..");
    readCurrentValues();
    printConfigValues();
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

void startWifiManager() {
    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect("CanAirIO_ConfigMe!")){
        Serial.println(">WM: connected! :)");
        WiFi.setHostname("CanAirIO");
        readCurrentValues();
        printConfigValues();
    }
    else {
        Serial.println(">WM: Config portal is running");
    }
    // always start configportal for a little while
    wm.setConfigPortalTimeout(PORTAL_TIMEOUT);
    wm.startConfigPortal("CanAirIO Config", "CanAirIO");
    isPortalRunning=true;
}

void setupWifiManager() {
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
}

int keepAliveTick;

void keepAlivePortal(){
    if (isPortalRunning && keepAliveTick++ > PORTAL_TIMEOUT / APP_REFRESH_TIME) {
        Serial.println("-AV: App ok. Disconnecting Config portal.");
        wm.stopConfigPortal();
        keepAliveTick=0;
        isPortalRunning = false;
    }
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
    startWifiManager();
    Serial.println(">VD: setup ready!");
}

void loop() {
    static uint_fast64_t timeStamp = 0;
    wm.process();
    if(!WiFi.isConnected() && !isPortalRunning){
        Serial.println("-AW: failed to connect, starting config portal..");
        startWifiManager();
        isPortalRunning=true;
        timeStamp = millis();
    }
    if(WiFi.isConnected()) {
        if(millis() - timeStamp > APP_REFRESH_TIME*1000) {   
            runPingTest();                    // <== Application code running here
            timeStamp = millis();
            keepAlivePortal();
        }
    } else {
        if (millis() - timeStamp > PORTAL_TIMEOUT*1000) {
            Serial.println("-AW: WiFi is disconnected!");
            WiFi.disconnect();
            isPortalRunning=false;
            timeStamp = millis();
        }
    }
}
