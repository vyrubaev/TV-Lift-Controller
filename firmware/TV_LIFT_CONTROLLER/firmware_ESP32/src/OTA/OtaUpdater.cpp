#include "OtaUpdater.h"

static char logBuf[128];

OtaUpdater::OtaUpdater(const char* checkUrl, uint32_t checkIntervalMs)
    : m_checkUrl(checkUrl), m_checkIntervalMs(checkIntervalMs) {}

void OtaUpdater::init() {
    Logger::info("OTA: Инициализация сервиса обновлений");
}

void OtaUpdater::update() {
    if (WiFi.status() == WL_CONNECTED && (millis() - m_lastCheckMs >= m_checkIntervalMs)) {
        m_lastCheckMs = millis();
        checkForUpdates();
    }
}

void OtaUpdater::forceCheck() {
    checkForUpdates();
}

void OtaUpdater::checkForUpdates() {
    if (WiFi.status() != WL_CONNECTED) {
        Logger::warning("OTA: WiFi не подключен, проверка отменена");
        return;
    }

    HTTPClient http;
    http.begin(m_checkUrl);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, http.getString());

        if (!error) {
            const char* serverVersion = doc["version"];
            const char* binUrl = doc["url"];

            if (serverVersion && binUrl && isNewerVersion(serverVersion)) {
                snprintf(logBuf, sizeof(logBuf), "OTA: Найдено обновление: %s", serverVersion);
                Logger::info(logBuf);
                performOTA(binUrl);
            } else {
                snprintf(logBuf, sizeof(logBuf), "OTA: Актуальная версия: %s", CURRENT_FIRMWARE_VERSION);
                Logger::info(logBuf);
            }
        } else {
            snprintf(logBuf, sizeof(logBuf), "OTA: Ошибка парсинга JSON: %s", error.c_str());
            Logger::error(logBuf);
        }
    } else {
        snprintf(logBuf, sizeof(logBuf), "OTA: Ошибка запроса манифеста: %d", httpCode);
        Logger::error(logBuf);
    }
    http.end();
}

bool OtaUpdater::isNewerVersion(const char* serverVersion) {
    return strcmp(serverVersion, CURRENT_FIRMWARE_VERSION) != 0;
}

void OtaUpdater::performOTA(const char* binUrl) {
    WiFiClient client;
    
    // Включение авто-перезагрузки ESP32 после успешной прошивки
    httpUpdate.rebootOnUpdate(true);

    t_httpUpdate_return ret = httpUpdate.update(client, binUrl);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            snprintf(logBuf, sizeof(logBuf), "OTA: Ошибка обновления (%d): %s", 
                     httpUpdate.getLastError(), 
                     httpUpdate.getLastErrorString().c_str());
            Logger::error(logBuf);
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Logger::info("OTA: Нет обновлений");
            break;

        case HTTP_UPDATE_OK:
            Logger::info("OTA: Успешно обновлено! Перезагрузка...");
            break;
    }
}