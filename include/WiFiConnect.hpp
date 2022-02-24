#include <ArduinoJson.h>
#include <FS.h>
#include <WiFiManager.h>

#ifdef ESP32
#include <SPIFFS.h>
#endif

#define WIFI_TIMEOUT            180  // Config portal timeout
#define APP_REFRESH_TIME        10   // polling time for check the app
 
struct configStruct {
    char influx_server[40];  // influxdb server IP
    char influx_port[6];     // influxdb server port
    char influx_db[32];      // influxdb database name
    char geohash[32];        // CanAirIO geohash location
    char stime[8];           // sample time
    char stype[4];           // sensor type
};

void setupWifiManager();

void wifiConnectLoop();

configStruct getConfig();

