#pragma once

#include "../Drivers/Motor.h"
#include "../Inputs/InputManager.h"
#include "../Drivers/IRReceiver.h"
#include "../Config/DeviceConfig.h"
#include <atomic>

enum ElevatorMountType : uint8_t {
    MOUNT_FLOOR = 0,   // Напольный (Вверх = Open)
    MOUNT_CEILING = 1, // Потолочный (Вниз = Open)
    MOUNT_WALL = 2     // Настенный (Выдвижение = Open)
};

enum class CommandSource {
    NONE,
    BUTTON,
    IR,
    CLI,
    WEB,
    APP
};

enum class ElevatorState {
    OPEN,
    CLOSED,
    MOVING_UP,
    MOVING_DOWN,
    RUN_ON,
    STOPPED,
    EMERGENCY,
    TIMEOUT,
    UNKNOWN
};

class Elevator
{
public:
    // 1. Выносим PendingCommand в PUBLIC, чтобы метод postWebCommand и WebManager видели этот тип
    struct PendingCommand {
        enum class Type { NONE, UP, DOWN, STOP };
        Type type = Type::NONE;
        CommandSource source = CommandSource::NONE;
    };

    Elevator();

    void init();
    void update();
    void moveUp(CommandSource src = CommandSource::NONE);
    void moveDown(CommandSource src = CommandSource::NONE);
    void stop(CommandSource src = CommandSource::NONE);

    float getCurrentAmps();

    // 2. Теперь метод корректно видит публичный PendingCommand::Type
    void postWebCommand(PendingCommand::Type type) {
        m_webPendingType.store(type, std::memory_order_relaxed);
    }

    // Геттер для получения текущего состояния (например, для Web/CLI)
    ElevatorState getState() const { return m_state; }
    const char* sourceToString(CommandSource src);

private:
    void setState(ElevatorState newState);
    ElevatorState m_state = ElevatorState::UNKNOWN;

    // Потокобезопасная переменная для Web-команды
    std::atomic<PendingCommand::Type> m_webPendingType{PendingCommand::Type::NONE};

    PendingCommand getNextCommand();
    void executeCommand(const PendingCommand& cmd);

    bool isForwardLimitReached();
    bool isReverseLimitReached();

    enum class LimitRunOnDirection : uint8_t {
        NONE,
        FORWARD,
        REVERSE
    };

    bool m_invertStatus = false;

    LimitRunOnDirection m_limitRunOnDirection = LimitRunOnDirection::NONE;
    uint32_t m_limitRunOnStartMs = 0;

    void handleLimitReached(LimitRunOnDirection dir);

    bool m_overcurrentTimerActive = false;

    uint32_t m_moveStartMs{0}; // Время запуска мотора

    Motor m_motor;
    InputManager m_input;
    IRReceiver m_irReceiver;
};