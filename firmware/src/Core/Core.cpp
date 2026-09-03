#include "Core.h"
#include "Logger/Logger.h"
#include "Elevator/Elevator.h" 
#include "Network/WebManager.h"
#include "OTA/OtaUpdater.h"
#include "Config/DeviceConfig.h" 

Elevator elevator;
WebManager webManager;
OtaUpdater otaUpdater;

// Состояние отложенной перезагрузки
static bool s_rebootRequested = false;
static unsigned long s_rebootStartMs = 0; 

bool Core::init()
{
    Logger::init();
    
    Logger::info("Core initialization");

    printSystemInfo(); 

    elevator.init();

    // Инициализируем веб-сервер (поднимет Wi-Fi, mDNS и сокеты)
    Logger::info("WebManager initialization...");
    webManager.init(&elevator);
    otaUpdater.init();

    return true;
}

void Core::loop()
{ 
    elevator.update();

    // Обязательно вызываем update() веб-сервера и OTA
    webManager.update();
    otaUpdater.update();

    // Безопасная обработка отложенной перезагрузки:
    // Ждем 1000 мс с вычислением разности (millis() - start), 
    // чтобы AsyncWebServer успел отправить HTTP-ответ 200 OK 
    // и закрыть сокеты. Защищено от переполнения uint32_t.
    if (s_rebootRequested && (millis() - s_rebootStartMs >= 1000)) {
        Logger::info("Executing scheduled system reboot now...");
        ESP.restart();
    }
}

/**
 * Мягкий запрос на перезагрузку системы.
 * Фиксированная задержка перед перезагрузкой — 1000 мс.
 */
void Core::reboot()
{
    Logger::info("System reboot requested...");
    s_rebootStartMs = millis();
    s_rebootRequested = true;
}

// РЕАЛИЗАЦИЯ КАК МЕТОДА КЛАССА
void Core::printSystemInfo() {
    char buffer[96];
    snprintf(buffer, sizeof(buffer), "[SYSTEM] Firmware Version: %s (Build: %s %s)", 
                DeviceConfig::VERSION, __DATE__, __TIME__);
    Logger::info(buffer);
}