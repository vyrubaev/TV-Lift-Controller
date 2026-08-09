#pragma once

#include "Inputs/InputManager.h"

class BenchTest
{
public:

    void init();
    void run();

private:

    InputManager m_inputs;
};