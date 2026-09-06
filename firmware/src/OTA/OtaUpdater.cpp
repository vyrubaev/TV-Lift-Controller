#include "OtaUpdater.h"



static char logBuf[128];
// Статическая переменная состояния
static bool s_inOtaProcess = false;

OtaUpdater::OtaUpdater(const char* checkUrl, uint32_t checkIntervalMs)
    : m_checkUrl(checkUrl) {}


void OtaUpdater::init(Elevator* elevatorPtr) {
    m_elevator = elevatorPtr;
    Logger::info("OTA: Инициализация сервиса обновлений");
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
    return g_isUpdating; // или s_inOtaProcess;
}

void OtaUpdater::checkForUpdates() {
    if (WiFi.status() != WL_CONNECTED) {
        Logger::warning("OTA: WiFi не подключен, проверка отменена");
        return;
    }

    // Жесткая проверка: если лифт вообще существует и движется — сразу выход
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

    // Финальная проверка перед самой записью во флеш
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
    g_isUpdating = true; // Устанавливаем флаг блокировки перед началом

    WiFiClient client;
    client.setTimeout(60); 
    
    snprintf(logBuf, sizeof(logBuf), "OTA: Начинаю загрузку с %s", binUrl);
    Logger::info(logBuf);

    // Вызываем обновлятор ровно один раз
    t_httpUpdate_return ret = httpUpdate.update(client, binUrl);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            snprintf(logBuf, sizeof(logBuf), "OTA ОШИБКА КОД: %d | Текст: %s", 
                     httpUpdate.getLastError(), 
                     httpUpdate.getLastErrorString().c_str());
            Logger::error(logBuf);
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Logger::info("OTA: Нет обновлений");
            break;

        case HTTP_UPDATE_OK:
            Logger::info("OTA: Успешно обновлено! Перезагружаю систему...");
            delay(200); // Даем время логеру вытолкнуть данные в Serial
            ESP.restart(); // Жесткий системный рестарт ESP32
            break;
    }
    g_isUpdating = false; // Снимаем флаг, если обновление не удалось
}