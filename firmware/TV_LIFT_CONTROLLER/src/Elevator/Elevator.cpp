#include "Elevator.h"

#include "../Logger/Logger.h"


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
    // Аварийный STOP
    // -------------------------------------------------

    if (m_input.stopTriggered())
    {
        Logger::info("Elevator STOP");

        m_motor.stop();

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
        Logger::warning(
            "FORWARD limit reached"
        );

        m_motor.stop();

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
        Logger::warning(
            "REVERSE limit reached"
        );

        m_motor.stop();

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