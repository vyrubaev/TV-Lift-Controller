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

    Motor m_motor;
    InputManager m_input;
};