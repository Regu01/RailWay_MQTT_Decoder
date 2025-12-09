#include "TurnoutSignals.h"
#include "mqtt/MqttClient.h"
#include <strings.h>

// Mapping built from the sample Signals[2][42] arrays:
// First row -> expander index; second row -> pin number on that expander.
const TurnoutLedMapping TURNOUT_LED_MAP[TURNOUT_LED_COUNT] = {
    // ledId, group, color, expander, pin
    {  1,  1, 0, 0,  6}, // R1
    {  2,  1, 1, 0,  7}, // Y1
    {  3,  1, 2, 0,  2}, // G1
    {  4,  2, 0, 0,  3}, // R2
    {  5,  2, 1, 0,  4}, // Y2
    {  6,  2, 2, 0,  5}, // G2
    {  7,  3, 0, 0,  1}, // R3
    {  8,  3, 1, 0,  0}, // Y3
    {  9,  4, 0, 0, 12}, // R4
    { 10,  4, 1, 0, 13}, // Y4
    { 11,  4, 2, 0, 14}, // G4
    { 12,  5, 0, 0, 15}, // R5
    { 13,  5, 1, 0, 11}, // Y5
    { 14,  5, 2, 0, 10}, // G5
    { 15,  6, 0, 0,  9}, // R6
    { 16,  6, 1, 0,  8}, // Y6
    { 17,  7, 0, 1, 14}, // R7
    { 18,  7, 1, 1, 15}, // Y7
    { 19,  7, 2, 1, 12}, // G7
    { 20,  8, 0, 1, 13}, // R8
    { 21,  8, 1, 1,  8}, // Y8
    { 22,  8, 2, 1,  9}, // G8
    { 23,  9, 0, 1, 10}, // R9
    { 24,  9, 1, 1, 11}, // Y9
    { 25,  9, 2, 1,  7}, // G9
    { 26, 10, 0, 1,  6}, // R10
    { 27, 10, 1, 1,  5}, // Y10
    { 28, 10, 2, 1,  4}, // G10
    { 29, 11, 0, 1,  2}, // R11
    { 30, 11, 1, 1,  3}, // Y11
    { 31, 11, 2, 1,  1}, // G11
    { 32, 12, 0, 1,  0}, // R12
    { 33, 12, 1, 1, 12}, // Y12
    { 34, 12, 2, 2, 13}, // G12
    { 35, 13, 0, 2, 14}, // R13
    { 36, 13, 1, 2, 15}, // Y13
    { 37, 13, 2, 2, 11}, // G13
    { 38, 14, 0, 2, 10}, // R14
    { 39, 14, 1, 2,  9}, // Y14
    { 40, 14, 2, 2,  8}, // G14
    { 41, 14, 0, 2,  7}, // extra (kept from provided map)
    { 42, 14, 1, 2,  6}  // extra (kept from provided map)
};

TurnoutSignals::TurnoutSignals(uint8_t boardId, Expander* expanders, size_t expanderCount)
    : _boardId(boardId), _expanders(expanders), _expanderCount(expanderCount) {}

void TurnoutSignals::setupOutputs() {
    if (!_expanders) return;
    for (const auto& map : TURNOUT_LED_MAP) {
        if (map.expanderIndex >= _expanderCount) continue;
        _expanders[map.expanderIndex].direction(static_cast<PCA95x5::Port::Port>(map.pin), PCA95x5::Direction::OUT);
        _expanders[map.expanderIndex].write(static_cast<PCA95x5::Port::Port>(map.pin), PCA95x5::Level::H); // OFF by default (active LOW)
    }
}

bool TurnoutSignals::handleMqttMessage(const char* topic, const char* payload) {
    if (!topic || !payload) return false;

    uint8_t ledId = 0;
    if (!parseTopic(topic, ledId)) {
        return false;
    }

    bool on = false;
    if (strcasecmp(payload, "CLOSED") == 0 || strcasecmp(payload, "OFF") == 0 || strcmp(payload, "0") == 0) {
        on = false; // CLOSED => LED OFF
    } else if (strcasecmp(payload, "THROWN") == 0 || strcasecmp(payload, "ON") == 0 || strcmp(payload, "1") == 0) {
        on = true;  // THROWN => LED ON
    } else {
        return false; // Unknown payload
    }

    return setLed(ledId, on);
}

bool TurnoutSignals::setLed(uint8_t ledId, bool on) {
    if (ledId == 0 || ledId > TURNOUT_LED_COUNT) {
        return false;
    }
    const TurnoutLedMapping& map = TURNOUT_LED_MAP[ledId - 1];
    // Skip write if state unchanged
    if (_states[ledId - 1] == on) {
        return true;
    }
    bool ok = writeLed(map, on);
    if (ok) {
        _states[ledId - 1] = on;
    }
    return ok;
}

bool TurnoutSignals::setGroupAspect(uint8_t groupIndex, const char* aspect) {
    if (!aspect || groupIndex == 0 || groupIndex > 14) return false;

    int onColor = -1;
    if (strcasecmp(aspect, "RED") == 0) onColor = 0;
    else if (strcasecmp(aspect, "YELLOW") == 0) onColor = 1;
    else if (strcasecmp(aspect, "GREEN") == 0) onColor = 2;
    else if (strcasecmp(aspect, "OFF") == 0) onColor = -2;
    else return false;

    bool ok = true;
    for (const auto& map : TURNOUT_LED_MAP) {
        if (map.groupIndex != groupIndex) continue;
        bool turnOn = (onColor >= 0 && map.colorIndex == static_cast<uint8_t>(onColor));
        ok &= writeLed(map, turnOn);
    }
    return ok;
}

bool TurnoutSignals::parseTopic(const char* topic, uint8_t& ledId) const {
    const char base[] = "trains/track/turnout/";

    size_t baseLen = strlen(base);
    if (strncmp(topic, base, baseLen) != 0) {
        return false;
    }

    const char* afterBase = topic + baseLen;

    uint16_t globalIdRaw = static_cast<uint16_t>(atoi(afterBase));
    if (globalIdRaw < TURNOUT_ID_OFFSET) {
        return false;
    }
    uint16_t adjusted = globalIdRaw - TURNOUT_ID_OFFSET;
    uint8_t boardFromTopic = adjusted / 100;
    uint8_t localId = adjusted % 100;

    if (boardFromTopic != _boardId || localId == 0 || localId > TURNOUT_LED_COUNT) {
        return false;
    }

    ledId = localId;
    return true;
}

bool TurnoutSignals::writeLed(const TurnoutLedMapping& map, bool on) {
    if (!_expanders || map.expanderIndex >= _expanderCount) {
        return false;
    }

    auto port = static_cast<PCA95x5::Port::Port>(map.pin);
    _expanders[map.expanderIndex].write(port, on ? PCA95x5::Level::L : PCA95x5::Level::H);
    return true;
}

uint16_t TurnoutSignals::globalId(uint8_t localLedId) const {
    if (localLedId == 0 || localLedId > TURNOUT_LED_COUNT) return 0;
    return TURNOUT_ID_OFFSET + static_cast<uint16_t>(_boardId) * 100 + localLedId;
}

void TurnoutSignals::publishAll(MqttClient& client, const char* payload) {
    if (!payload) return;
    char topic[64];
    for (uint8_t i = 1; i <= TURNOUT_LED_COUNT; i++) {
        uint16_t gid = globalId(i);
        snprintf(topic, sizeof(topic), "trains/track/turnout/%u", gid);
        client.publish(topic, payload);
    }
}
