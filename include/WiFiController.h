#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiController {
public:
    WiFiController();

    // Begin Wi-Fi connection process
    void begin();
    
    // Call in the main loop to handle reconnections if needed
    void update();

    // Return whether Wi-Fi is connected
    bool isConnected() const;

    // Get the ESP32 MAC Address as a String to use as a UUID
    String getUUID() const;

private:
    String _uuid;
    unsigned long _lastReconnectAttempt;
    
    void _connect();
};

#endif // WIFI_CONTROLLER_H
