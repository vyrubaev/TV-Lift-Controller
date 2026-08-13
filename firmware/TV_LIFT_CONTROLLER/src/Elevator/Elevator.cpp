#include "Elevator.h"

#include "../Logger/Logger.h"

#include "../Config/DeviceConfig.h"

Elevator::Elevator()
{
}


void Elevator::init()
{
    m_motor.init();
    m_input.init();

    Logger::info("Elevator initialized");
}


void Elevator::update()
{
    m_input.update();


    // -------------------------------------------------
    //  STOP
    // -------------------------------------------------

    if (m_input.stopTriggered())
    {
        Logger::info("Elevator STOP");

        m_limitRunOnDirection =
            LimitRunOnDirection::NONE; // Команда STOP отменяет любой начатый добег.

        m_motor.stop();

        return;
    }

    // -------------------------------------------------
    // Добег после концевика
    // -------------------------------------------------

    if (m_limitRunOnDirection != LimitRunOnDirection::NONE)
    {
        const uint32_t now = millis();

        uint32_t runOnTimeMs = 0;

        if (
            m_limitRunOnDirection ==
            LimitRunOnDirection::FORWARD
        )
            {
                runOnTimeMs =
                    DeviceConfig::FORWARD_LIMIT_RUN_ON_MS;
            }
        else
            {
                runOnTimeMs =
                    DeviceConfig::REVERSE_LIMIT_RUN_ON_MS;
            }

        if (now - m_limitRunOnStartMs >= runOnTimeMs)
        {
            Logger::info("Limit run-on completed");

            m_motor.stop();

            m_limitRunOnDirection =
                LimitRunOnDirection::NONE;
        }

        // Пока выполняется добег, новые команды не принимаем.
        return;
    }

    // -------------------------------------------------
    // FORWARD LIMIT
    // -------------------------------------------------

    if (
        m_input.forwardLimit() &&
        m_motor.getState() == MotorState::FORWARD
    )
    {
        if (DeviceConfig::FORWARD_LIMIT_RUN_ON_MS == 0)
        {
            Logger::warning(
                "FORWARD limit reached; motor stopped"
            );

            m_motor.stop();

            return;
        }

        Logger::warning(
        "FORWARD limit reached; run-on started"
    );
    
    
    m_limitRunOnDirection =
        LimitRunOnDirection::FORWARD;

        m_limitRunOnStartMs = millis();

        return;
    }


    // -------------------------------------------------
    // REVERSE LIMIT
    // -------------------------------------------------

    if (
        m_input.reverseLimit() &&
        m_motor.getState() == MotorState::REVERSE
    )
    {
        if (DeviceConfig::REVERSE_LIMIT_RUN_ON_MS == 0)
        {
            Logger::warning(
                "REVERSE limit reached; motor stopped"
            );

            m_motor.stop();

            return;
        }
        
        Logger::warning(
        "REVERSE limit reached; run-on started"
    );

    m_limitRunOnDirection =
        LimitRunOnDirection::REVERSE;

    m_limitRunOnStartMs = millis();

        return;
    }


    // -------------------------------------------------
    // FORWARD command
    // -------------------------------------------------

    if (m_input.forwardTriggered())
    {
        if (m_input.forwardLimit())
        {
            Logger::warning(
                "FORWARD blocked by limit switch"
            );

            return;
        }

        Logger::info("Elevator FORWARD");

        m_motor.forward();

        return;
    }


    // -------------------------------------------------
    // REVERSE command
    // -------------------------------------------------

    if (m_input.reverseTriggered())
    {
        if (m_input.reverseLimit())
        {
            Logger::warning(
                "REVERSE blocked by limit switch"
            );

            return;
        }

        Logger::info("Elevator REVERSE");

        m_motor.reverse();

        return;
    }
}