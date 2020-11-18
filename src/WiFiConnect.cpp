#include <WiFiConnect.hpp>

WiFiManager wm;

configStruct cfg;

// setup custom parameters
// id/name  placeholder/prompt  default  length
WiFiManagerParameter custom_influx_server("server", "influx server", cfg.influx_server, 40);
WiFiManagerParameter custom_influx_port("port", "influx port", cfg.influx_port, 6);
WiFiManagerParameter custom_influx_db("influxdb", "database", cfg.influx_db, 32);
WiFiManagerParameter custom_devicename("devicename", "device name", cfg.devicename, 32);
WiFiManagerParameter custom_country_code("country_code", "country code", cfg.country_code, 32);
WiFiManagerParameter custom_sample_time("stime", "sample time in seconds", cfg.stime, 8);
// WiFiManagerParameter custom_sensor_type; // custom radio button
// WiFiManagerParameter custom_sensor_type; // custom radio button
WiFiManagerParameter custom_station_lat("lat", "Latitude", cfg.lat, 12);
WiFiManagerParameter custom_station_lon("lon", "Longitude", cfg.lon, 12);
WiFiManagerParameter custom_sensor_type; // global param ( for non blocking w params )

//flag for saving data
bool shouldSaveConfig = false;

void loadCustomFieldsValues() {
    custom_influx_server.setValue(cfg.influx_server, 40);
    custom_influx_port.setValue(cfg.influx_port, 6);
    custom_influx_db.setValue(cfg.influx_db, 32);
    custom_devicename.setValue(cfg.devicename, 32);
    custom_country_code.setValue(cfg.country_code, 32);
    custom_sample_time.setValue(cfg.stime, 8);
    // custom_sensor_type.setValue(cfg.stype, 4); => WM not handled it
    custom_station_lat.setValue(cfg.lat, 12);
    custom_station_lon.setValue(cfg.lon, 12);
}

void loadCustomFieldsDefaults() {
    strlcpy(cfg.influx_server, "influxdb.canair.io", sizeof(cfg.influx_server));
    strlcpy(cfg.influx_port, "8086", sizeof(cfg.influx_port));
    strlcpy(cfg.influx_db, "canairio", sizeof(cfg.influx_db));
    strlcpy(cfg.devicename, "PM25_Fixed_Station", sizeof(cfg.devicename));
    strlcpy(cfg.country_code, "XX", sizeof(cfg.country_code));
    strlcpy(cfg.stime, "10", sizeof(cfg.stime));
    strlcpy(cfg.stype, "0", sizeof(cfg.stype));
    strlcpy(cfg.lat, "0.0", sizeof(cfg.lat));
    strlcpy(cfg.lon, "0.0", sizeof(cfg.lon));

    loadCustomFieldsValues();
}

bool setupSpiffs() {
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

                strlcpy(cfg.influx_server, json["hostname"] | "", sizeof(cfg.influx_server));
                strlcpy(cfg.influx_port, json["port"] | "", sizeof(cfg.influx_port));
                strlcpy(cfg.influx_db, json["influxdb"] | "", sizeof(cfg.influx_db));
                strlcpy(cfg.devicename, json["devicename"] | "", sizeof(cfg.devicename));
                strlcpy(cfg.country_code, json["country_code"] | "", sizeof(cfg.country_code));
                strlcpy(cfg.stime, json["stime"] | "", sizeof(cfg.stime));
                strlcpy(cfg.stype, json["stype"] | "", sizeof(cfg.stype));
                strlcpy(cfg.lat, json["lat"] | "", sizeof(cfg.lat));
                strlcpy(cfg.lon, json["lon"] | "", sizeof(cfg.lon));

                loadCustomFieldsValues();

                configFile.close();

                return true;

            } else {
                Serial.println("-WM: failed to open json config!");
            }
        } else {
            Serial.println("-WM: config file not found!");
        }
    } else {
        Serial.println("-WM: failed to mount FS!");
    }
    
    return false;
}
String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void readCurrentValues(bool isSaving){
    //read updated parameters
    strcpy(cfg.influx_server, custom_influx_server.getValue());
    strcpy(cfg.influx_port, custom_influx_port.getValue());
    strcpy(cfg.influx_db, custom_influx_db.getValue());
    strcpy(cfg.devicename, custom_devicename.getValue());
    strcpy(cfg.country_code, custom_country_code.getValue());
    strcpy(cfg.stime, custom_sample_time.getValue());
    if(isSaving) strcpy(cfg.stype, getParam("customfieldid").c_str());
    strcpy(cfg.lat, custom_station_lat.getValue());
    strcpy(cfg.lon, custom_station_lon.getValue());
}

void writeConfigFile() {
    Serial.println(">WM: saving config");
    StaticJsonDocument<1024> json;
    json["devicename"] = cfg.devicename;
    json["country_code"] = cfg.country_code;
    json["hostname"] = cfg.influx_server;
    json["port"] = cfg.influx_port;
    json["influxdb"] = cfg.influx_db;
    json["stime"] = cfg.stime;
    json["stype"] = cfg.stype;
    json["lat"] = cfg.lat;
    json["lon"] = cfg.lon;

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
    Serial.print(">WM: country:\t");
    Serial.println(cfg.country_code);
    Serial.print(">WM: device:\t");
    Serial.println(cfg.devicename);
    Serial.print(">WM: server:\t");
    Serial.println(cfg.influx_server);
    Serial.print(">WM: port:\t");
    Serial.println(cfg.influx_port);
    Serial.print(">WM: database:\t");
    Serial.println(cfg.influx_db);
    Serial.print(">WM: stime:\t");
    Serial.println(cfg.stime);
    Serial.print(">WM: stype:\t");
    Serial.println(cfg.stype);
    Serial.print(">WM: lat:\t");
    Serial.println(cfg.lat);
    Serial.print(">WM: lon:\t");
    Serial.println(cfg.lon);
}

//callback notifying us of the need to save config
void saveConfigCallback() {
    Serial.println(">WM: saving new config..");
    readCurrentValues(true);
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
    // const char* custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
    const char* custom_radio_str = "<br/><label for='customfieldid'>Sensor type</label><input type='radio' name='customfieldid' value='0' checked> Auto<br><input type='radio' name='customfieldid' value='1'> Panasonic<br><input type='radio' name='customfieldid' value='2'> Sensirion";
    new (&custom_sensor_type) WiFiManagerParameter(custom_radio_str);  // custom html input

    //reading config file from flash
    if(!setupSpiffs())loadCustomFieldsDefaults();
    //adding custom parameters for CanAirIO device configuration
    wm.setDebugOutput(false);
    wm.addParameter(&custom_devicename);
    wm.addParameter(&custom_country_code);
    wm.addParameter(&custom_influx_server);
    wm.addParameter(&custom_influx_db);
    wm.addParameter(&custom_influx_port);
    wm.addParameter(&custom_sample_time);
    wm.addParameter(&custom_station_lat);
    wm.addParameter(&custom_station_lon);
    wm.addParameter(&custom_sensor_type);

    //reset settings - wipe credentials for testing
    // wm.resetSettings();
    startConfigPortal();
    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect("CanAirIO Config", "CanAirIO")){
        Serial.println(">WM: connected! :)");
        wm.setHostname("CanAirIO");
    }
    readCurrentValues(false);
    printConfigValues();

}

void wifiConnectLoop(){
    wm.process();
}

configStruct getConfig(){
    return cfg;
}