#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoLog.h>
#include <vector>

// Trick to store the active instance (PubSubClient imposes a static callback)
class MqttClient;
extern MqttClient* mqttActiveInstance;

class MqttClient {
public:
    MqttClient(const char* server, int port, const char* user, const char* password, const char* clientId);

    void connect();
    void disconnect();
    void publish(const char* topic, const char* message);
    void subscribe(const char* topic);
    bool isConnected();
    void loop();

    void setWill(const char* topic, const char* message, bool retain = true, uint8_t qos = 1);

    // Let callers define their own callback
    void setCallback(MQTT_CALLBACK_SIGNATURE);

private:
    const char* _server;
    int _port;
    const char* _user;
    const char* _password;
    const char* _clientId;

    WiFiClient _wifiClient;
    PubSubClient _client;

    // MQTT LED (active LOW to align with Wi-Fi LED)
    const int MQTT_LED_PIN = 19;
    const int BLINK_DELAY = 200;
    const unsigned long MQTT_RECONNECT_BASE_DELAY = 2000;
    const unsigned long MQTT_RECONNECT_MAX_DELAY = 20000;

    unsigned long _lastReconnectAttempt = 0;
    unsigned long _reconnectDelay = MQTT_RECONNECT_BASE_DELAY;

    // Stored user callback
    MQTT_CALLBACK_SIGNATURE;
    std::vector<String> _subscriptions;

    const char* _willTopic = nullptr;
    const char* _willMessage = nullptr;
    bool _willRetain = true;
    uint8_t _willQos = 1;

    static void internalCallback(char* topic, byte* payload, unsigned int length);
    void resubscribeStoredTopics();
    bool reconnectOnce(bool logOnFailure);

    void reconnect();
    void updateLed();
    void blinkDuringConnection();
};

#endif // MQTTCLIENT_H
