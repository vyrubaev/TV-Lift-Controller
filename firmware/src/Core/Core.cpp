#include "Core.h"
#include "Logger/Logger.h"
#include "Elevator/Elevator.h" 
#include "Network/WebManager.h"
#include "OTA/OtaUpdater.h"
#include "Config/DeviceConfig.h" 
#include "CLI/ConsoleManager.h"

Elevator elevator;
WebManager webManager;
OtaUpdater otaUpdater;
ConsoleManager consoleManager; // Объект командной строки

// Состояние отложенной перезагрузки
static bool s_rebootRequested = false;
static unsigned long s_rebootStartMs = 0; 

// Задача FreeRTOS для безопасного и непрерывного управления лифтом
void elevatorTask(void* pvParameters) {
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(10); // Опрос каждые 10 мс (100 Гц)

    xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // Критическая логика работает автономно от сети
        elevator.update();
        
        // Строгая задержка до следующего такта
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

bool Core::init()
{
    Logger::init();
    Logger::info("Core initialization");

    // 1. Загружаем конфигурацию
    DeviceConfig::load();
    printSystemInfo();

    elevator.init();

    // Инициализация командной строки и привязка к объектам
    consoleManager.init(&elevator); 
    Logger::info("CLI Manager initialized. Type commands in Serial Monitor.");

    // Инициализируем веб-сервер и OTA
    webManager.init(&elevator);
    Logger::info("WebManager initialization...");
    webManager.init(&elevator);
    Logger::info("OTA initialization...");
    otaUpdater.init(&elevator);

    // 2. Создаем независимую высокоприоритетную задачу для лифта на ядре 1
    xTaskCreatePinnedToCore(
        elevatorTask,   // Функция задачи
        "ElevatorTask", // Имя задачи для отладки
        4096,           // Размер стека в байтах
        NULL,           // Параметр
        3,              // Высокий приоритет (выше сетевых задач)
        NULL,           // Дескриптор задачи
        1               // Ядро (0 или 1)
    );
    Logger::info("Elevator task created on Core 1");

    return true;
}

void Core::loop()
{ 
    // Больше не вызываем здесь elevator.update(), лифт под защитой FreeRTOS!

    // Сетевой стек и веб-интерфейс крутятся в штатном режиме и теперь 
    // не смогут заблокировать физическую безопасность лифта.
    webManager.update();
    otaUpdater.update();

    if (s_rebootRequested && (millis() - s_rebootStartMs >= 1000)) {
        Logger::info("Executing scheduled system reboot now...");
        ESP.restart();
    }
}

void Core::reboot()
{
    Logger::info("System reboot requested...");
    s_rebootStartMs = millis();
    s_rebootRequested = true;
}

void Core::printSystemInfo() {
    char buffer[96];
    snprintf(buffer, sizeof(buffer), "[SYSTEM] Firmware Version: %s (Build: %s %s)", 
                DeviceConfig::VERSION, __DATE__, __TIME__);
    Logger::info(buffer);
}