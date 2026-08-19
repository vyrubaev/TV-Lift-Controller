#include "Elevator.h"
#include "../Logger/Logger.h"
#include <cstdio>

Elevator::Elevator()
    : m_state(ElevatorState::UNKNOWN)
{
}

void Elevator::init()
{
    if (DeviceConfig::MOUNT_TYPE == MOUNT_CEILING) {
        m_invertStatus = true;
        Logger::info("Elevator initialized: CEILING type");
    } else if (DeviceConfig::MOUNT_TYPE == MOUNT_WALL) {
        m_invertStatus = false;
        Logger::info("Elevator initialized: WALL type");
    } else {
        m_invertStatus = false;
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

// -------------------------------------------------
// Помощник для установки состояния и вывода логов
// -------------------------------------------------
void Elevator::setState(ElevatorState newState) {
    if (m_state == newState) return;
    
    m_state = newState;
    switch (m_state) {
        case ElevatorState::OPEN:
            Logger::info("Elevator status: OPENED");
            break;
        case ElevatorState::CLOSED:
            Logger::info("Elevator status: CLOSED");
            break;
        case ElevatorState::MOVING_UP:
            Logger::info("Elevator status: MOVING UP");
            break;
        case ElevatorState::MOVING_DOWN:
            Logger::info("Elevator status: MOVING DOWN");
            break;
        case ElevatorState::STOPPED:
            Logger::info("Elevator status: STOPPED");
            break;
        case ElevatorState::EMERGENCY:
            Logger::error("Elevator status: EMERGENCY FAULT");
            break;
        default:
            break;
    }
}

// Вызывается при полной остановке мотора (в т.ч. после добега)
void Elevator::handleLimitReached(LimitRunOnDirection dir) {
    m_motor.stop();
    m_limitRunOnDirection = LimitRunOnDirection::NONE;

    if (dir == LimitRunOnDirection::FORWARD) {
        // Если потолочный — FORWARD это закрытие, иначе — открытие
        setState(m_invertStatus ? ElevatorState::CLOSED : ElevatorState::OPEN);
    } else if (dir == LimitRunOnDirection::REVERSE) {
        // Если потолочный — REVERSE это открытие, иначе — закрытие
        setState(m_invertStatus ? ElevatorState::OPEN : ElevatorState::CLOSED);
    }
}

void Elevator::moveUp(CommandSource src) {
    char logBuffer[64];
    snprintf(logBuffer, sizeof(logBuffer), "Command UP received from: %s", sourceToString(src));
    Logger::info(logBuffer);

    m_motor.forward();
    setState(ElevatorState::MOVING_UP);
}

void Elevator::moveDown(CommandSource src) {
    char logBuffer[64];
    snprintf(logBuffer, sizeof(logBuffer), "Command DOWN received from: %s", sourceToString(src));
    Logger::info(logBuffer);

    m_motor.reverse();
    setState(ElevatorState::MOVING_DOWN);
}

void Elevator::stop(CommandSource src) {
    char logBuffer[64];
    snprintf(logBuffer, sizeof(logBuffer), "Command STOP received from: %s", sourceToString(src));
    Logger::info(logBuffer);

    m_limitRunOnDirection = LimitRunOnDirection::NONE;
    m_motor.stop();
    setState(ElevatorState::STOPPED);
}

bool Elevator::isForwardLimitReached() {
    return m_input.forwardLimit();
}

bool Elevator::isReverseLimitReached() {
    return m_input.reverseLimit();
}

// -------------------------------------------------
// Сбор команд со всех источников
// -------------------------------------------------
Elevator::PendingCommand Elevator::getNextCommand()
{
    if (m_input.stopTriggered()) {
        return { PendingCommand::Type::STOP, CommandSource::BUTTON };
    }

    if (m_input.forwardTriggered()) {
        return { PendingCommand::Type::UP, CommandSource::BUTTON };
    }
    if (m_input.reverseTriggered()) {
        return { PendingCommand::Type::DOWN, CommandSource::BUTTON };
    }

    IRCommand irCmd = m_irReceiver.getCommand();
    if (irCmd == IRCommand::STOP) {
        return { PendingCommand::Type::STOP, CommandSource::IR };
    }
    if (irCmd == IRCommand::UP) {
        return { PendingCommand::Type::UP, CommandSource::IR };
    }
    if (irCmd == IRCommand::DOWN) {
        return { PendingCommand::Type::DOWN, CommandSource::IR };
    }

    return { PendingCommand::Type::NONE, CommandSource::NONE };
}

// -------------------------------------------------
// Исполнитель команд
// -------------------------------------------------
void Elevator::executeCommand(const PendingCommand& cmd)
{
    switch (cmd.type) {
        case PendingCommand::Type::STOP:
            // Сброс OVERCURRENT происходит ТОЛЬКО при вызове STOP
            if (m_motor.isOverCurrent()) {
                m_motor.clearOverCurrent();
            }
            stop(cmd.source);
            break;

        case PendingCommand::Type::UP:
            // Если мотор в аварии по току — игнорируем команду UP
            if (m_motor.isOverCurrent()) {
                Logger::warning("UP blocked: Motor is in OVERCURRENT fault! Press STOP to reset.");
                return;
            }
            if (isForwardLimitReached()) {
                Logger::warning("UP blocked: FORWARD limit switch is active!");
                return;
            }
            moveUp(cmd.source);
            break;

        case PendingCommand::Type::DOWN:
            // Если мотор в аварии по току — игнорируем команду DOWN
            if (m_motor.isOverCurrent()) {
                Logger::warning("DOWN blocked: Motor is in OVERCURRENT fault! Press STOP to reset.");
                return;
            }
            if (isReverseLimitReached()) {
                Logger::warning("DOWN blocked: REVERSE limit switch is active!");
                return;
            }
            moveDown(cmd.source);
            break;

        case PendingCommand::Type::NONE:
        default:
            break;
    }
}

// -------------------------------------------------
// Главный цикл
// -------------------------------------------------
void Elevator::update()
{   
    m_input.update();
    m_irReceiver.update();
    m_motor.update();

    // 1. Проверка АВАРИИ
    if (m_motor.isEmergency())
    {
        if (m_motor.getState() != MotorState::EMERGENCY_STOP)
        {
            Logger::error("EMERGENCY FAULT: Motor driver reported error via DIAGNOSTIC pin!");
            m_limitRunOnDirection = LimitRunOnDirection::NONE;
            m_motor.stop();
            setState(ElevatorState::EMERGENCY);
        }
        return;
    }

    // 2. Обработка завершения добега
    if (m_limitRunOnDirection != LimitRunOnDirection::NONE)
    {
        const uint32_t now = millis();
        uint32_t runOnTimeMs = (m_limitRunOnDirection == LimitRunOnDirection::FORWARD) 
            ? DeviceConfig::FORWARD_LIMIT_RUN_ON_MS 
            : DeviceConfig::REVERSE_LIMIT_RUN_ON_MS;

        if (now - m_limitRunOnStartMs >= runOnTimeMs)
        {
            Logger::info("Limit run-on completed");
            handleLimitReached(m_limitRunOnDirection); // Останавливаем и пишем статус OPEN/CLOSED
        }
        return;
    }

    // 3. Остановка по концевику при движении FORWARD
    if (m_input.forwardLimit() && m_motor.getState() == MotorState::FORWARD)
    {
        if (DeviceConfig::FORWARD_LIMIT_RUN_ON_MS == 0)
        {
            Logger::warning("FORWARD limit reached");
            handleLimitReached(LimitRunOnDirection::FORWARD); // Мгновенный стоп + статус
            return;
        }

        Logger::warning("FORWARD limit reached; run-on started");
        m_limitRunOnDirection = LimitRunOnDirection::FORWARD;
        m_limitRunOnStartMs = millis();
        return;
    }

    // 4. Остановка по концевику при движении REVERSE
    if (m_input.reverseLimit() && m_motor.getState() == MotorState::REVERSE)
    {
        if (DeviceConfig::REVERSE_LIMIT_RUN_ON_MS == 0)
        {
            Logger::warning("REVERSE limit reached");
            handleLimitReached(LimitRunOnDirection::REVERSE); // Мгновенный стоп + статус
            return;
        }

        Logger::warning("REVERSE limit reached; run-on started");
        m_limitRunOnDirection = LimitRunOnDirection::REVERSE;
        m_limitRunOnStartMs = millis();
        return;
    }

    // 5. Сбор и выполнение новых команд
    PendingCommand cmd = getNextCommand();
    if (cmd.type != PendingCommand::Type::NONE) {
        executeCommand(cmd);
    }
}