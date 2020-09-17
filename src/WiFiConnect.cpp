#include <WiFiConnect.hpp>

WiFiManager wm;

configStruct cfg;

// setup custom parameters
// id/name  placeholder/prompt  default  length
WiFiManagerParameter custom_influx_server("server", "influx server", cfg.influx_server, 40);
WiFiManagerParameter custom_influx_port("port", "influx port", cfg.influx_port, 6);
WiFiManagerParameter custom_influx_db("influxdb", "database", cfg.influx_db, 32);
WiFiManagerParameter custom_devicename("devicename", "device name", cfg.devicename, 32);

//flag for saving data
bool shouldSaveConfig = false;

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

                strlcpy(cfg.influx_server, json["hostname"] | cfg.influx_server, sizeof(cfg.influx_server));
                strlcpy(cfg.influx_port, json["port"] | cfg.influx_port, sizeof(cfg.influx_port));
                strlcpy(cfg.influx_db, json["influxdb"] | cfg.influx_db, sizeof(cfg.influx_db));
                strlcpy(cfg.devicename, json["devicename"] | "", sizeof(cfg.devicename));

                custom_influx_server.setValue(cfg.influx_server, 40);
                custom_influx_port.setValue(cfg.influx_port, 6);
                custom_influx_db.setValue(cfg.influx_db, 32);
                custom_devicename.setValue(cfg.devicename, 32);

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
    strcpy(cfg.influx_server, custom_influx_server.getValue());
    strcpy(cfg.influx_port, custom_influx_port.getValue());
    strcpy(cfg.influx_db, custom_influx_db.getValue());
    strcpy(cfg.devicename, custom_devicename.getValue());
}

void writeConfigFile() {
    Serial.println(">WM: saving config");
    StaticJsonDocument<512> json;
    json["devicename"] = cfg.devicename;
    json["hostname"] = cfg.influx_server;
    json["port"] = cfg.influx_port;
    json["influxdb"] = cfg.influx_db;

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
    Serial.println(cfg.devicename);
    Serial.print(">WM: server:\t");
    Serial.println(cfg.influx_server);
    Serial.print(">WM: port:\t");
    Serial.println(cfg.influx_port);
    Serial.print(">WM: database:\t");
    Serial.println(cfg.influx_db);
}

//callback notifying us of the need to save config
void saveConfigCallback() {
    Serial.println(">WM: saving new config..");
    readCurrentValues();
    printConfigValues();
    writeConfigFile();
}

void startConfigPortal(){
    wm.setConfigPortalBlocking(false);
    //set config save notify callback
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.startConfigPortal("CanAirIO Config", "CanAirIO");
    Serial.println(">WM: config portal is running..");
}

void setupWifiManager() {
    //reading config file from flash
    setupSpiffs();
    //adding custom parameters for CanAirIO device configuration
    wm.addParameter(&custom_devicename);
    wm.addParameter(&custom_influx_server);
    wm.addParameter(&custom_influx_db);
    wm.addParameter(&custom_influx_port);
    //reset settings - wipe credentials for testing
    // wm.resetSettings();
    startConfigPortal();
    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect("CanAirIO Config", "CanAirIO")){
        Serial.println(">WM: connected! :)");
        #ifdef ESP32
        WiFi.setHostname("CanAirIO");
        #endif
    }
    readCurrentValues();
    printConfigValues();

}

void wifiConnectLoop(){
    wm.process();
}

configStruct getConfig(){
    return cfg;
}