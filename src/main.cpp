#include <Arduino.h>

#include "credentials.h"
#include "wifi/wifi.h"
#include "mqtt/MqttClient.h"
#include "signal_blocs/SignalBlocs.h"

// Wi-Fi and MQTT instances
WifiManager wifiManager(WIFI_SSID, WIFI_PASSWORD);
MqttClient mqttClient(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, MQTT_CLIENT_ID);

// SignalBlocs module
SignalBlocs signalBlocs(1, &mqttClient);

void setup() {
    delay(1000);

    Serial.begin(115200);
    Log.begin(LOG_LEVEL_NOTICE, &Serial);

    Log.notice(F("-------------------------\n"));
    wifiManager.connect();
    mqttClient.connect();

    if (mqttClient.isConnected()) {
        Serial.println("MQTT client connected. Initializing blocks...");
        signalBlocs.initBlocks();
        Serial.println("Blocks initialized.");
    } else {
        Serial.println("MQTT client not connected at setup.");
    }
    Log.notice(F("-------------------------\n"));
}

void loop() {
    // Non-blocking maintenance of Wi-Fi and MQTT connections
    wifiManager.maintain();
    mqttClient.loop();

    static unsigned long lastDisplayTime = 0;
    const unsigned long DISPLAY_INTERVAL = 5000;

    unsigned long now = millis();
    if (now - lastDisplayTime >= DISPLAY_INTERVAL) {
        lastDisplayTime = now;
    }

    signalBlocs.displayBlockStates();
    delay(50);
}
