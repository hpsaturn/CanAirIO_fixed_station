#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include <FS.h>           // this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager

#ifdef ESP32
#include <SPIFFS.h>
#endif

#define WIFI_TIMEOUT            180  // Config portal timeout
#define APP_REFRESH_TIME        10   // polling time for check the app
 
struct configStruct {
    char influx_server[40];  // influxdb server IP
    char influx_port[6];     // influxdb server port
    char influx_db[32];      // influxdb database name
    char devicename[32];     // CanAirIO station name
    char country_code[32];   // Country code like US, CO
    char stime[8];           // sample time
    char lat[12];            // latitude
    char lon[12];            // longitude
};

void setupWifiManager();

void wifiConnectLoop();

configStruct getConfig();

