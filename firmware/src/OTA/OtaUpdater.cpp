#include "Logger/Logger.h"
#include "OtaUpdater.h"
#include <WiFiClientSecure.h> //dvddv
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>

#include "Config/BoardConfig.h" // Обязательно для прерываний пинов!

static char logBuf[128];

OtaUpdater::OtaUpdater(const char* checkUrl, uint32_t checkIntervalMs)
    : m_checkUrl(checkUrl) {}

void OtaUpdater::init(Elevator* elevatorPtr) {
    m_elevator = elevatorPtr;
    Logger::info("OTA: Initializing update service");
}

void OtaUpdater::update() {
    if (WiFi.status() == WL_CONNECTED && (millis() - m_lastCheckMs >= DeviceConfig::otaUpdateIntervalMs)) {
        m_lastCheckMs = millis();
        checkForUpdates();
    }
}

void OtaUpdater::forceCheck() {
    checkForUpdates();
}

bool OtaUpdater::isUpdating() {
    return g_isUpdating;
}

void OtaUpdater::checkForUpdates() {
    if (WiFi.status() != WL_CONNECTED) {
        Logger::warning("OTA: WiFi не подключен, проверка отменена");
        return;
    }

    // Защита: если лифт движется — откладываем
    if (m_elevator && m_elevator->isMoving()) {
        Logger::warning("OTA: Лифт в движении. Проверка обновлений отложена.");
        return;
    }

    String targetBinUrl = "";
    bool needUpdate = false;

    {
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
                    targetBinUrl = String(binUrl); 
                    needUpdate = true;
                }
            }
        }
        http.end(); 
    }

    if (needUpdate && targetBinUrl.length() > 0) {
        if (m_elevator && m_elevator->isMoving()) {
            Logger::error("OTA ОТМЕНЕНА: Лифт начал движение перед загрузкой!");
            return;
        }
        performOTA(targetBinUrl.c_str());
    }
}

bool OtaUpdater::isNewerVersion(const char* serverVersion) {
    int s_major = 0, s_minor = 0, s_patch = 0;
    int c_major = 0, c_minor = 0, c_patch = 0;

    sscanf(serverVersion, "%d.%d.%d", &s_major, &s_minor, &s_patch);
    sscanf(DeviceConfig::VERSION, "%d.%d.%d", &c_major, &c_minor, &c_patch);

    if (s_major != c_major) return s_major > c_major;
    if (s_minor != c_minor) return s_minor > c_minor;
    return s_patch > c_patch;
}

void OtaUpdater::performOTA(const char* binUrl) {
    if (m_elevator && m_elevator->isMoving()) {
        Logger::warning("OTA: Отменено, лифт всё ещё находится в движении!");
        return;
    }

    g_isUpdating = true; // Блокируем логику работы

    // Отключаем обработчики прерываний, чтобы избежать Guru Meditation Error при перезаписи Flash
    detachInterrupt(digitalPinToInterrupt(BoardConfig::ENC_A));
    detachInterrupt(digitalPinToInterrupt(BoardConfig::ENC_B));
    detachInterrupt(digitalPinToInterrupt(BoardConfig::MOTOR1_DIAG));

    snprintf(logBuf, sizeof(logBuf), "OTA: Начинаю загрузку с %s", binUrl);
    Logger::info(logBuf);
    delay(100);

    httpUpdate.rebootOnUpdate(false);
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    t_httpUpdate_return ret;

    if (strncmp(binUrl, "https", 5) == 0) {
        WiFiClientSecure clientSecure;
        clientSecure.setInsecure(); // Экономия RAM при SSL
        ret = httpUpdate.update(clientSecure, binUrl);
    } else {
        WiFiClient client;
        client.setTimeout(30);
        ret = httpUpdate.update(client, binUrl);
    }

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            snprintf(logBuf, sizeof(logBuf), "OTA ОШИБКА КОД: %d | Текст: %s", 
                     httpUpdate.getLastError(), 
                     httpUpdate.getLastErrorString().c_str());
            Logger::error(logBuf);
            
            // Восстановление через перезагрузку
            delay(1000);
            ESP.restart(); 
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Logger::info("OTA: Нет обновлений");
            break;

        case HTTP_UPDATE_OK:
            Logger::info("OTA: Успешно обновлено! Перезагрузка...");
            delay(500); 
            ESP.restart();
            break;
    }

    g_isUpdating = false;
}