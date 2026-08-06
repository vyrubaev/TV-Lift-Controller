#include "Core.h"
#include "Module.h"

Core::Core()
{
    m_state = CoreState::Boot;
}

bool Core::init()
{
    m_state = CoreState::Initializing;

    // Здесь позже будем запускать все модули

    m_state = CoreState::Running;

    return true;
}

void Core::loop()
{
    switch (m_state)
    {
        case CoreState::Boot:
            handleBoot();
            break;

        case CoreState::Initializing:
            handleInitializing();
            break;

        case CoreState::Running:
            handleRunning();
            break;

        case CoreState::Rebooting:
            handleRebooting();
            break;

        case CoreState::FatalError:
            handleFatalError();
            break;
    }
}

void Core::reboot()
{
    m_state = CoreState::Rebooting;
}