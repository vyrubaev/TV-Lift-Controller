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
                snprintf(logBuf, sizeof(logBuf), "OTA: Актуальная версия: %s", DeviceConfig::VERSION);
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
    int s_major = 0, s_minor = 0, s_patch = 0;
    int c_major = 0, c_minor = 0, c_patch = 0;

    // Парсим серверную версию
    sscanf(serverVersion, "%d.%d.%d", &s_major, &s_minor, &s_patch);
    // Парсим текущую локальную версию из DeviceConfig
    sscanf(DeviceConfig::VERSION, "%d.%d.%d", &c_major, &c_minor, &c_patch);

    // Сравниваем покомпонентно
    if (s_major != c_major) return s_major > c_major;
    if (s_minor != c_minor) return s_minor > c_minor;
    return s_patch > c_patch;
}

void OtaUpdater::performOTA(const char* binUrl) {
    WiFiClient client;
    
    snprintf(logBuf, sizeof(logBuf), "OTA: Начинаю загрузку с %s", binUrl);
    Logger::info(logBuf);

    t_httpUpdate_return ret = httpUpdate.update(client, binUrl);

    // Принудительно выводим текст ошибки ДО свича, чтобы она точно попала в лог
    if (ret != HTTP_UPDATE_OK) {
        snprintf(logBuf, sizeof(logBuf), "OTA ОШИБКА КОД: %d | Текст: %s", 
                 httpUpdate.getLastError(), 
                 httpUpdate.getLastErrorString().c_str());
        Logger::error(logBuf);
    }

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Logger::error("OTA: Обновление провалено!");
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Logger::info("OTA: Нет обновлений (сервер вернул тот же бинарник)");
            break;

        case HTTP_UPDATE_OK:
            Logger::info("OTA: Успешно обновлено! Перезагрузка...");
            break;
    }
}