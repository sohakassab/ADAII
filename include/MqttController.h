#ifndef MQTT_CONTROLLER_H
#define MQTT_CONTROLLER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "BuzzerController.h"

class MqttController {
public:
    MqttController();

    // Call this in setup() after WiFi is connected to initialize MQTT
    void begin(const String& deviceUUID);

    // Register peripheral controllers so the MQTT callback can drive them
    void setBuzzerController(BuzzerController* buzzer);

    // Call this in loop() to handle messages and reconnects
    void update();

    // Publish data to a specific subtopic: devices/{uuid}/{subTopic}
    bool publish(const String& subTopic, const String& payload);

private:
    WiFiClientSecure _espClient;
    PubSubClient _client;
    String _uuid;
    unsigned long _lastReconnectAttempt;

    // Generated topic paths
    String _lwtTopic;

    // Pointer to the buzzer (set via setBuzzerController)
    static BuzzerController* _buzzer;

    void _reconnect();
    static void _callback(char* topic, byte* payload, unsigned int length);
};

#endif // MQTT_CONTROLLER_H
