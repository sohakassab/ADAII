#ifndef GPSCONTROLLER_H
#define GPSCONTROLLER_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "Config.h"

class GPSController {
public:
    struct LocationData {
        double latitude;
        double longitude;
        double altitude;
        double speedMps;     // Speed in Meters Per Second
        double speedKmph;    // Speed in Kilometers Per Hour
        double totalDistanceMeters; // Aggregated distance
        uint32_t satellites;
        bool isUpdated;
        bool isValid;        // To check if we explicitly have a fix
    };

    GPSController(uint8_t serialPort = 2);

    void begin(uint32_t baudRate = GPS_BAUD, uint8_t rxPin = GPS_RX_PIN, uint8_t txPin = GPS_TX_PIN);
    
    // Call this in the main loop to process incoming GPS data
    void update();

    const LocationData& getData() const;

    bool isLocationUpdated() const;
    void printLocation() const;

private:
    TinyGPSPlus _gps;
    HardwareSerial _gpsSerial;
    LocationData _data;
    
    double _previousLat;
    double _previousLng;
};

#endif // GPSCONTROLLER_H
