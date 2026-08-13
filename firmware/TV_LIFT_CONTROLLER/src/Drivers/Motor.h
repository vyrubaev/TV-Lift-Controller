#pragma once

#include <Arduino.h>
#include <atomic> // Для безопасного обмена флагами между ISR и main thread

enum class MotorState
{
    STOPPED,
    FORWARD,
    REVERSE,
    EMERGENCY_STOP
};

class Motor
{
public:
    Motor();

    void init();

    void forward();

    void reverse();

    void stop();
    
    bool isEmergency() const; // Проверка, случалась ли авария
    
    void clearEmergency(); // Сброс флага аварии (если нужно восстановить работу)

    IRAM_ATTR static void emergencyStopFromISR(); // Обработчик прерывания для аварийной остановки мотора

    MotorState getState();

    void setSpeed(uint8_t speed);

    uint8_t getSpeed();

private:

    //static constexpr uint8_t PWM_CHANNEL = 0;
    static constexpr uint32_t PWM_FREQUENCY = 20000;
    static constexpr uint8_t PWM_RESOLUTION = 8;

    uint8_t m_speed;
   
    MotorState m_state = MotorState::STOPPED;

    static std::atomic<bool> s_isEmergency; // Флаг аварийной остановки, доступный из ISR

};