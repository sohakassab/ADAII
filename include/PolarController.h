#ifndef POLAR_CONTROLLER_H
#define POLAR_CONTROLLER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEScan.h>

class PolarController {
public:
    struct BiometricData {
        uint8_t  heartRate;          // BPM
        uint16_t ppi;               // Peak-to-Peak Interval (ms)
        float    hrv;                // RMSSD (ms)
        float    respiratoryRate;    // Derived Breaths per minute
        bool     isConnected;
        bool     isUpdated;          // True when new data arrived
    };

    PolarController();

    void begin();
    void update();
    const BiometricData& getData() const;
    bool isConnected() const;

private:
    BiometricData _data;

    static const uint8_t PPI_BUFFER_SIZE = 32;
    uint16_t _ppiBuffer[PPI_BUFFER_SIZE];
    uint8_t  _ppiHead;
    uint8_t  _ppiCount;
    unsigned long _lastScanAttempt;

    // BLE internals
    static BLEAdvertisedDevice* _targetDevice;
    static BLEClient* _pClient;
    static bool _doConnect;
    static PolarController* _instance;

    bool _connectToServer();
    void _startScan();
    void _clearData();
    void _calcHRV();
    void _calcRespiratoryRate();

    class DeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
        void onResult(BLEAdvertisedDevice advertisedDevice) override;
    };

    class ClientCallbacks : public BLEClientCallbacks {
        void onConnect(BLEClient* pclient) override {}
        void onDisconnect(BLEClient* pclient) override;
    };

    static void _notifyCallback(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify);
};

#endif // POLAR_CONTROLLER_H