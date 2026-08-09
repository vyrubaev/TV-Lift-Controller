#pragma once

#include <Arduino.h>

enum class MotorState
{
    STOPPED,
    FORWARD,
    REVERSE
};

class Motor
{
public:
    Motor();

    void init();

    void forward();

    void reverse();

    void stop();

    MotorState getState();

    void setSpeed(uint8_t speed);

    uint8_t getSpeed();

private:

    //static constexpr uint8_t PWM_CHANNEL = 0;
    static constexpr uint32_t PWM_FREQUENCY = 20000;
    static constexpr uint8_t PWM_RESOLUTION = 8;

    uint8_t m_speed;
   
    MotorState m_state = MotorState::STOPPED;

};