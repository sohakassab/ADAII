#include "MPU6500.h"

MPU6500::MPU6500(uint8_t address) : _address(address), _wire(nullptr) {
    // Initialize data array to zeros
    _data = {0};
}

bool MPU6500::begin(TwoWire& wirePort) {
    _wire = &wirePort;

    _wire->beginTransmission(_address);
    _wire->write(0x6B); // Power management register 1
    _wire->write(0x00); // Wake up sensor, set to internal clock
    
    if (_wire->endTransmission() != 0) {
        // Try the alternate address automatically
        uint8_t altAddress = (_address == 0x69) ? 0x68 : 0x69;
        _address = altAddress;
        
        _wire->beginTransmission(_address);
        _wire->write(0x6B);
        _wire->write(0x00);
        
        if (_wire->endTransmission() != 0) {
            _wire = nullptr; // Neither address responded
            return false;
        }
        Serial.print("MPU6500 found at alternate address 0x");
        Serial.println(_address, HEX);
    }

    _lastCalcTime = millis();
    _prevAccelX = 0;
    _prevAccelY = 0;
    _prevAccelZ = 0;
    _isSprinting = false;
    _sprintSustainStartTime = 0;
    _lastSprintFinishTime = 0;
    _imuPotentialSprint = false;
    resetSportsMetrics();

    return true;
}

void MPU6500::readSensor() {
    if (!_wire) return;

    _wire->beginTransmission(_address);
    _wire->write(0x3B); // Start reading from Accelerometer X High byte
    _wire->endTransmission(false);
    
    // Request 14 bytes (Accel X/Y/Z, Temp, Gyro X/Y/Z)
    _wire->requestFrom((int)_address, 14, (int)true);

    if (_wire->available() == 14) {
        _rawAccelX = _wire->read() << 8 | _wire->read();
        _rawAccelY = _wire->read() << 8 | _wire->read();
        _rawAccelZ = _wire->read() << 8 | _wire->read();

        _rawTemp   = _wire->read() << 8 | _wire->read();

        _rawGyroX  = _wire->read() << 8 | _wire->read();
        _rawGyroY  = _wire->read() << 8 | _wire->read();
        _rawGyroZ  = _wire->read() << 8 | _wire->read();

        // Convert raw values to standard units
        _data.accelX = _rawAccelX / _accelScale;
        _data.accelY = _rawAccelY / _accelScale;
        _data.accelZ = _rawAccelZ / _accelScale;

        // Temperature equation from MPU6500 datasheet
        _data.temperature = (_rawTemp / 333.87f) + 21.0f;

        _data.gyroX = _rawGyroX / _gyroScale;
        _data.gyroY = _rawGyroY / _gyroScale;
        _data.gyroZ = _rawGyroZ / _gyroScale;
    }
}

void MPU6500::calcOrientation() {
    unsigned long currentTime = millis();
    float dt = (currentTime - _lastCalcTime) / 1000.0f; // dt in seconds
    _lastCalcTime = currentTime;

    // Calculate Pitch & Roll from Accelerometer
    float accelPitch = atan2(_data.accelY, sqrt(_data.accelX * _data.accelX + _data.accelZ * _data.accelZ)) * 180.0 / PI;
    float accelRoll = atan2(-_data.accelX, _data.accelZ) * 180.0 / PI;

    // Complementary Filter
    float alpha = COMPLEMENTARY_FILTER_ALPHA; // Tuning parameter
    _data.pitch = alpha * (_data.pitch + _data.gyroX * dt) + (1.0f - alpha) * accelPitch;
    _data.roll = alpha * (_data.roll + _data.gyroY * dt) + (1.0f - alpha) * accelRoll;

    // Yaw cannot be corrected with an accelerometer, only relies on gyro
    _data.yaw += _data.gyroZ * dt;
}

void MPU6500::calcSportsMetrics(float currentGpsSpeedMps) {
    // 1. Calculate Acceleration Magnitude
    _data.accelMagnitude = sqrt(_data.accelX * _data.accelX + 
                                _data.accelY * _data.accelY + 
                                _data.accelZ * _data.accelZ);

    // 2. Track Maximum Impact
    if (_data.accelMagnitude > _impactThreshold && _data.accelMagnitude > _data.maxImpact) {
        _data.maxImpact = _data.accelMagnitude;
    }

    // 3. Calculate Player Load
    if (_prevAccelX != 0 || _prevAccelY != 0 || _prevAccelZ != 0) {
        float dx = _data.accelX - _prevAccelX;
        float dy = _data.accelY - _prevAccelY;
        float dz = _data.accelZ - _prevAccelZ;
        
        float instantaneousLoad = sqrt(dx*dx + dy*dy + dz*dz) * _playerLoadScale;
        _data.playerLoad += instantaneousLoad;
    }

    _prevAccelX = _data.accelX;
    _prevAccelY = _data.accelY;
    _prevAccelZ = _data.accelZ;

    // 4. Sprint Sensor Fusion (IMU Sustained + GPS)
    unsigned long currentMs = millis();
    
    // Are we still in cooldown phase? If so, we reject all new sprints.
    if (currentMs - _lastSprintFinishTime < SPRINT_COOLDOWN_MS) {
        _isSprinting = false;
        _imuPotentialSprint = false;
        return; // Exit out of the sprint logic entirely
    }

    // A: GPS Confidence Check
    bool isGpsSprinting = (currentGpsSpeedMps > GPS_SPRINT_SPEED_MPS);

    // B: IMU Confidence Check
    // A sprint must overcome the threshold continuously
    bool imuOverThreshold = (abs(_data.accelX) > _sprintThreshold);

    if (imuOverThreshold) {
        if (!_imuPotentialSprint) {
            // First time crossing the line, mark the time
            _imuPotentialSprint = true;
            _sprintSustainStartTime = currentMs;
        }
    } else {
        // Player dropped below the threshold, reset the potential sprint
        _imuPotentialSprint = false;
    }

    bool isImuSprinting = _imuPotentialSprint && ((currentMs - _sprintSustainStartTime) >= SPRINT_SUSTAIN_MS);

    // C: Combine them. Either GPS proves running, or IMU proves sustained acceleration
    bool currentIsSprinting = isGpsSprinting || isImuSprinting;
    
    if (currentIsSprinting && !_isSprinting) {
        // We just STARTED a real sprint!
        _data.sprintCount++;
        _isSprinting = true;
        Serial.println(">>> SPRINT REGISTERED! <<<");
    } else if (!currentIsSprinting && _isSprinting) {
        // We just FINISHED a sprint, enter cooldown
        _isSprinting = false;
        _imuPotentialSprint = false;
        _lastSprintFinishTime = currentMs;
    }
}

void MPU6500::resetSportsMetrics() {
    _data.playerLoad = 0.0f;
    _data.maxImpact = 0.0f;
    _data.sprintCount = 0;
}

float MPU6500::getAccelX() const { return _data.accelX; }
float MPU6500::getAccelY() const { return _data.accelY; }
float MPU6500::getAccelZ() const { return _data.accelZ; }

float MPU6500::getGyroX() const { return _data.gyroX; }
float MPU6500::getGyroY() const { return _data.gyroY; }
float MPU6500::getGyroZ() const { return _data.gyroZ; }

float MPU6500::getTemperature() const { return _data.temperature; }

float MPU6500::getPitch() const { return _data.pitch; }
float MPU6500::getRoll() const { return _data.roll; }
float MPU6500::getYaw() const { return _data.yaw; }

float MPU6500::getAccelMagnitude() const { return _data.accelMagnitude; }
float MPU6500::getPlayerLoad() const { return _data.playerLoad; }
float MPU6500::getMaxImpact() const { return _data.maxImpact; }
uint32_t MPU6500::getSprintCount() const { return _data.sprintCount; }

const MPU6500::SensorData& MPU6500::getData() const {
    return _data;
}
