#include "Motor.h"

#include <Arduino.h>

#include "../Config/BoardConfig.h"

#include "../Config/DeviceConfig.h"

#include "Logger/Logger.h" // тут он для тестов, в будущем будет удален

Motor::Motor()
    : m_speed(DeviceConfig::MOTOR_SPEED)
{
}

void Motor::init()
{

    pinMode(
        BoardConfig::MOTOR1_INA,
        OUTPUT);

    pinMode(
        BoardConfig::MOTOR1_INB,
        OUTPUT);

    ledcAttach(
        BoardConfig::MOTOR1_PWM,
        PWM_FREQUENCY,
        PWM_RESOLUTION);

    stop();
}

void Motor::reverse()
{
    if(m_state == MotorState::REVERSE)
    {
        return;
    }

    stop();

    delay(10);

    digitalWrite(
        BoardConfig::MOTOR1_INA,
        LOW);

    digitalWrite(
        BoardConfig::MOTOR1_INB,
        HIGH);

    ledcWrite(
        BoardConfig::MOTOR1_PWM,
        m_speed);

    m_state = MotorState::REVERSE;    

    Logger::debug("Motor reverse");
}

void Motor::forward()
{
    if(m_state == MotorState::FORWARD)
    {
        return;
    }

    stop();

    delay(10);

    digitalWrite(
        BoardConfig::MOTOR1_INB,
        LOW);

    digitalWrite(
        BoardConfig::MOTOR1_INA,
        HIGH);

    ledcWrite(
        BoardConfig::MOTOR1_PWM,
        m_speed);
    
    m_state = MotorState::FORWARD;  
    Logger::debug("Motor forward");  //
}

void Motor::stop()
{
    if(m_state == MotorState::STOPPED)
    {
        return;
    }

    ledcWrite(
        BoardConfig::MOTOR1_PWM,
        0);

    digitalWrite(
        BoardConfig::MOTOR1_INA,
        LOW);

    digitalWrite(
        BoardConfig::MOTOR1_INB,
        LOW);
    
    m_state = MotorState::STOPPED;  
    
    Logger::debug("Motor stop");

}

MotorState Motor::getState()
{
    return m_state;
}

void Motor::setSpeed(uint8_t speed)
{
    m_speed = speed;

    Logger::debug("Motor speed changed");
}

uint8_t Motor::getSpeed()
{
    return m_speed;
}