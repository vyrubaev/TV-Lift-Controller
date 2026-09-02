#include "Core.h"

#include "Logger/Logger.h"
#include "Elevator/Elevator.h" 
#include "Network/WebManager.h"
#include "OTA/OtaUpdater.h"

Elevator elevator;
WebManager webManager;
OtaUpdater otaUpdater;

// Таймер для фоновой проверки обновлений (например, раз в 30 минут)
unsigned long lastOtaCheck = 0;
const unsigned long OTA_CHECK_INTERVAL = 30 * 60 * 1000; // 30 минут в мс

bool Core::init()
{
    Logger::init();
    Logger::info("Core initialization");

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

    // 4. Обязательно вызываем update() веб-сервера
    // Это нужно для обработки DNS (в режиме настройки) и очистки WebSocket
    webManager.update();
    otaUpdater.update();
    

   
}


void Core::reboot()
{
    Logger::info("System rebooting...");
    ESP.restart(); // Встроенная функция сброса ESP32
}