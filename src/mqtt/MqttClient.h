#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoLog.h>

// Trick pour stocker l'instance active (nécessaire car PubSubClient impose un callback static)
class MqttClient;
extern MqttClient* mqttActiveInstance;

class MqttClient {
public:
    MqttClient(const char* server, int port, const char* user, const char* password);

    void connect();
    void disconnect();
    void publish(const char* topic, const char* message);
    void subscribe(const char* topic);
    bool isConnected();
    void loop();

    // Pour que l'utilisateur puisse définir son callback
    void setCallback(MQTT_CALLBACK_SIGNATURE);

private:
    const char* _server;
    int _port;
    const char* _user;
    const char* _password;

    WiFiClient _wifiClient;
    PubSubClient _client;

    // LED MQTT
    const int MQTT_LED_PIN = 19;
    const int BLINK_DELAY = 400;

    // Callback utilisateur stocké
    MQTT_CALLBACK_SIGNATURE;

    // Callback interne static → redirige vers l'instance
    static void internalCallback(char* topic, byte* payload, unsigned int length);

    void reconnect();
    void updateLed();
    void blinkDuringConnection();
};

#endif // MQTTCLIENT_H
