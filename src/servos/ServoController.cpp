#include "ServoController.h"
#include <ArduinoLog.h>
#include "mqtt/MqttClient.h"

ServoController::ServoController(uint8_t boardId, uint8_t i2cAddress, uint8_t servoCount)
    : _boardId(boardId), _i2cAddress(i2cAddress), _servoCount(servoCount > 16 ? 16 : servoCount)
{
    for (uint8_t i = 0; i < 16; i++) {
        _closed[i] = DEFAULT_CLOSED;
        _thrown[i] = DEFAULT_THROWN;
        _isThrown[i] = false; // start CLOSED
    }
}

void ServoController::begin(TwoWire* wire) {
    _wire = wire;
    sendAll();
}

void ServoController::publishAll(MqttClient& client) {
    char topic[64];
    for (uint8_t i = 1; i <= _servoCount; i++) {
        uint16_t globalId = static_cast<uint16_t>(_boardId) * 100 + i;
        snprintf(topic, sizeof(topic), "trains/track/turnout/%u", globalId);
        client.publish(topic, _isThrown[i - 1] ? "THROWN" : "CLOSED");
    }
}

bool ServoController::handleTrackMessage(const char* topic, const char* payload) {
    uint8_t localId = 0;
    if (!parseTrackTopic(topic, localId)) {
        return false;
    }

    bool makeThrown = false;
    if (strcasecmp(payload, "THROWN") == 0 || strcasecmp(payload, "ON") == 0 || strcmp(payload, "1") == 0) {
        makeThrown = true;
    } else if (strcasecmp(payload, "CLOSED") == 0 || strcasecmp(payload, "OFF") == 0 || strcmp(payload, "0") == 0) {
        makeThrown = false;
    } else {
        return false;
    }

    uint8_t idx = localId - 1;
    if (_isThrown[idx] == makeThrown) {
        return true; // no change
    }
    _isThrown[idx] = makeThrown;
    sendAll();
    return true;
}

bool ServoController::handleConfigMessage(const char* topic, const char* payload) {
    uint8_t localId = 0;
    if (!parseConfigTopic(topic, localId)) {
        return false;
    }

    uint8_t c = DEFAULT_CLOSED;
    uint8_t t = DEFAULT_THROWN;
    if (!parseConfigPayload(payload, c, t)) {
        return false;
    }

    uint8_t idx = localId - 1;
    _closed[idx] = c;
    _thrown[idx] = t;
    Log.notice("Servo cfg: id %u closed=%u thrown=%u" CR, localId, c, t);
    sendAll();
    return true;
}

void ServoController::sendAll() {
    if (!_wire) return;

    _wire->beginTransmission(_i2cAddress);
    for (uint8_t i = 0; i < _servoCount; i++) {
        uint8_t angle = _isThrown[i] ? _thrown[i] : _closed[i];
        _wire->write(angle);
    }
    _wire->endTransmission();
}

bool ServoController::parseTrackTopic(const char* topic, uint8_t& localId) const {
    const char base[] = "trains/track/turnout/";
    size_t baseLen = strlen(base);
    if (strncmp(topic, base, baseLen) != 0) {
        return false;
    }
    uint16_t globalId = static_cast<uint16_t>(atoi(topic + baseLen));
    uint8_t board = globalId / 100;
    uint8_t local = globalId % 100;
    if (board != _boardId || local == 0 || local > _servoCount) {
        return false;
    }
    localId = local;
    return true;
}

bool ServoController::parseConfigTopic(const char* topic, uint8_t& localId) const {
    const char base[] = "trains/config/servo/";
    size_t baseLen = strlen(base);
    if (strncmp(topic, base, baseLen) != 0) {
        return false;
    }
    uint16_t globalId = static_cast<uint16_t>(atoi(topic + baseLen));
    uint8_t board = globalId / 100;
    uint8_t local = globalId % 100;
    if (board != _boardId || local == 0 || local > _servoCount) {
        return false;
    }
    localId = local;
    return true;
}

uint8_t ServoController::clampAngle(int value) const {
    if (value < MIN_ANGLE) return MIN_ANGLE;
    if (value > MAX_ANGLE) return MAX_ANGLE;
    return static_cast<uint8_t>(value);
}

bool ServoController::parseConfigPayload(const char* payload, uint8_t& closed, uint8_t& thrown) const {
    if (!payload) return false;
    // Accept minimal JSON like {"closed":70,"thrown":110} or {"c":70,"t":110}
    int c = -1, t = -1;
    const char* cPos = strcasestr(payload, "\"closed\"");
    if (!cPos) cPos = strcasestr(payload, "\"c\"");
    if (cPos) {
        cPos = strchr(cPos, ':');
        if (cPos) c = atoi(cPos + 1);
    }
    const char* tPos = strcasestr(payload, "\"thrown\"");
    if (!tPos) tPos = strcasestr(payload, "\"t\"");
    if (tPos) {
        tPos = strchr(tPos, ':');
        if (tPos) t = atoi(tPos + 1);
    }

    if (c < 0 && t < 0) {
        // fallback: try two numbers "70,110"
        c = atoi(payload);
        const char* comma = strchr(payload, ',');
        if (comma) {
            t = atoi(comma + 1);
        }
    }

    if (c < 0) c = DEFAULT_CLOSED;
    if (t < 0) t = DEFAULT_THROWN;

    closed = clampAngle(c);
    thrown = clampAngle(t);
    return true;
}
