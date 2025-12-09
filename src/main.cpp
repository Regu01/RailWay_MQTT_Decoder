#include <Arduino.h>
#include <Wire.h>
#include <PCA95x5.h>

#include "credentials.h"
#include "settings.h"
#include "wifi/wifi.h"
#include "mqtt/MqttClient.h"
#include "signals/TurnoutSignals.h"
#include "signal_blocs/SignalBlocs.h"
#include "servos/ServoTester.h"
#include "servos/ServoController.h"

using Expander = PCA95x5::PCA95x5<>;
Expander expanders[] = { Expander(), Expander(), Expander() };
const uint8_t EXPANDER_ADDRS[] = { 0x20, 0x21, 0x22 };

// Wi-Fi and MQTT instances
WifiManager wifiManager(WIFI_SSID, WIFI_PASSWORD);
MqttClient mqttClient(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, MQTT_CLIENT_ID);

// Turnout signals controller
TurnoutSignals turnoutSignals(BOARD_ID, expanders, sizeof(expanders) / sizeof(expanders[0]));
TurnoutSignals* turnoutSignalsPtr = &turnoutSignals;

// SignalBlocs module
SignalBlocs signalBlocs(1, &mqttClient);

// Servo test helper (periodically toggles all servos to verify the chain)
ServoTester servoTester(0x08, 16);
ServoController servoController(BOARD_ID, 0x08, 16);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Copy payload to a null-terminated buffer
    char payloadBuf[64];
    unsigned int copyLen = length < sizeof(payloadBuf) - 1 ? length : sizeof(payloadBuf) - 1;
    memcpy(payloadBuf, payload, copyLen);
    payloadBuf[copyLen] = '\0';

    bool handled = false;
    if (turnoutSignalsPtr && turnoutSignalsPtr->handleMqttMessage(topic, payloadBuf)) {
        handled = true;
        Log.notice("Handled turnout LED topic: %s payload: %s" CR, topic, payloadBuf);
    }
    if (servoController.handleTrackMessage(topic, payloadBuf)) {
        handled = true;
        Log.notice("Handled servo track topic: %s payload: %s" CR, topic, payloadBuf);
    }
    if (servoController.handleConfigMessage(topic, payloadBuf)) {
        handled = true;
        Log.notice("Handled servo config topic: %s payload: %s" CR, topic, payloadBuf);
    }
    if (!handled) {
        Log.verbose("MQTT message ignored: %s payload: %s" CR, topic, payloadBuf);
    }
}

void setup() {
    delay(1000);

    Serial.begin(115200);
    Wire.begin();
    Log.begin(LOG_LEVEL_NOTICE, &Serial);

    // Init expanders and outputs
    for (size_t i = 0; i < (sizeof(expanders) / sizeof(expanders[0])); i++) {
        expanders[i].attach(Wire, EXPANDER_ADDRS[i]);
    }
    turnoutSignals.setupOutputs();
    servoTester.begin(&Wire);
    servoController.begin(&Wire);

    mqttClient.setCallback(mqttCallback);

    Log.notice(F("-------------------------\n"));
    wifiManager.connect();
    mqttClient.connect();
    mqttClient.subscribe("trains/track/turnout/#");
    mqttClient.subscribe("trains/config/servo/#");

    if (mqttClient.isConnected()) {
        turnoutSignals.publishAll(mqttClient, "THROWN"); // initial state
        servoController.publishAll(mqttClient);
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
    static bool publishedRefs = false;
    static bool publishedServos = false;
    const unsigned long DISPLAY_INTERVAL = 5000;

    unsigned long now = millis();

    // Publish references once per MQTT connection
    if (mqttClient.isConnected()) {
        if (!publishedRefs) {
            turnoutSignals.publishAll(mqttClient, "THROWN");
            publishedRefs = true;
        }
        if (!publishedServos) {
            servoController.publishAll(mqttClient);
            publishedServos = true;
        }
    } else {
        publishedRefs = false;
        publishedServos = false;
    }

    if (now - lastDisplayTime >= DISPLAY_INTERVAL) {
        lastDisplayTime = now;
    }

    signalBlocs.displayBlockStates();
    // Comment out tester in production; keep active for validation only
    // servoTester.tick();
    delay(100);
}
