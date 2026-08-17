#pragma once

#include "../Drivers/Motor.h"
#include "../Inputs/InputManager.h"
#include "../Drivers/IRReceiver.h"
#include "../Config/DeviceConfig.h"

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
    FORWARD,
    REVERSE,
    RUN_ON,
    IDLE,
    EMERGENCY
};

class Elevator
{
public:
    Elevator();

    void init();
    void update();
    void open(CommandSource src = CommandSource::NONE);
    void close(CommandSource src = CommandSource::NONE);
    void stop(CommandSource src = CommandSource::NONE);

    const char* sourceToString(CommandSource src);

private:
    enum class LimitRunOnDirection : uint8_t {
        NONE,
        FORWARD,
        REVERSE
    };

    bool m_invertMotor = false;

    LimitRunOnDirection m_limitRunOnDirection = LimitRunOnDirection::NONE;
    uint32_t m_limitRunOnStartMs = 0;

    Motor m_motor;
    InputManager m_input;
    IRReceiver m_irReceiver;
};