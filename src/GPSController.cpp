#include "GPSController.h"

GPSController::GPSController(uint8_t serialPort) : _gpsSerial(serialPort) {
    _data.latitude = 0.0;
    _data.longitude = 0.0;
    _data.altitude = 0.0;
    _data.speedMps = 0.0;
    _data.speedKmph = 0.0;
    _data.totalDistanceMeters = 0.0;
    _data.satellites = 0;
    _data.isUpdated = false;
    _data.isValid = false;
    
    _previousLat = 0.0;
    _previousLng = 0.0;
}

void GPSController::begin(uint32_t baudRate, uint8_t rxPin, uint8_t txPin) {
    _gpsSerial.begin(baudRate, SERIAL_8N1, rxPin, txPin);
}

void GPSController::update() {
    _data.isUpdated = false; // Reset the updated flag for this cycle

    while (_gpsSerial.available() > 0) {
        char c = _gpsSerial.read();
        _gps.encode(c);
    }
    
    // Always track satellites even if location is not fully updated
    _data.satellites = _gps.satellites.value();

    if (_gps.location.isUpdated()) {
        _data.isValid = _gps.location.isValid();
        
        if (_data.isValid) {
            double currentLat = _gps.location.lat();
            double currentLng = _gps.location.lng();
            
            // Calculate distance if we have a previous fix
            if (_previousLat != 0.0 && _previousLng != 0.0) {
                double dist = _gps.distanceBetween(_previousLat, _previousLng, currentLat, currentLng);
                // Filter out jitter (e.g. only add if moved more than 0.5 meters)
                if (dist > 0.5) {
                    _data.totalDistanceMeters += dist;
                }
            }
            
            _previousLat = currentLat;
            _previousLng = currentLng;
            
            _data.latitude = currentLat;
            _data.longitude = currentLng;
            _data.altitude = _gps.altitude.meters();
            
            // --- Add Speed Calculations ---
            _data.speedMps = _gps.speed.mps();   // Meters per second
            _data.speedKmph = _gps.speed.kmph(); // Kilometers per hour
        }
        
        _data.isUpdated = true;
    }
}

const GPSController::LocationData& GPSController::getData() const {
    return _data;
}

bool GPSController::isLocationUpdated() const {
    return _data.isUpdated;
}

void GPSController::printLocation() const {
    if (_data.isUpdated) {
        Serial.print("Latitude: ");
        Serial.println(_data.latitude, 6);

        Serial.print("Longitude: ");
        Serial.println(_data.longitude, 6);

        Serial.print("Satellites: ");
        Serial.println(_data.satellites);

        Serial.print("Altitude: ");
        Serial.println(_data.altitude);

        Serial.println("-----------------------");
    } else {
        Serial.println("Waiting for GPS fix...");
    }
}
