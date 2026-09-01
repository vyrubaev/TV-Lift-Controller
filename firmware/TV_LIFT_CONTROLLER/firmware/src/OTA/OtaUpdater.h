#pragma once

#include <Arduino.h>
#include <WiFi.h>            // <--- Добавили, чтобы решить ошибки 1 и 3 (WiFi и WiFiClient)
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include "Logger/Logger.h"

// Версия текущей прошивки
#define CURRENT_FIRMWARE_VERSION "1.0.0"

class OtaUpdater {
public:
    OtaUpdater(const char* checkUrl, uint32_t checkIntervalMs = 3600000); // По умолчанию опрос раз в 1 час
    
    void update(); // Вызывать в loop()
    void forceCheck(); // Принудительный запуск проверки

private:
    const char* m_checkUrl;
    uint32_t m_checkIntervalMs;
    uint32_t m_lastCheckMs = 0;

    void checkForUpdates();
    void performOTA(const char* binUrl);
    bool isNewerVersion(const char* serverVersion);
};