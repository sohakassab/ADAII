#include <Arduino.h>
#include "Config.h"
#include "GPSController.h"
#include "WiFiController.h"
#include "MqttController.h"
#include "MPU6500.h"
#include "PolarController.h"
#include "BuzzerController.h"

GPSController gpsController;
WiFiController wifiController;
MqttController mqttController;
MPU6500 mpu;
PolarController polarController;
BuzzerController buzzerController;
bool mpuReady = false;

void setup() {
	Serial.begin(115200);
	
	// 1. Initialize GPS
	gpsController.begin();
	Serial.println("GPS Started...");

	// 2. Initialize Wi-Fi
	wifiController.begin();

	// 3. Initialize MQTT (must be after WiFi)
	mqttController.begin(wifiController.getUUID());

	// 4. Initialize Buzzer
	buzzerController.begin(BUZZER_PIN);
	mqttController.setBuzzerController(&buzzerController);

	// 4. Initialize MPU6500 — auto-scans 0x69 then 0x68
	Wire.begin(SDA_PIN, SCL_PIN);
	mpuReady = mpu.begin();
	if (!mpuReady) {
		Serial.println("MPU6500 not found on either address. Check wiring.");
	} else {
		Serial.println("MPU6500 Initialized.");
	}

	// 5. Start BLE scan for Polar HR sensor
	polarController.begin();
}

void loop() {
	// Update hardware & network components
	gpsController.update();
	wifiController.update();
	mqttController.update();
	polarController.update();
	buzzerController.update();
	
	// Read IMU — always attempt once initialized
	if (mpuReady) {
		mpu.readSensor();
		mpu.calcOrientation();
		double currentSpeed = gpsController.getData().speedMps;
		mpu.calcSportsMetrics(currentSpeed);
	}

	static unsigned long lastMsg = 0;
	if (millis() - lastMsg > 5000) {
		lastMsg = millis();
		
		// Publish fused data every 5 seconds
		const auto& locData = gpsController.getData();
		const auto& mpuData = mpu.getData();
		
		// Create a comprehensive JSON payload
		String payload = "{";
		
		// GPS Position, Speed, and Distance
		// NOTE: isValid persists once a fix is acquired.
		// isUpdated is only true for ONE loop cycle after a new NMEA sentence —
		// never gate on (isUpdated && isValid) here or you land in NO_FIX almost always.
		if (locData.isValid) {
			payload += "\"lat\":" + String(locData.latitude, 20) + ",";
			payload += "\"lng\":" + String(locData.longitude, 20) + ",";
			payload += "\"speed_mps\":" + String(locData.speedMps, 2) + ",";
			payload += "\"distance_m\":" + String(locData.totalDistanceMeters, 2) + ",";
		} else {
			// No valid fix yet — satellites still acquiring
			payload += "\"location_status\":\"NO_FIX\",";
		}
		
		// Always report satellite count
		payload += "\"satellites\":" + String(locData.satellites) + ",";
		
		// Track 3D Acceleration Vector
		payload += "\"accel_x\":" + String(mpuData.accelX, 2) + ",";
		payload += "\"accel_y\":" + String(mpuData.accelY, 2) + ",";
		payload += "\"accel_z\":" + String(mpuData.accelZ, 2) + ",";
		
		// Sports Metrics
		payload += "\"sprints\":" + String(mpuData.sprintCount) + ",";
		payload += "\"player_load\":" + String(mpuData.playerLoad, 2) + ",";
		
		// Biometrics from Polar BLE sensor + MPU chip temp
		const auto& polar = polarController.getData();
		payload += "\"polar_connected\":" + String(polar.isConnected ? "true" : "false") + ",";
		payload += "\"body_temp\":" + String(mpuData.temperature, 2) + ",";
		payload += "\"heart_rate\":" + String(polar.heartRate) + ",";
		payload += "\"hrv\":" + String(polar.hrv, 1) + ",";
		payload += "\"respiratory_rate\":" + String(polar.respiratoryRate, 1);
		payload += "}";
		
		if (mqttController.publish("data", payload)) {
			Serial.println("[MQTT] Published " + String(payload.length()) + " bytes: " + payload);
		} else {
			Serial.println("[MQTT] Publish FAILED — not connected or payload too large (" + String(payload.length()) + " bytes)");
		}
	}
}


