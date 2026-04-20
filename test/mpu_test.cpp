#include <Arduino.h>
#include <Wire.h>
#include "MPU6500.h"
#include "Config.h"

MPU6500 mpu(MPU_ADDRESS);

void setup()
{
	Serial.begin(115200);
	Wire.begin(SDA_PIN, SCL_PIN);

	if (!mpu.begin(Wire))
	{
		Serial.println("MPU6500 not found!");
		while (1)
			;
	}

	Serial.println("MPU6500 Controller Ready!");
	Serial.println("Sports Metrics Tracking Active...");
}

void loop()
{
	// 1. Read Raw/Scaled Data
	mpu.readSensor();

	// 2. Calculate Orientation (Pitch, Roll, Yaw)
	mpu.calcOrientation();

	// 3. Calculate Sports Metrics (Load, Impact, Sprints)
	mpu.calcSportsMetrics();

	// --- Output Data ---
	Serial.println("----------------------------------------");
	
	// Movement & Orientation
	Serial.print("Roll: ");  Serial.print(mpu.getRoll(), 2);  Serial.print("°  ");
	Serial.print("Pitch: "); Serial.print(mpu.getPitch(), 2); Serial.print("°  ");
	Serial.print("Yaw: ");   Serial.print(mpu.getYaw(), 2);   Serial.println("°");

	mpu.getData();
	// Acceleration & Gyro
	Serial.print("Accel Mag: "); Serial.print(mpu.getAccelMagnitude(), 3); Serial.print("g  ");
	Serial.print("Temp: ");     Serial.print(mpu.getTemperature(), 1);     Serial.println("°C");

	// Sports Specific Metrics
	Serial.print("Player Load: "); Serial.print(mpu.getPlayerLoad(), 4); Serial.print("  ");
	Serial.print("Max Impact: ");  Serial.print(mpu.getMaxImpact(), 2);  Serial.print("g  ");
	Serial.print("Sprints: ");    Serial.println(mpu.getSprintCount());

	delay(100); // Faster update for sports tracking
}