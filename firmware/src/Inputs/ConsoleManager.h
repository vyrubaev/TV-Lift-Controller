#pragma once

#include <Arduino.h>
#include "Elevator/Elevator.h" // Путь к твоему классу лифта

class ConsoleManager {
public:
    ConsoleManager();
    void init(Elevator* elevatorPtr);
    void update(); // Вызывается в главном loop()

private:
    Elevator* m_elevator;
    String m_inputBuffer;

    void processCommand(const String& cmd);
    void printHelp();
};