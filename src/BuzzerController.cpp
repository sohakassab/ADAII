#include "BuzzerController.h"
#include <Arduino.h>

BuzzerController::BuzzerController()
    : _pin(0), _active(false), _startTime(0), _duration(0) {}

void BuzzerController::begin(uint8_t pin) {
    _pin = pin;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    Serial.printf("[Buzzer] Initialized on pin %d\n", _pin);
}

void BuzzerController::update() {
    if (_active && (millis() - _startTime >= _duration)) {
        stop();
    }
}

void BuzzerController::buzzFor(unsigned long durationMs) {
    if (durationMs == 0) {
        stop();
        return;
    }
    _duration  = durationMs;
    _startTime = millis();
    _active    = true;
    digitalWrite(_pin, HIGH);
    Serial.printf("[Buzzer] ON for %lu ms\n", durationMs);
}

void BuzzerController::stop() {
    if (!_active) return;
    _active = false;
    digitalWrite(_pin, LOW);
    Serial.println("[Buzzer] OFF");
}

bool BuzzerController::isActive() const {
    return _active;
}
