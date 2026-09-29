#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "Config/DeviceConfig.h"
#include "Elevator/Elevator.h"

extern bool g_isUpdating; // Флаг процесса OTA

class OtaUpdater {
public:
    OtaUpdater(const char* checkUrl = DeviceConfig::otaUrl, uint32_t checkIntervalMs = DeviceConfig::otaUpdateIntervalMs);
    
    void init(Elevator* elevatorPtr = nullptr);
    void update();     
    void forceCheck(); 
    static bool isUpdating(); 
    
private:
    const char* m_checkUrl;
    uint32_t m_lastCheckMs = 0;
    Elevator* m_elevator = nullptr;

    void checkForUpdates();
    void performOTA(const char* binUrl);
    bool isNewerVersion(const char* serverVersion);
};