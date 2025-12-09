#ifndef SERVO_TESTER_H
#define SERVO_TESTER_H

#include <Arduino.h>
#include <Wire.h>

class ServoTester {
public:
    ServoTester(uint8_t i2cAddress = 0x08, uint8_t servoCount = 16);

    void begin(TwoWire* wire = &Wire);
    void tick();

private:
    TwoWire* _wire = nullptr;
    uint8_t _i2cAddress;
    uint8_t _servoCount;
    bool _state = false;
    unsigned long _lastToggle = 0;

    static constexpr unsigned long TOGGLE_INTERVAL_MS = 3000; // period between positions
    static constexpr uint8_t ANGLE_POS0 = 70;  // CLOSED test angle
    static constexpr uint8_t ANGLE_POS1 = 110; // THROWN test angle

    void sendAll(uint8_t angle);
};

#endif // SERVO_TESTER_H
