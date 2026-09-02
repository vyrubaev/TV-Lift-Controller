#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include "Logger/Logger.h"

// Версия текущей прошивки
#define CURRENT_FIRMWARE_VERSION "1.0.2"

class OtaUpdater {
public:
    // По умолчанию URL ведет на ваш Express-сервер
    OtaUpdater(const char* checkUrl = "http://192.168.88.33:3000/firmware/version.json", uint32_t checkIntervalMs = 1800000);
    
    void init();
    void update();     // Вызывается в Core::loop()
    void forceCheck(); // Для принудительного вызова из WebManager

private:
    const char* m_checkUrl;
    uint32_t m_checkIntervalMs;
    uint32_t m_lastCheckMs = 0;

    void checkForUpdates();
    void performOTA(const char* binUrl);
    bool isNewerVersion(const char* serverVersion);
};