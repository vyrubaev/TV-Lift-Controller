#include "Core.h"
#include "Logger/Logger.h"
#include "Elevator/Elevator.h" 
#include "Network/WebManager.h"
#include "OTA/OtaUpdater.h"
#include "Config/DeviceConfig.h" // Убедись, что подключен конфиг с VERSION

Elevator elevator;
WebManager webManager;
OtaUpdater otaUpdater;

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

    // Обязательно вызываем update() веб-сервера
    webManager.update();
    otaUpdater.update();
}

void Core::reboot()
{
    Logger::info("System rebooting...");
    ESP.restart();
}

// РЕАЛИЗАЦИЯ КАК МЕТОДА КЛАССА (убран static, добавлено Core::)
void Core::printSystemInfo() {
    char buffer[96];
    snprintf(buffer, sizeof(buffer), "[SYSTEM] Firmware Version: %s (Build: %s %s)", 
                DeviceConfig::VERSION, __DATE__, __TIME__);
    Logger::info(buffer);
}