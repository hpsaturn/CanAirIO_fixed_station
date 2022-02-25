#include <Arduino.h>
#include <WiFiConnect.hpp>  // Abstraction and handler of WifiManager
#include <InfluxDbClient.h>
#include <Sensors.hpp>

#define PUBLISH_INTERVAL 30       // publish to cloud each 30 seconds
#define WIFI_RETRY_CONNECTION 30  // 30 seconds wait for wifi connection
#define IFX_RETRY_CONNECTION 5    // influxdb publish retry 

uint32_t ifxdbwcount;
int rssi = 0;
String hostId = "";

InfluxDBClient influx;
Point sensor ("fixed_stations_esp8266");
bool ifx_ready;

/******************************************************************************
*   T O O L S
******************************************************************************/

String getDeviceId() {
    uint8_t baseMac[6];
// Get MAC address for WiFi station
#ifdef ESP8266
    return WiFi.macAddress();
#else
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    char baseMacChr[18] = {0};
    sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5] + 2);
    return String(baseMacChr);
#endif
}

void blinkOnboardLed() {
    digitalWrite(BUILTIN_LED , LOW);
    delay(50);  // fast blink for low power
    digitalWrite(BUILTIN_LED , HIGH);
}

/******************************************************************************
*   I N F L U X D B   M E T H O D S
******************************************************************************/

bool influxDbIsConfigured() {
    if(getConfig().influx_db != nullptr && getConfig().influx_server != nullptr && getConfig().geohash != nullptr) {
        Serial.println("-->[W][IFDB] ifxdb is configured but Location (GeoHash) is missing!");
        return true;
    }
    return false;
}

String influxdbGetStationName() {
    String name = "";
    name = name + String(getConfig().geohash).substring(0,3); // GeoHash ~70km https://en.wikipedia.org/wiki/Geohash
    String flavor = String(FLAVOR);
    if(flavor.length() > 6) flavor = flavor.substring(0,7); // validation possible issue with short names
    name = name + flavor;                         // Flavor short, firmware name (board)
    name = name + getDeviceId().substring(10);    // MAC address 4 digts
    name.replace("_","");
    name.replace(":","");
    name.toUpperCase();
    return name;
}

void influxDbAddTags() {
    String geo3 = String(getConfig().geohash);
    sensor.addTag("mac",getDeviceId().c_str());
    sensor.addTag("geo3", geo3.substring(0,3).c_str());
    sensor.addTag("name",influxdbGetStationName().c_str());
    sensor.addTag("rev","v"+String(VERSION)+"r"+String(REVISION));
}

void influxDbInit() {
    if (!ifx_ready && WiFi.isConnected() && influxDbIsConfigured()) {
        String url = "http://"+String(getConfig().influx_server)+":"+String(getConfig().influx_port);
        influx.setInsecure();
        influx.setConnectionParamsV1(url.c_str(),getConfig().influx_db);
        Serial.printf("-->[IFDB] config: %s@%s:%s\n",getConfig().influx_db,getConfig().influx_server,getConfig().influx_port);
        influxDbAddTags();
        if(influx.validateConnection()) {
            Serial.printf("-->[IFDB] connected to %s\n",influx.getServerUrl().c_str());
            ifx_ready = true;
        }
        else Serial.println("-->[E][IFDB] connection error!");
        delay(100);
    }
}

/**
 * @influxDbParseFields:
 *
 */
void influxDbParseFields() {
    // select humi and temp for publish it
    float humi = sensors.getHumidity();
    if(humi == 0.0) humi = sensors.getCO2humi();
    float temp = sensors.getTemperature();
    if(temp == 0.0) temp = sensors.getCO2temp();

    sensor.clearFields();
    sensor.addField("pm1",sensors.getPM1());
    sensor.addField("pm25",sensors.getPM25());
    sensor.addField("pm10",sensors.getPM10());
    sensor.addField("co2",sensors.getCO2());
    sensor.addField("co2hum",sensors.getCO2humi());
    sensor.addField("co2tmp",sensors.getCO2temp());
    sensor.addField("tmp",temp);
    sensor.addField("hum",humi);
    sensor.addField("geo",String(getConfig().geohash).substring(0,7).c_str());
    sensor.addField("prs",sensors.getPressure());
    sensor.addField("gas",sensors.getGas());
    sensor.addField("alt",sensors.getAltitude());
    sensor.addField("rssi",WiFi.RSSI());
    sensor.addField("name",influxdbGetStationName().c_str());
}

bool influxDbWrite() {
    influxDbParseFields();
    Serial.printf("[IFDB] %s\n",influx.pointToLineProtocol(sensor).c_str());
    if (!influx.writePoint(sensor)) {
        Serial.print("-->[E][IFDB] Write Point failed: ");
        Serial.println(influx.getLastErrorMessage());
        return false;
    }
    return true;
}

void influxDbLoop() {
    static uint_fast64_t timeStamp = 0;
    if (atoi(getConfig().stime) > 0 && millis() - timeStamp > atoi(getConfig().stime) * 2 * 1000) {
        timeStamp = millis();
        if (sensors.isDataReady() && WiFi.isConnected()) {
            Serial.printf(">VM: [INFLUXDB] %s ", getConfig().geohash);
            Serial.printf("to %s\n", getConfig().influx_server);

            if (influxDbWrite()){
                Serial.println("-->[IFDB] write done.");
                blinkOnboardLed();
            }
            else
                Serial.printf("-->[E][IFDB] write error to %s@%s:%s \n",getConfig().geohash,getConfig().influx_server,getConfig().influx_port);
        }
    }  
}

/******************************************************************************
*   S E N S O R S
******************************************************************************/

void onSensorDataOk() {
    Serial.print(">VM: [SENSORS] PM1.0: "+sensors.getStringPM1());
    Serial.print  (" PM2.5: " + sensors.getStringPM25());
    Serial.println(" PM10: " + sensors.getStringPM10());
}

void onSensorDataError(const char * msg){
    Serial.println("-EE: [SENSORS] "+String(msg));
}

void sensorsInit() {
    sensors.setSampleTime(atoi(getConfig().stime));                       // config sensors sample time interval
    sensors.setOnDataCallBack(&onSensorDataOk);     // all data read callback
    sensors.setOnErrorCallBack(&onSensorDataError); // [optional] error callback
    sensors.setDebugMode(true);                    // [optional] debug mode
    sensors.detectI2COnly(false);                    // force only i2c sensors
#ifdef CCP_ESP32
    sensors.init(atoi(getConfig().stype));          // Sensor selected on captive portal
#else
    sensors.init(atoi(getConfig().stype),5,6);      // Sensor configured on pines 5 and 6 (RX and TX SwSerial 8266)
#endif
    delay(100);
    sensors.printUnitsRegistered(true);
}

/******************************************************************************
*   M A I N
******************************************************************************/

void setup() {
    Serial.begin(115200);
    Serial.println();

    // GPIO LED setup
    pinMode(BUILTIN_LED , OUTPUT);
    digitalWrite(BUILTIN_LED , HIGH);

    setupWifiManager();
    influxDbInit();
    sensorsInit();
   
    Serial.println(">VM: [SETUP] sample time : "+String(atoi(getConfig().stime))+" sec");
    Serial.println(">VM: [SETUP] wifi timeout: "+String(WIFI_TIMEOUT)+" sec");
    Serial.println(">VM: [SETUP] setup ready!");
}

void loop() {
    static uint_fast64_t timeStamp = 0;
    wifiConnectLoop();

    if(WiFi.isConnected()) {
        sensors.loop();  // read sensor data and showed it in callback
        influxDbLoop();
    } else {
        if (millis() - timeStamp > WIFI_TIMEOUT*1000) {
            Serial.println("-AW: WiFi is disconnected!");
            timeStamp = millis();
            WiFi.disconnect();
            Serial.println(">AW: reboot..");
            delay(1000);
            ESP.restart();
        }
    }
}
