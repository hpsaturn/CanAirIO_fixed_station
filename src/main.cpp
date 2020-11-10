#include <Arduino.h>
#include <WiFiConnect.hpp>  // Abstraction and handler of WifiManager
#include <Sensors.hpp>

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


void onSensorDataOk() {
    Serial.print("-->[MAIN] PM1.0: "+sensors.getStringPM1());
    Serial.print  (" PM2.5: " + sensors.getStringPM25());
    Serial.println(" PM10: " + sensors.getStringPM10());
}

void onSensorDataError(const char * msg){
    Serial.println(msg);
}

void blinkOnboardLed() {
    digitalWrite(GPIO_LED_GREEN, LOW);
    delay(50);  // fast blink for low power
    digitalWrite(GPIO_LED_GREEN, HIGH);
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

    sensors.setSampleTime(5);                       // config sensors sample time interval
    sensors.setOnDataCallBack(&onSensorDataOk);     // all data read callback
    sensors.setOnErrorCallBack(&onSensorDataError); // [optional] error callback
    sensors.setDebugMode(true);                     // [optional] debug mode
    sensors.init(sensors.Sensirion);                // Force detection to Sensirion sensor

    if(sensors.isPmSensorConfigured())
        Serial.println("-->[SETUP] Sensor configured: " + sensors.getPmDeviceSelected());
    
    Serial.println(">VD: setup ready!");
}

void loop() {
    static uint_fast64_t timeStamp = 0;
    wifiConnectLoop();
    if(WiFi.isConnected()) {
        if(millis() - timeStamp > APP_REFRESH_TIME*1000) {   
            runPingTest();                    // <== Application code running here
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
    sensors.loop();  // read sensor data and showed it
}
