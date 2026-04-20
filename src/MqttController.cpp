#include "MqttController.h"
#include "Config.h"
#include <ArduinoJson.h>

// Static member definition
BuzzerController* MqttController::_buzzer = nullptr;

MqttController::MqttController() : _client(_espClient), _lastReconnectAttempt(0) {
}

void MqttController::begin(const String& deviceUUID) {
    _uuid = deviceUUID;
    _lwtTopic = "devices/" + _uuid + "/status";

    _espClient.setInsecure();

    // Increase buffer from the default 256 bytes — our JSON payload is ~320+ bytes.
    // publish() silently returns false if the payload exceeds this limit.
    _client.setBufferSize(512);

    _client.setServer(MQTT_SERVER, MQTT_PORT);
    _client.setCallback(MqttController::_callback);

    _reconnect();
}

void MqttController::setBuzzerController(BuzzerController* buzzer) {
    _buzzer = buzzer;
}

void MqttController::_reconnect() {
    if (_client.connected()) return;

    Serial.print("Attempting MQTT connection...");
    
    // Reverted to your exact working example without LWT to guarantee HiveMQ Free Tier compatibility
    if (_client.connect(_uuid.c_str(), MQTT_USER, MQTT_PASS)) {
        Serial.println("connected");
        
        // Publish online status manually
        _client.publish(_lwtTopic.c_str(), "online", false);

        // We can subscribe to paths specifically for this device
        String cmdTopic = "devices/" + _uuid + "/cmd/#";
        _client.subscribe(cmdTopic.c_str());
    } else {
        Serial.print("failed, rc=");
        Serial.println(_client.state());
    }
}

void MqttController::update() {
    if (!_client.connected()) {
        unsigned long currentMillis = millis();
        // Non-blocking reconnect every 5 seconds
        if (currentMillis - _lastReconnectAttempt > 5000) {
            _lastReconnectAttempt = currentMillis;
            _reconnect();
        }
    } else {
        // Only call loop if connected
        _client.loop();
    }
}

bool MqttController::publish(const String& subTopic, const String& payload) {
    if (!_client.connected()) return false;
    
    String fullTopic = "devices/" + _uuid + "/" + subTopic;
    return _client.publish(fullTopic.c_str(), payload.c_str());
}

void MqttController::_callback(char* topic, byte* payload, unsigned int length) {
    String topicStr(topic);
    String payloadStr;
    for (unsigned int i = 0; i < length; i++) payloadStr += (char)payload[i];

    Serial.print("[MQTT] Message arrived [");
    Serial.print(topicStr);
    Serial.print("] ");
    Serial.println(payloadStr);

    // -------------------------------------------------------
    // Route: devices/{UUID}/cmd/buzzer
    // Payload JSON: {"duration_ms": 3000}
    // -------------------------------------------------------
    if (topicStr.endsWith("/cmd/buzzer")) {
        if (_buzzer == nullptr) {
            Serial.println("[MQTT] BuzzerController not registered!");
            return;
        }

        // Parse JSON payload
        StaticJsonDocument<64> doc;
        DeserializationError err = deserializeJson(doc, payloadStr);
        if (err) {
            Serial.print("[MQTT] Buzzer JSON parse error: ");
            Serial.println(err.c_str());
            return;
        }

        unsigned long durationMs = doc["duration_ms"] | 0UL;

        // Enforce safety cap from Config.h
        if (durationMs > BUZZER_MAX_DURATION_MS) {
            durationMs = BUZZER_MAX_DURATION_MS;
            Serial.printf("[MQTT] Buzzer duration capped to %lu ms\n", durationMs);
        }

        if (durationMs == 0) {
            _buzzer->stop();
        } else {
            _buzzer->buzzFor(durationMs);
        }
        return;
    }

    // -------------------------------------------------------
    // Add further command handlers below
    // -------------------------------------------------------
}
