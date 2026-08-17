#include "Elevator.h"
#include "../Logger/Logger.h"
#include <cstdio>

Elevator::Elevator()
{
}

void Elevator::init()
{
    if (DeviceConfig::MOUNT_TYPE == MOUNT_CEILING) {
        m_invertMotor = true;
        Logger::info("Elevator initialized: CEILING type");
    } else if (DeviceConfig::MOUNT_TYPE == MOUNT_WALL) {
        m_invertMotor = false;
        Logger::info("Elevator initialized: WALL type");
    } else {
        m_invertMotor = false;
        Logger::info("Elevator initialized: FLOOR type");
    }

    m_motor.init();
    m_input.init();
    m_irReceiver.init();
}

const char* Elevator::sourceToString(CommandSource src) {
    switch (src) {
        case CommandSource::BUTTON: return "BUTTON";
        case CommandSource::IR:     return "IR";
        case CommandSource::CLI:    return "CLI";
        case CommandSource::WEB:    return "WEB";
        case CommandSource::APP:    return "APP";
        default:                    return "UNKNOWN";
    }
}

void Elevator::open(CommandSource src) {
    char logBuffer[64];
    snprintf(logBuffer, sizeof(logBuffer), "Command OPEN received from: %s", sourceToString(src));
    Logger::info(logBuffer);

    if (m_invertMotor) {
        m_motor.reverse();
    } else {
        m_motor.forward();
    }
}

void Elevator::close(CommandSource src) {
    char logBuffer[64];
    snprintf(logBuffer, sizeof(logBuffer), "Command CLOSE received from: %s", sourceToString(src));
    Logger::info(logBuffer);

    if (m_invertMotor) {
        m_motor.forward();
    } else {
        m_motor.reverse();
    }
}

void Elevator::stop(CommandSource src) {
    char logBuffer[64];
    snprintf(logBuffer, sizeof(logBuffer), "Command STOP received from: %s", sourceToString(src));
    Logger::info(logBuffer);

    m_limitRunOnDirection = LimitRunOnDirection::NONE;
    m_motor.stop();
}

void Elevator::update()
{   
    m_input.update();
    m_irReceiver.update();
    m_motor.update();

    // -------------------------------------------------
    // EMERGENCY FAULT
    // -------------------------------------------------
    if (m_motor.isEmergency())
    {
        if (m_motor.getState() != MotorState::EMERGENCY_STOP)
        {
            Logger::error("EMERGENCY FAULT: Motor driver reported error via DIAG pin!");
            m_limitRunOnDirection = LimitRunOnDirection::NONE;
            m_motor.stop();
        }

        if (m_input.forwardTriggered() || m_input.reverseTriggered())
        {
            Logger::warning("Command rejected: Motor driver is in FAULT state!");
        }
        return;
    }

    // -------------------------------------------------
    // STOP BUTTON
    // -------------------------------------------------
    if (m_input.stopTriggered())
    {
        stop(CommandSource::BUTTON);
        return;
    }

    // -------------------------------------------------
    // Добег после концевика
    // -------------------------------------------------
    if (m_limitRunOnDirection != LimitRunOnDirection::NONE)
    {
        const uint32_t now = millis();
        uint32_t runOnTimeMs = (m_limitRunOnDirection == LimitRunOnDirection::FORWARD) 
            ? DeviceConfig::FORWARD_LIMIT_RUN_ON_MS 
            : DeviceConfig::REVERSE_LIMIT_RUN_ON_MS;

        if (now - m_limitRunOnStartMs >= runOnTimeMs)
        {
            Logger::info("Limit run-on completed");
            m_motor.stop();
            m_limitRunOnDirection = LimitRunOnDirection::NONE;
        }

        return;
    }

    // -------------------------------------------------
    // FORWARD LIMIT
    // -------------------------------------------------
    if (m_input.forwardLimit() && m_motor.getState() == MotorState::FORWARD)
    {
        if (DeviceConfig::FORWARD_LIMIT_RUN_ON_MS == 0)
        {
            Logger::warning("FORWARD limit reached; motor stopped");
            m_motor.stop();
            return;
        }

        Logger::warning("FORWARD limit reached; run-on started");
        m_limitRunOnDirection = LimitRunOnDirection::FORWARD;
        m_limitRunOnStartMs = millis();
        return;
    }

    // -------------------------------------------------
    // REVERSE LIMIT
    // -------------------------------------------------
    if (m_input.reverseLimit() && m_motor.getState() == MotorState::REVERSE)
    {
        if (DeviceConfig::REVERSE_LIMIT_RUN_ON_MS == 0)
        {
            Logger::warning("REVERSE limit reached; motor stopped");
            m_motor.stop();
            return;
        }

        Logger::warning("REVERSE limit reached; run-on started");
        m_limitRunOnDirection = LimitRunOnDirection::REVERSE;
        m_limitRunOnStartMs = millis();
        return;
    }

    // -------------------------------------------------
    // BUTTON COMMANDS (Используем open/close)
    // -------------------------------------------------
    if (m_input.forwardTriggered())
    {
        if (m_input.forwardLimit())
        {
            Logger::warning("FORWARD blocked by limit switch");
            return;
        }

        open(CommandSource::BUTTON);
        return;
    }

    if (m_input.reverseTriggered())
    {
        if (m_input.reverseLimit())
        {
            Logger::warning("REVERSE blocked by limit switch");
            return;
        }

        close(CommandSource::BUTTON);
        return;
    }

    // -------------------------------------------------
    // IR REMOTE COMMANDS
    // -------------------------------------------------
    IRCommand command = m_irReceiver.getCommand();

    if (m_motor.isOverCurrent() && command != IRCommand::NONE) {
        m_motor.clearOverCurrent();
    }

    switch (command) {
        case IRCommand::FORWARD:
            open(CommandSource::IR);
            break;

        case IRCommand::REVERSE:
            close(CommandSource::IR);
            break;

        case IRCommand::STOP:
            stop(CommandSource::IR);
            break;

        case IRCommand::NONE:
        default:
            break;
    }
}