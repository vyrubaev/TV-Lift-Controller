#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include "Logger/Logger.h"
#include "Config/DeviceConfig.h"
#include "Elevator/Elevator.h"


extern bool g_isUpdating; // Флаг, указывающий, что идет процесс OTA обновления (для блокировки команд лифта)


class OtaUpdater {
public:
    // Сохраняем вашу оригинальную сигнатуру конструктора с дефолтным URL
    OtaUpdater(const char* checkUrl = DeviceConfig::otaUrl, uint32_t checkIntervalMs = DeviceConfig::otaUpdateIntervalMs);
    
    // Перегружаем init, чтобы опционально принимать указатель на лифт (обратная совместимость сохранена)
    void init(Elevator* elevatorPtr = nullptr);
    
    void update();     // Вызывается в Core::loop()
    void forceCheck(); // Для принудительного вызова из WebManager
    static bool isUpdating(); // Метод для проверки состояния OTA
    
private:
    const char* m_checkUrl;
    uint32_t m_checkIntervalMs;
    uint32_t m_lastCheckMs = 0;
    Elevator* m_elevator = nullptr; // Указатель на лифт

    void checkForUpdates();
    void performOTA(const char* binUrl);
    bool isNewerVersion(const char* serverVersion);
};