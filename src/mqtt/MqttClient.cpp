#include "MqttClient.h"

// Store the current instance so the static callback can forward properly
MqttClient* mqttActiveInstance = nullptr;

MqttClient::MqttClient(const char* server, int port, const char* user, const char* password, const char* clientId)
    : _server(server), _port(port), _user(user), _password(password), _clientId(clientId), _client(_wifiClient)
{
    mqttActiveInstance = this;
    _client.setServer(_server, _port);
    _client.setCallback(internalCallback);

    pinMode(MQTT_LED_PIN, OUTPUT);
    digitalWrite(MQTT_LED_PIN, HIGH); // LED OFF at startup (active LOW)
}

void MqttClient::connect() {
    _reconnectDelay = MQTT_RECONNECT_BASE_DELAY;
    reconnectOnce(true);
}

bool MqttClient::reconnectOnce(bool logOnFailure) {
    if (_client.connected()) {
        return true;
    }

    blinkDuringConnection();
    if (_client.connect(_clientId, _user, _password)) {
        Log.notice("Connected to MQTT broker." CR);
        resubscribeStoredTopics();
        _reconnectDelay = MQTT_RECONNECT_BASE_DELAY;
        updateLed();
        return true;
    }

    if (logOnFailure) {
        Log.error("Failed to connect to MQTT. State: %d" CR, _client.state());
    }

    updateLed();
    return false;
}

void MqttClient::reconnect() {
    reconnectOnce(false);
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
    if (!topic) {
        return;
    }

    bool known = false;
    for (auto& t : _subscriptions) {
        if (t.equals(topic)) {
            known = true;
            break;
        }
    }
    if (!known) {
        _subscriptions.emplace_back(topic);
    }

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
        _reconnectDelay = MQTT_RECONNECT_BASE_DELAY;
        updateLed();
        return;
    }

    unsigned long now = millis();
    if (now - _lastReconnectAttempt >= _reconnectDelay) {
        _lastReconnectAttempt = now;

        if (reconnectOnce(false)) {
            _reconnectDelay = MQTT_RECONNECT_BASE_DELAY;
        } else {
            unsigned long nextDelay = _reconnectDelay * 2;
            _reconnectDelay = nextDelay > MQTT_RECONNECT_MAX_DELAY ? MQTT_RECONNECT_MAX_DELAY : nextDelay;
        }
    }

    updateLed();
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

void MqttClient::resubscribeStoredTopics() {
    for (auto& t : _subscriptions) {
        _client.subscribe(t.c_str());
    }
}

// --- LED MQTT ---

void MqttClient::updateLed() {
    digitalWrite(MQTT_LED_PIN, _client.connected() ? LOW : HIGH); // Active LOW
}

void MqttClient::blinkDuringConnection() {
    digitalWrite(MQTT_LED_PIN, LOW);
    delay(BLINK_DELAY);
    digitalWrite(MQTT_LED_PIN, HIGH);
    delay(BLINK_DELAY);
}
