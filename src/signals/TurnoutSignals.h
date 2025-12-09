#ifndef TURNOUT_SIGNALS_H
#define TURNOUT_SIGNALS_H

#include <Arduino.h>
#include <PCA95x5.h>

// Mapping for 14 groups of 3 LEDs (R, J, V) = 42 LEDs total.
// Each LED has a global numeric id (1..42) and a position on a PCA95x5 expander.
// MQTT topic scheme (global id): trains/track/turnout/<globalLedId>

constexpr uint8_t TURNOUT_LED_COUNT = 42;
using Expander = PCA95x5::PCA95x5<>;
constexpr uint16_t TURNOUT_ID_OFFSET = 20; // JMRI offset applied to MQTT ids

struct TurnoutLedMapping {
    uint8_t ledId;         // 1..42
    uint8_t groupIndex;    // 1..14 (LED group)
    uint8_t colorIndex;    // 0=Red, 1=Yellow, 2=Green
    uint8_t expanderIndex; // Which PCA95x5 (0..)
    uint8_t pin;           // Pin on the expander
};

// Fixed mapping derived from the provided Signals arrays.
extern const TurnoutLedMapping TURNOUT_LED_MAP[TURNOUT_LED_COUNT];

class TurnoutSignals {
public:
    TurnoutSignals(uint8_t boardId, Expander* expanders, size_t expanderCount);

    // Configure expander pins as outputs.
    void setupOutputs();

    // Parse a topic/payload and drive the addressed LED.
    // Expected topic: trains/track/turnout/<globalLedId> (globalLedId = boardId*100 + localLedId)
    // Payload: "CLOSED" turns the LED ON, "THROWN" turns it OFF (aliases: ON/OFF/1/0).
    bool handleMqttMessage(const char* topic, const char* payload);

    // Direct control helpers
    bool setLed(uint8_t ledId, bool on);
    bool setGroupAspect(uint8_t groupIndex, const char* aspect); // aspect: "RED","YELLOW","GREEN","OFF"
    uint16_t globalId(uint8_t localLedId) const;

    // Publish initial states for all LEDs (helper; implemented in .cpp)
    void publishAll(class MqttClient& client, const char* payload);

private:
    uint8_t _boardId;
    Expander* _expanders;
    size_t _expanderCount;
    bool _states[TURNOUT_LED_COUNT] = {false};

    bool parseTopic(const char* topic, uint8_t& ledId) const;
    bool writeLed(const TurnoutLedMapping& map, bool on);
};

#endif // TURNOUT_SIGNALS_H
