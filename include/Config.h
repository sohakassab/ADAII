#ifndef CONFIG_H
#define CONFIG_H

// --- Device Settings ---
#define DEVICE_UUID "Player1_ESP32" // Set your constant UUID for this device here

// --- WiFi Settings ---
#define WIFI_SSID "real"
#define WIFI_PASSWORD "12345678a"

// --- MQTT Settings ---
#define MQTT_SERVER "162dfd8d0eec4ab3866fe6a9eec7a2ac.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USER "Player1"
#define MQTT_PASS "aA12345678"



// --- MPU6500 Settings ---
#define MPU_ADDRESS 0x69   // Default is 0x68, your board uses 0x69
#define SDA_PIN 21
#define SCL_PIN 22

// --- GPS Settings ---
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define GPS_BAUD 9600

// --- Buzzer Settings ---
#define BUZZER_PIN 19           // GPIO pin connected to the buzzer
#define BUZZER_MAX_DURATION_MS 30000  // Safety cap: never buzz longer than 30 s

// --- Sports Metrics Thresholds ---
#define SPRINT_THRESHOLD 1.5f        // IMU g-force required to trigger a sprint tracking block
#define SPRINT_SUSTAIN_MS 500        // Player must maintain g-force > threshold for 500ms
#define SPRINT_COOLDOWN_MS 3000      // Wait 3 seconds after a sprint before allowing another
#define GPS_SPRINT_SPEED_MPS 5.0f    // 5.0 m/s (approx 18 km/h) triggers instant sprint from GPS

#define IMPACT_THRESHOLD 4.0f        // g-force threshold for a significant impact
#define PLAYER_LOAD_SCALE 0.01f // Scaling factor for Player Load accumulation

// --- Filter Settings ---
#define COMPLEMENTARY_FILTER_ALPHA 0.98f // Weight given to gyro in orientation calc

#endif // CONFIG_H
