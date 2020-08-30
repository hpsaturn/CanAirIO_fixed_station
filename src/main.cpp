#include <FS.h>          // this needs to be first, or it all crashes and burns...
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson

#ifdef ESP32
  #include <SPIFFS.h>
#endif

//define your default values here, if there are different values in config.json, they are overwritten.
char influx_server[40] = "influxdb.canair.io";
char influx_port[6]  = "8080";
char influx_db[32] = "canairio";
char devicename[32];

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setupSpiffs(){
  //clean FS, for testing
  // SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
      Serial.println("mounted file system");
      if (SPIFFS.exists("/config.json")) {
          //file exists, reading and loading
          Serial.println("reading config file");
          File configFile = SPIFFS.open("/config.json", "r");
          if (configFile) {
              Serial.println("opened config file");
              // Allocate a temporary JsonDocument
              // Don't forget to change the capacity to match your requirements.
              // Use arduinojson.org/v6/assistant to compute the capacity.
              StaticJsonDocument<512> json;
              // Deserialize the JSON document
              DeserializationError error = deserializeJson(json, configFile);
              if (error)
                  Serial.println(F("Failed to read file, using default configuration"));

              Serial.println("\nparsed json");

              strcpy(influx_server, json["hostname"]);
              strcpy(influx_port, json["port"]);
              strcpy(influx_port, json["influxdb"]);
              strcpy(devicename, json["devicename"]);

              // if(json["ip"]) {
              //   Serial.println("setting custom ip from config");
              //   strcpy(static_ip, json["ip"]);
              //   strcpy(static_gw, json["gateway"]);
              //   strcpy(static_sn, json["subnet"]);
              //   Serial.println(static_ip);
              // } else {
              //   Serial.println("no custom ip in config");
              // }

          } else {
              Serial.println("failed to load json config");
          }
      }
  } else {
      Serial.println("failed to mount FS");
  }
  //end read
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  setupSpiffs();

  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  //set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);

  // setup custom parameters
  // 
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_influx_server("server", "influx server", influx_server, 40);
  WiFiManagerParameter custom_influx_port("port", "influx port", influx_port, 6);
  WiFiManagerParameter custom_influx_db("influxdb", "database", influx_db, 32);
  WiFiManagerParameter custom_devicename("devicename", "device name", "", 32);

  //add all your parameters here
  wm.addParameter(&custom_devicename);
  wm.addParameter(&custom_influx_server);
  wm.addParameter(&custom_influx_db);
  wm.addParameter(&custom_influx_port);

  // set static ip
  // IPAddress _ip,_gw,_sn;
  // _ip.fromString(static_ip);
  // _gw.fromString(static_gw);
  // _sn.fromString(static_sn);
  // wm.setSTAStaticIPConfig(_ip, _gw, _sn);

  //reset settings - wipe credentials for testing
  wm.resetSettings();

  //automatically connect using saved credentials if they exist
  //If connection fails it starts an access point with the specified name
  //here  "AutoConnectAP" if empty will auto generate basedcon chipid, if password is blank it will be anonymous
  //and goes into a blocking loop awaiting configuration
  if (!wm.autoConnect("CanAirIO_ConfigMe!")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    // if we still have not connected restart and try all over again
    ESP.restart();
    delay(5000);
  }

  // always start configportal for a little while
  // wm.setConfigPortalTimeout(60);
  // wm.startConfigPortal("AutoConnectAP","password");

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(influx_server, custom_influx_server.getValue());
  strcpy(influx_port, custom_influx_port.getValue());
  strcpy(influx_db, custom_influx_db.getValue());
  strcpy(devicename, custom_devicename.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    StaticJsonDocument<256> json;
    json["devicename"] = devicename;
    json["hostname"] = influx_server;
    json["port"]   = influx_port;
    json["influxdb"]   = influx_db;

    // json["ip"]          = WiFi.localIP().toString();
    // json["gateway"]     = WiFi.gatewayIP().toString();
    // json["subnet"]      = WiFi.subnetMask().toString();

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
    // Serialize JSON to file
    if (serializeJson(json, configFile) == 0) {
        Serial.println(F("Failed to write to file"));
    }
    configFile.close();
    //end save
    shouldSaveConfig = false;
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());

  Serial.print(">WM: IP:\t");   Serial.println(WiFi.localIP());
  Serial.print(">WM: SSID:\t"); Serial.println(wm.getWiFiSSID());
  Serial.print(">WM: MAC:\t");  Serial.println(WiFi.macAddress());
  Serial.print(">WM: RSSI:\t"); Serial.println(WiFi.RSSI());
}

void loop() {
  // put your main code here, to run repeatedly:


}
