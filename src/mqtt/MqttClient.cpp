#include "MqttClient.h"

// Stockage de l'instance courante
MqttClient* mqttActiveInstance = nullptr;

MqttClient::MqttClient(const char* server, int port, const char* user, const char* password)
    : _server(server), _port(port), _user(user), _password(password), _client(_wifiClient)
{
    mqttActiveInstance = this;                    // on retient l'instance actuelle
    _client.setServer(_server, _port);
    _client.setCallback(internalCallback);        // redirection du callback

    // Init LED MQTT
    pinMode(MQTT_LED_PIN, OUTPUT);
    digitalWrite(MQTT_LED_PIN, LOW);
}

void MqttClient::connect() {
    Log.notice("Connecting to MQTT server: %s:%d" CR, _server, _port);
    reconnect();
}

void MqttClient::reconnect() {
    while (!_client.connected()) {
        Log.notice("Attempting MQTT connection..." CR);

        blinkDuringConnection(); 

        if (_client.connect("ESP8266Client", _user, _password)) {
            Log.notice("Connected to MQTT broker." CR);
        } else {
            Log.error("Failed to connect to MQTT. Retrying in 5 seconds..." CR);
            delay(5000);
        }
    }

    updateLed();
}

void MqttClient::disconnect() {
    if (_client.connected()) {
        _client.disconnect();
        Log.notice("Disconnected from MQTT broker." CR);
    } else {
        Log.warning("Tried to disconnect, but not connected to MQTT broker." CR);
    }

    updateLed();
}

void MqttClient::publish(const char* topic, const char* message) {
    if (_client.connected()) {
        _client.publish(topic, message);
        Log.notice("Published message to topic %s: %s" CR, topic, message);
    } else {
        Log.error("Failed to publish message. MQTT not connected." CR);
    }
}

void MqttClient::subscribe(const char* topic) {
    if (_client.connected()) {
        _client.subscribe(topic);
        Log.notice("Subscribed to topic: %s" CR, topic);
    } else {
        Log.error("Failed to subscribe. MQTT not connected." CR);
    }
}

bool MqttClient::isConnected() {
    return _client.connected();
}

void MqttClient::loop() {
    if (_client.connected()) {
        _client.loop();
    }
}

// --- CALLBACK MQTT ---

void MqttClient::setCallback(MQTT_CALLBACK_SIGNATURE) {
    this->callback = callback;
}

void MqttClient::internalCallback(char* topic, byte* payload, unsigned int length) {
    if (mqttActiveInstance && mqttActiveInstance->callback) {
        mqttActiveInstance->callback(topic, payload, length);
    }
}

// --- LED MQTT ---

void MqttClient::updateLed() {
    digitalWrite(MQTT_LED_PIN, _client.connected() ? HIGH : LOW);
}

void MqttClient::blinkDuringConnection() {
    digitalWrite(MQTT_LED_PIN, HIGH);
    delay(BLINK_DELAY);
    digitalWrite(MQTT_LED_PIN, LOW);
    delay(BLINK_DELAY);
}
