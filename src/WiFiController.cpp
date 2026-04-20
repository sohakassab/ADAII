#include "WiFiController.h"
#include "Config.h"

WiFiController::WiFiController() : _lastReconnectAttempt(0) {
#ifdef DEVICE_UUID
    _uuid = DEVICE_UUID;
#else
    // Fallback to MAC address if no constant is defined
    _uuid = WiFi.macAddress();
    _uuid.replace(":", "");
    _uuid = "esp32_" + _uuid;
#endif
}

void WiFiController::begin() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to Wi-Fi SSID: ");
    Serial.println(WIFI_SSID);

    // Some cases require disconnecting first to assure a clean connection
    WiFi.disconnect(true);
    delay(100);

    WiFi.mode(WIFI_STA);
    _connect();
}

void WiFiController::_connect() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    Serial.print("Waiting for WiFi...");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected.");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        Serial.print("Device UUID: ");
        Serial.println(_uuid);
    } else {
        Serial.println("\nWiFi connection failed. Will retry later.");
    }
}

void WiFiController::update() {
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long currentMillis = millis();
        // Try to reconnect every 10 seconds
        if (currentMillis - _lastReconnectAttempt > 10000) {
            _lastReconnectAttempt = currentMillis;
            Serial.println("WiFi disconnected. Reconnecting...");
            WiFi.disconnect();
            WiFi.reconnect();
        }
    }
}

bool WiFiController::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiController::getUUID() const {
    return _uuid;
}
