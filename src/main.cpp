#include <Arduino.h>
#include <Wire.h>
#include <PCA95x5.h>

#include "credentials.h"
#include "wifi/wifi.h"
#include "mqtt/MqttClient.h"
#include "signals/TurnoutSignals.h"
#include "signal_blocs/SignalBlocs.h"
#include "servos/ServoController.h"

using Expander = PCA95x5::PCA95x5<>;
Expander expanders[] = { Expander(), Expander(), Expander() };
const uint8_t EXPANDER_ADDRS[] = { 0x20, 0x21, 0x22 };

struct ServoDefault {
    uint16_t id;
    uint8_t closed;
    uint8_t thrown;
};

const ServoDefault SERVO_DEFAULTS[] = {
    {101, 70, 80},
    {102, 40, 80},
    {103, 70, 110},
    {104, 70, 110},
    {105, 70, 110},
    {106, 70, 110},
    {107, 70, 110},
    {108, 70, 110},
    {109, 70, 110},
    {110, 70, 110},
    {111, 70, 110},
    {112, 70, 110},
    {113, 70, 110},
    {114, 70, 110},
    {115, 70, 110},
    {116, 70, 110},
};

const int STATUS_LED_PIN = 2; // Active HIGH indicator

// Wi-Fi and MQTT instances
WifiManager wifiManager(WIFI_SSID, WIFI_PASSWORD);
MqttClient mqttClient(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, MQTT_CLIENT_ID);

// Turnout signals controller
TurnoutSignals turnoutSignals(BOARD_ID, expanders, sizeof(expanders) / sizeof(expanders[0]));
TurnoutSignals* turnoutSignalsPtr = &turnoutSignals;

// SignalBlocs module
SignalBlocs signalBlocs(1, &mqttClient);

ServoController servoController(BOARD_ID, 0x08, 16);

char STATUS_TOPIC[64];

void applyServoDefaults(bool publishToMqtt = false) {
    char topic[48];
    char payload[48];
    for (size_t i = 0; i < (sizeof(SERVO_DEFAULTS) / sizeof(SERVO_DEFAULTS[0])); i++) {
        snprintf(topic, sizeof(topic), "trains/config/servo/%u", SERVO_DEFAULTS[i].id);
        snprintf(payload, sizeof(payload), "{\"closed\":%u,\"thrown\":%u}", SERVO_DEFAULTS[i].closed, SERVO_DEFAULTS[i].thrown);
        servoController.handleConfigMessage(topic, payload);
        if (publishToMqtt && mqttClient.isConnected()) {
            mqttClient.publish(topic, payload);
        }
    }
}

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

    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW); // off until connected

    // Init expanders and outputs
    for (size_t i = 0; i < (sizeof(expanders) / sizeof(expanders[0])); i++) {
        expanders[i].attach(Wire, EXPANDER_ADDRS[i]);
    }
    turnoutSignals.setupOutputs();
    servoController.begin(&Wire);
    applyServoDefaults();

    mqttClient.setCallback(mqttCallback);

    Log.notice(F("-------------------------\n"));
    wifiManager.connect();
    snprintf(STATUS_TOPIC, sizeof(STATUS_TOPIC), "trains/status_card/ESP_DECODER_0%u", BOARD_ID);
    mqttClient.setWill(STATUS_TOPIC, "offline", true, 1);
    mqttClient.connect();
    mqttClient.subscribe("trains/track/turnout/#");
    mqttClient.subscribe("trains/config/servo/#");

    if (mqttClient.isConnected()) {
        turnoutSignals.publishAll(mqttClient, "THROWN"); // initial state
        servoController.publishAll(mqttClient);
        applyServoDefaults(true);
        mqttClient.publish(STATUS_TOPIC, "online");
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
    static bool publishedServoConfig = false;
    static bool publishedStatus = false;
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
        if (!publishedServoConfig) {
            applyServoDefaults(true);
            publishedServoConfig = true;
        }
        if (!publishedStatus) {
            mqttClient.publish(STATUS_TOPIC, "online");
            publishedStatus = true;
        }
        digitalWrite(STATUS_LED_PIN, HIGH); // connected
    } else {
        publishedRefs = false;
        publishedServos = false;
        publishedServoConfig = false;
        publishedStatus = false;
        digitalWrite(STATUS_LED_PIN, LOW);
    }

    if (now - lastDisplayTime >= DISPLAY_INTERVAL) {
        lastDisplayTime = now;
    }

    signalBlocs.displayBlockStates();
    delay(100);
}
