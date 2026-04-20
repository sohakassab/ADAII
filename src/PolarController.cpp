#include "PolarController.h"
#include <math.h>

// Standard Bluetooth GATT Heart Rate Service & Characteristic
static BLEUUID serviceUUID("0000180d-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID   ("00002a37-0000-1000-8000-00805f9b34fb");

// Static members initialization
BLEAdvertisedDevice* PolarController::_targetDevice = nullptr;
BLEClient* PolarController::_pClient       = nullptr;
bool                 PolarController::_doConnect     = false;
PolarController* PolarController::_instance      = nullptr;

PolarController::PolarController() : _ppiHead(0), _ppiCount(0), _lastScanAttempt(0) {
    _instance = this;
    _clearData();
}

void PolarController::begin() {
    Serial.println("PolarController: Initializing BLE...");
    BLEDevice::init("");
    _startScan();
}

void PolarController::update() {
    // 1. Connection Logic
    if (_doConnect) {
        _doConnect = false;
        
        // CRITICAL: Stop scanning before attempting to connect
        BLEDevice::getScan()->stop();
        
        // CRITICAL: Give the Bluetooth radio time to release hardware
        delay(200); 
        
        if (_connectToServer()) {
            Serial.println("PolarController: Connection Successful.");
            _data.isConnected = true;
        } else {
            Serial.println("PolarController: Connection Failed.");
            if (_targetDevice) { delete _targetDevice; _targetDevice = nullptr; }
            _lastScanAttempt = millis(); // Cooldown before rescanning
        }
    }

    // 2. Health Check
    if (_data.isConnected && _pClient != nullptr) {
        if (!_pClient->isConnected()) {
            Serial.println("PolarController: Connection lost detected in update loop.");
            _data.isConnected = false; 
            // Client cleanup is handled by onDisconnect callback
        }
    }

    // 3. Auto-Reconnect logic
    if (!_data.isConnected && !_doConnect) {
        unsigned long now = millis();
        if (now - _lastScanAttempt > 10000) { // Try scanning every 10 seconds
            _lastScanAttempt = now;
            _startScan();
        }
    }
}

void PolarController::_startScan() {
    Serial.println("PolarController: Starting Scan...");
    BLEScan* pScan = BLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new DeviceCallbacks(), false);
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);
    pScan->start(5, false); // Scan for 5 seconds
}

bool PolarController::_connectToServer() {
    if (!_targetDevice) return false;

    Serial.print("PolarController: Connecting to ");
    Serial.println(_targetDevice->getAddress().toString().c_str());

    // CRITICAL: Prevent memory leaks / slot exhaustion by reusing the client
    if (_pClient == nullptr) {
        _pClient = BLEDevice::createClient();
        _pClient->setClientCallbacks(new ClientCallbacks());
    }

    // Force clear any zombie states before connecting
    if (_pClient->isConnected()) {
        _pClient->disconnect();
        delay(100);
    }

    // Connect to the remote BLE Server
    if (!_pClient->connect(_targetDevice)) {
        return false;
    }

    BLERemoteService* pService = _pClient->getService(serviceUUID);
    if (!pService) {
        _pClient->disconnect();
        return false;
    }

    BLERemoteCharacteristic* pChar = pService->getCharacteristic(charUUID);
    if (!pChar) {
        _pClient->disconnect();
        return false;
    }

    if (pChar->canNotify()) {
        pChar->registerForNotify(_notifyCallback);
    }

    return true;
}

void PolarController::_clearData() {
    _data.heartRate       = 0;
    _data.ppi             = 0;
    _data.hrv             = 0.0f;
    _data.respiratoryRate = 0.0f;
    _data.isConnected     = false;
    _data.isUpdated       = false;
    memset(_ppiBuffer, 0, sizeof(_ppiBuffer));
    _ppiHead  = 0;
    _ppiCount = 0;
}

const PolarController::BiometricData& PolarController::getData() const {
    return _data;
}

bool PolarController::isConnected() const {
    return _data.isConnected;
}

void PolarController::_calcHRV() {
    if (_ppiCount < 2) return;
    float sumSqDiff = 0.0f;
    uint8_t count = 0;
    for (uint8_t i = 1; i < _ppiCount; i++) {
        uint8_t curr = (_ppiHead - i + PPI_BUFFER_SIZE) % PPI_BUFFER_SIZE;
        uint8_t prev = (_ppiHead - i - 1 + PPI_BUFFER_SIZE) % PPI_BUFFER_SIZE;
        float diff = (float)_ppiBuffer[curr] - (float)_ppiBuffer[prev];
        sumSqDiff += diff * diff;
        count++;
    }
    if (count > 0) _data.hrv = sqrt(sumSqDiff / count);
}

void PolarController::_calcRespiratoryRate() {
    if (_ppiCount < 4 || _data.hrv <= 0) return;
    float sumPpi = 0.0f;
    for (uint8_t i = 0; i < _ppiCount; i++) sumPpi += _ppiBuffer[i];
    float avgPpi = sumPpi / _ppiCount;
    if (avgPpi > 0) {
        float rrBpm = (_data.hrv / avgPpi) * 120.0f;
        _data.respiratoryRate = constrain(rrBpm, 8.0f, 25.0f);
    }
}

void PolarController::_notifyCallback(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
    if (!_instance || length < 2) return;

    _instance->_data.heartRate = pData[1];
    for (uint8_t i = 2; i + 1 < length; i += 2) {
        uint16_t ppi = pData[i] + (pData[i + 1] << 8);
        _instance->_ppiBuffer[_instance->_ppiHead] = ppi;
        _instance->_ppiHead = (_instance->_ppiHead + 1) % PPI_BUFFER_SIZE;
        if (_instance->_ppiCount < PPI_BUFFER_SIZE) _instance->_ppiCount++;
        _instance->_data.ppi = ppi;
    }
    _instance->_calcHRV();
    _instance->_calcRespiratoryRate();
    _instance->_data.isUpdated = true;
}

void PolarController::DeviceCallbacks::onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
        BLEDevice::getScan()->stop();
        if (PolarController::_targetDevice) delete PolarController::_targetDevice;
        PolarController::_targetDevice = new BLEAdvertisedDevice(advertisedDevice);
        PolarController::_doConnect = true;
    }
}

void PolarController::ClientCallbacks::onDisconnect(BLEClient* pclient) {
    if (PolarController::_instance) {
        PolarController::_instance->_clearData();
        PolarController::_instance->_lastScanAttempt = millis();
    }
    // CRITICAL: We NO LONGER set _pClient to nullptr here so we can reuse it.
    Serial.println("PolarController: Disconnected from device.");
}