#include "BenchTest.h"

#include "../Logger/Logger.h"


void BenchTest::init()
{
    m_inputs.init();

    Logger::info("Input test initialized");
}


void BenchTest::run()
{
    m_inputs.update();


    if (m_inputs.forwardTriggered())
    {
        Logger::debug("FORWARD triggered");
    }


    if (m_inputs.reverseTriggered())
    {
        Logger::debug("REVERSE triggered");
    }


    if (m_inputs.stopTriggered())
    {
        Logger::debug("STOP triggered");
    }


    if (m_inputs.forwardLimit())
    {
        Logger::debug("FORWARD LIMIT active");
    }


    if (m_inputs.reverseLimit())
    {
        Logger::debug("REVERSE LIMIT active");
    }
} 