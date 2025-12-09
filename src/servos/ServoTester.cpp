#include "ServoTester.h"
#include <ArduinoLog.h>

ServoTester::ServoTester(uint8_t i2cAddress, uint8_t servoCount)
    : _i2cAddress(i2cAddress), _servoCount(servoCount) {}

void ServoTester::begin(TwoWire* wire) {
    _wire = wire;
}

void ServoTester::tick() {
    if (!_wire) return;

    unsigned long now = millis();
    if (now - _lastToggle < TOGGLE_INTERVAL_MS) {
        return;
    }

    _lastToggle = now;
    _state = !_state;

    uint8_t targetAngle = _state ? ANGLE_POS1 : ANGLE_POS0;
    Log.notice("Servo test: setting %d servos to %d degrees" CR, _servoCount, targetAngle);
    sendAll(targetAngle);
}

void ServoTester::sendAll(uint8_t angle) {
    _wire->beginTransmission(_i2cAddress);
    for (uint8_t i = 0; i < _servoCount; i++) {
        _wire->write(angle);
    }
    _wire->endTransmission();
}
