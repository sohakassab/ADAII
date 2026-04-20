#ifndef MPU6500_H
#define MPU6500_H

#include <Arduino.h>
#include <Wire.h>
#include "Config.h"

class MPU6500 {
public:
    struct SensorData {
        float accelX;
        float accelY;
        float accelZ;
        float gyroX;
        float gyroY;
        float gyroZ;
        float temperature;
        float pitch;
        float roll;
        float yaw;
        // Advanced Sports Metrics
        float accelMagnitude;
        float playerLoad;
        float maxImpact;
        uint32_t sprintCount;
    };

    MPU6500(uint8_t address = 0x68);

    bool begin(TwoWire& wirePort = Wire);
    void readSensor();
    
    // Call this if you want to calculate pitch, roll, yaw
    void calcOrientation(); 
    
    // Call this to update Player Load, Impacts, and Sprint Counting
    void calcSportsMetrics(float currentGpsSpeedMps = 0.0f);

    // Reset accumulated metrics (Player Load, Sprint Count, Impact)
    void resetSportsMetrics();

    float getAccelX() const;
    float getAccelY() const;
    float getAccelZ() const;

    float getGyroX() const;
    float getGyroY() const;
    float getGyroZ() const;

    float getTemperature() const;

    float getPitch() const;
    float getRoll() const;
    float getYaw() const;

    float getAccelMagnitude() const;
    float getPlayerLoad() const;
    float getMaxImpact() const;
    uint32_t getSprintCount() const;

    const SensorData& getData() const;

private:
    uint8_t _address;
    TwoWire* _wire;
    SensorData _data;

    // Default scale factors: 
    // Accel FS_SEL=0 (±2g) -> 16384 LSB/g
    // Gyro FS_SEL=0 (±250 dps) -> 131.0 LSB/dps
    const float _accelScale = 16384.0f;
    const float _gyroScale = 131.0f;

    int16_t _rawAccelX, _rawAccelY, _rawAccelZ;
    int16_t _rawGyroX, _rawGyroY, _rawGyroZ;
    int16_t _rawTemp;

    // For orientation calculation
    unsigned long _lastCalcTime;

    // For sports metrics tracking
    float _prevAccelX, _prevAccelY, _prevAccelZ;
    bool _isSprinting;
    unsigned long _sprintSustainStartTime;
    unsigned long _lastSprintFinishTime;
    bool _imuPotentialSprint;
    
    // Constants for metrics
    const float _sprintThreshold = SPRINT_THRESHOLD;
    const float _impactThreshold = IMPACT_THRESHOLD;
    const float _playerLoadScale = PLAYER_LOAD_SCALE;
};

#endif // MPU6500_H
