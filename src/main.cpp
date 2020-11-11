#include <Arduino.h>
#include <WiFiConnect.hpp>  // Abstraction and handler of WifiManager
#include <Sensors.hpp>
#include <InfluxArduino.hpp>

#ifdef ESP32
#include <ESP32Ping.h>      // Only for tests
#elif ESP8266
#include <ESP8266Ping.h>
#endif

#ifdef TTGO_T7
#define GPIO_LED_GREEN          22  // Led on TTGO board (black)
#else
#define GPIO_LED_GREEN          LED_BUILTIN
#endif

#define PUBLISH_INTERVAL 30       // publish to cloud each 30 seconds
#define WIFI_RETRY_CONNECTION 30  // 30 seconds wait for wifi connection
#define IFX_RETRY_CONNECTION 5    // influxdb publish retry 

InfluxArduino influx;
uint32_t ifxdbwcount;

String getDeviceId() {
    uint8_t baseMac[6];
    // Get MAC address for WiFi station
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    char baseMacChr[18] = {0};
    sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]+2);
    return String(baseMacChr);
}

void blinkOnboardLed() {
    digitalWrite(GPIO_LED_GREEN, LOW);
    delay(50);  // fast blink for low power
    digitalWrite(GPIO_LED_GREEN, HIGH);
}

/******************************************************************************
*   I N F L U X D B   M E T H O D S
******************************************************************************/

void influxDbInit() {
    if (WiFi.isConnected()) {
        influx.configure(getConfig().influx_db, getConfig().influx_server);  //third argument (port number) defaults to 8086
        Serial.print(">VM: [INFLUXDB] using HTTPS: ");
        Serial.println(influx.isSecure());  //will be true if you've added the InfluxCert.hpp file.
        Serial.println(">VM: [INFLUXDB] connected.");
        delay(100);
    }
}

/**
 * @influxDbParseFields:
 *
 * Supported:
 * "id","pm1","pm25","pm10,"hum","tmp","lat","lng","alt","spd","stime","tstp"
 *
 */
void influxDbParseFields(char* fields) {
    sprintf(
        fields,
        "pm1=%u,pm25=%u,pm10=%u,hum=%f,tmp=%f,prs=%f,gas=%f,lat=%f,lng=%f,alt=%f,stime=%i",
        sensors.getPM1(),
        sensors.getPM25(),
        sensors.getPM10(),
        sensors.getHumidity(),
        sensors.getTemperature(),
        sensors.getPressure(),
        sensors.getGas(),
        getConfig().lat,
        getConfig().lon,
        sensors.getAltitude(),
        getConfig().stime
        );
}

void influxDbAddTags(char* tags) {
    sprintf(tags, "dname=%s,mac=%s,dtype=%s",
            getConfig().devicename,
            getDeviceId().c_str(),
            sensors.getPmDeviceSelected().c_str());
}

bool influxDbWrite() {
    char tags[128];
    influxDbAddTags(tags);
    char fields[256];
    influxDbParseFields(fields);
    return influx.write(getConfig().country_code, tags, fields);
}

void influxDbLoop() {
    static uint_fast64_t timeStamp = 0;
    if (millis() - timeStamp > getConfig().stime * 2 * 1000) {
        timeStamp = millis();
        if (sensors.isDataReady() && WiFi.isConnected()) {
            int ifx_retry = 0;
            Serial.printf(">VM: [INFLUXDB] %s ", getConfig().devicename);
            Serial.printf("to %s\n", getConfig().influx_server);
            while (!influxDbWrite() && (ifx_retry++ < IFX_RETRY_CONNECTION)) {
                delay(200);
            }
            if (ifx_retry > IFX_RETRY_CONNECTION) {
                Serial.println("-EE: [INFLUXDB] write error, try wifi restart..");
                // wifiRestart();
            } else {
                Serial.printf(">VM: [INFLUXDB] write done. Response: %d\n", influx.getResponse());
                blinkOnboardLed();
            }
        }
    }  
}

void onSensorDataOk() {
    Serial.print(">VM: [SENSORS] PM1.0: "+sensors.getStringPM1());
    Serial.print  (" PM2.5: " + sensors.getStringPM25());
    Serial.println(" PM10: " + sensors.getStringPM10());
}

void onSensorDataError(const char * msg){
    Serial.println(msg);
}

void runPingTest() {
    Serial.print(">VD: Pinging:\t");
    Serial.print(getConfig().influx_server);
    if (Ping.ping(getConfig().influx_server)) {
        blinkOnboardLed();
        Serial.println(" -> Success!!");
    } else {
        Serial.println(" -> Error :(");
    }
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println();

    // GPIO setup
    pinMode(GPIO_LED_GREEN, OUTPUT);
    digitalWrite(GPIO_LED_GREEN, HIGH);

    setupWifiManager();
    influxDbInit();

    sensors.setSampleTime(5);                       // config sensors sample time interval
    sensors.setOnDataCallBack(&onSensorDataOk);     // all data read callback
    sensors.setOnErrorCallBack(&onSensorDataError); // [optional] error callback
    sensors.setDebugMode(false);                     // [optional] debug mode
    sensors.init(sensors.Sensirion);                // Force detection to Sensirion sensor

    if(sensors.isPmSensorConfigured())
        Serial.println(">VM: [SENSORS] Sensor configured: " + sensors.getPmDeviceSelected());
    
    Serial.println(">VM: [SETUP] setup ready!");
}

void loop() {
    static uint_fast64_t timeStamp = 0;
    wifiConnectLoop();
    if(WiFi.isConnected()) {
        if(millis() - timeStamp > APP_REFRESH_TIME*1000) {
            sensors.loop();                   // read sensor data and showed it
            // runPingTest();                    // <== Application code running here
            influxDbLoop();
            timeStamp = millis();
        }
    } else {
        if (millis() - timeStamp > WIFI_TIMEOUT*1000) {
            Serial.println("-AW: WiFi is disconnected!");
            timeStamp = millis();
            WiFi.disconnect();
            delay(1000);
            ESP.restart();
        }
    }
}
