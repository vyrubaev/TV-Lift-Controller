#pragma once

#include "CoreState.h"

class Core
{
public:

    Core();

    bool init();

    void loop();

    void reboot();

private:

    CoreState m_state;

private:

    Module* m_modules[10];

    int m_moduleCount;    
};