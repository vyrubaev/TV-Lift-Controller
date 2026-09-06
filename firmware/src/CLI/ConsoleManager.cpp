#include "CLI/ConsoleManager.h"
#include "Core/Core.h"
#include "Logger/Logger.h"

ConsoleManager::ConsoleManager() {
    m_elevator = nullptr;
    m_inputBuffer = "";
}

void ConsoleManager::init(Elevator* elevatorPtr) {
    m_elevator = elevatorPtr;
    Logger::info("[CLI] Консоль управления лифтом запущенна.");
    printHelp();
}

void ConsoleManager::update() {
    // Читаем Serial посимвольно в неблокирующем режиме
    while (Serial.available() > 0) {
        char incomingChar = (char)Serial.read();

        // Игнорируем возврат каретки, ждем перевод строки (Enter)
        if (incomingChar == '\r') continue;

        if (incomingChar == '\n') {
            m_inputBuffer.trim(); // Убираем лишние пробелы/переводы
            if (m_inputBuffer.length() > 0) {
                processCommand(m_inputBuffer);
            }
            m_inputBuffer = ""; // Очищаем буфер для следующей команды
        } else {
            m_inputBuffer += incomingChar;
            Serial.print(incomingChar); // <--- ЭХО: выводим символ на экран при вводе
            // Защита от переполнения буфера бесконечным вводным мусором
            if (m_inputBuffer.length() > 64) {
                m_inputBuffer = "";
                Logger::warning("[CLI] Ошибка: Слишком длинная команда!");
            }
        }
    }
}

void ConsoleManager::processCommand(const String& cmd) {
    Serial.print("[CLI] > ");
    Serial.println(cmd);

    String upperCmd = cmd;
    upperCmd.toUpperCase();

    if (upperCmd == "HELP" || upperCmd == "?") {
        printHelp();
    } 
    else if (upperCmd == "UP") {
        if (m_elevator) {
            m_elevator->postCliCommand(Elevator::PendingCommand::Type::UP);
        }
    } 
    else if (upperCmd == "DOWN") {
        if (m_elevator) {
            m_elevator->postCliCommand(Elevator::PendingCommand::Type::DOWN);
        }
    } 
    else if (upperCmd == "STOP") {
        if (m_elevator) {
            m_elevator->postCliCommand(Elevator::PendingCommand::Type::STOP);
        }
    } 
    else if (upperCmd == "STATUS" || upperCmd == "STATE") {
        if (m_elevator) {
            Serial.print("[CLI] Текущий статус лифта (код): ");
            Serial.println((int)m_elevator->getState());
            Serial.print("[CLI] Текущий ток: ");
            Serial.print(m_elevator->getCurrentAmps());
            Serial.println(" A");
        }
    } 
    else if (upperCmd == "REBOOT") {
        Core::reboot();
    } 
    else {
        Logger::error("[CLI] Неизвестная команда. Введите HELP для справки.");
    }
}

void ConsoleManager::printHelp() {
    Serial.println("\n--- ДОСТУПНЫЕ КОМАНДЫ КОНСОЛИ ---");
    Serial.println("  UP     - Запустить лифт вверх");
    Serial.println("  DOWN   - Запустить лифт вниз");
    Serial.println("  STOP   - Экстренная остановка");
    Serial.println("  STATUS - Вывести текущее состояние и ток мотора");
    Serial.println("  REBOOT - Перезагрузить устройство");
    Serial.println("  HELP   - Показать эту справку");
    Serial.println("----------------------------------\n");
}