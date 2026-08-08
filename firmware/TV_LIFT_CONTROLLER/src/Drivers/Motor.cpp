#include "Motor.h"

#include <Arduino.h>

#include "../Config/BoardConfig.h"

#include "../Config/DeviceConfig.h"

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
}

void Motor::forward()
{
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
}

void Motor::stop()
{
    ledcWrite(
        BoardConfig::MOTOR1_PWM,
        0);

    digitalWrite(
        BoardConfig::MOTOR1_INA,
        LOW);

    digitalWrite(
        BoardConfig::MOTOR1_INB,
        LOW);

}