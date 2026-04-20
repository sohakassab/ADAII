#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H

#include <Arduino.h>

// --- Buzzer Controller ---
// Controls a buzzer on BUZZER_PIN.
// The backend publishes a JSON payload to the topic:
//   devices/{UUID}/cmd/buzzer
// Payload format:  {"duration_ms": 3000}
// The ESP32 will sound the buzzer for the specified duration (non-blocking).

class BuzzerController {
public:
    BuzzerController();

    // Call in setup() with the GPIO pin number
    void begin(uint8_t pin);

    // Call in loop() — handles timed shut-off without blocking
    void update();

    // Trigger the buzzer for the given number of milliseconds
    // Calling while already active will extend/override the remaining time.
    void buzzFor(unsigned long durationMs);

    // Immediately silence the buzzer
    void stop();

    // Returns true while the buzzer is actively sounding
    bool isActive() const;

private:
    uint8_t        _pin;
    bool           _active;
    unsigned long  _startTime;
    unsigned long  _duration;
};

#endif // BUZZER_CONTROLLER_H
