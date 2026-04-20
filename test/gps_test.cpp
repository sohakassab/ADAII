#include <Arduino.h>
#include "Config.h"
#include "GPSController.h"

GPSController gpsController;

void setup() {
	Serial.begin(115200);
	gpsController.begin();
	Serial.println("GPS Started...");
}

void loop() {
	gpsController.update();

	if (gpsController.isLocationUpdated()) {
		gpsController.printLocation();
	}

	delay(1000);
}