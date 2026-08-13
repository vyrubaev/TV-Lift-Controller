#pragma once

#include "../Drivers/Motor.h"
#include "../Inputs/InputManager.h"

class Elevator
{
public:

    Elevator();

    void init();
    void update();

private:
        // Какой концевик запустил добег двигателя.
    enum class LimitRunOnDirection : uint8_t
    {
        NONE,
        FORWARD,
        REVERSE
    };

    // Состояние текущего добега.
    LimitRunOnDirection m_limitRunOnDirection =
        LimitRunOnDirection::NONE;

    // Значение millis() в момент срабатывания концевика.
    uint32_t m_limitRunOnStartMs = 0;

    Motor m_motor;
    InputManager m_input;
};