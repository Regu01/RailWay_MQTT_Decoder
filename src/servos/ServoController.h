#ifndef SERVO_CONTROLLER_H
#define SERVO_CONTROLLER_H

#include <Arduino.h>
#include <Wire.h>

class MqttClient;

class ServoController {
public:
    ServoController(uint8_t boardId, uint8_t i2cAddress = 0x08, uint8_t servoCount = 16);

    void begin(TwoWire* wire = &Wire);
    void publishAll(class MqttClient& client);  // publish current state for all servos
    void setErrorPublisher(MqttClient* client, const char* topic); // optional MQTT error reporting

    // MQTT handlers
    bool handleTrackMessage(const char* topic, const char* payload);   // trains/track/turnout/<globalId>
    bool handleConfigMessage(const char* topic, const char* payload);  // trains/config/servo/<globalId> JSON: {"closed":70,"thrown":110}

    bool sendAll();  // push current states to the ATmega, false on I2C error

private:
    uint8_t _boardId;
    uint8_t _i2cAddress;
    uint8_t _servoCount;
    TwoWire* _wire = nullptr;
    MqttClient* _mqtt = nullptr;
    const char* _errTopic = nullptr;

    static constexpr uint8_t DEFAULT_CLOSED = 70;
    static constexpr uint8_t DEFAULT_THROWN = 110;
    static constexpr uint8_t MIN_ANGLE = 20;
    static constexpr uint8_t MAX_ANGLE = 160;

    // Per-servo config/state
    uint8_t _closed[16];
    uint8_t _thrown[16];
    bool _isThrown[16];  // false = CLOSED, true = THROWN

    bool parseTrackTopic(const char* topic, uint8_t& localId) const;
    bool parseConfigTopic(const char* topic, uint8_t& localId) const;
    uint8_t clampAngle(int value) const;
    bool parseConfigPayload(const char* payload, uint8_t& closed, uint8_t& thrown) const;
};

#endif // SERVO_CONTROLLER_H
