#include <WiFiConnect.hpp>  // Abstraction and handler of WifiManager

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
    startWifiManager();
    
    Serial.println(">VD: setup ready!");
}

void loop() {
    static uint_fast64_t timeStamp = 0;
    wifiConnectLoop();
    if(!WiFi.isConnected() && !isPortalRunning()){
        Serial.println("-AW: failed to connect, starting config portal..");
        startWifiManager();
        timeStamp = millis();
    }
    if(WiFi.isConnected()) {
        if(millis() - timeStamp > APP_REFRESH_TIME*1000) {   
            runPingTest();                    // <== Application code running here
            keepAlivePortal();                // validate if portal should be on
            timeStamp = millis();
        }
    } else {
        if (millis() - timeStamp > PORTAL_TIMEOUT*1000) {
            Serial.println("-AW: WiFi is disconnected!");
            WiFi.disconnect();
            restartWifiManager();
            timeStamp = millis();
        }
    }
}
