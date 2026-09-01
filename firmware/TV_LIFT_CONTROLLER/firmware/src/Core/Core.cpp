#include "Core.h"

#include "Logger/Logger.h"
#include "Elevator/Elevator.h" 
#include "Network/WebManager.h"

Elevator elevator;
WebManager webManager;

bool Core::init()
{
    Logger::init();
    Logger::info("Core initialization");

    elevator.init();

    // Инициализируем веб-сервер (поднимет Wi-Fi, mDNS и сокеты)
    Logger::info("WebManager initialization...");
    webManager.init(&elevator);

    return true;
}

void Core::loop()
{ 
    elevator.update();

    // 4. Обязательно вызываем update() веб-сервера
    // Это нужно для обработки DNS (в режиме настройки) и очистки WebSocket
    webManager.update();

   
}


void Core::reboot()
{
    Logger::info("System rebooting...");
    ESP.restart(); // Встроенная функция сброса ESP32
}