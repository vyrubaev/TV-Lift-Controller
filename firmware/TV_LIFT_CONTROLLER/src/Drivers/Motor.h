#pragma once

#include <Arduino.h>
#include <atomic> // Для безопасного обмена флагами между ISR и main thread

enum class MotorState
{
    STOPPED,
    FORWARD,
    REVERSE,
    EMERGENCY_STOP,
    OVERCURRENT
};

class Motor
{
public:
    Motor();

    void init();
    void update(); // - ВНИМАНИЕ- проверить!!! Вызывать в main loop! или может в Core::loop() для проверки аварийных условий и защиты по току - ВНИМАНИЕ- проверить!!!
    void forward();
    void reverse();
    void stop();
    void setSpeed(uint8_t speed);
    uint8_t getSpeed();
    MotorState getState();

    bool isEmergency() const; // Проверка, случалась ли авария
    void clearEmergency(); // Сброс флага аварии (если нужно восстановить работу)

    IRAM_ATTR static void emergencyStopFromISR(); // Обработчик прерывания для аварийной остановки мотора

    // Защита и диагностика
    float getCurrentAmps();
    bool isOverCurrent() const { return m_state == MotorState::OVERCURRENT; }
    void clearOverCurrent();

    // Управление светодиодом аварии  (для индикации состояния аварии)
    void setFaultLED(bool enable);

private:

    //static constexpr uint8_t PWM_CHANNEL = 0;
    static constexpr uint32_t PWM_FREQUENCY = 20000;
    static constexpr uint8_t PWM_RESOLUTION = 8;

    uint8_t m_speed;
   
    MotorState m_state = MotorState::STOPPED;

    uint32_t m_moveStartMs = 0; // Время старта движения мотора (для защиты по току)

    static std::atomic<bool> s_isEmergency; // Флаг аварийной остановки, доступный из ISR
    uint8_t m_overcurrentRetryCount = 0;
    uint32_t m_firstOvercurrentMs = 0; //  Нужно для отслеживания минуты в течени которой вохможно максимум 3 сброса OVERCURRENT
    uint32_t m_lastCurrentLogMs = 0; // Таймер периодического вывода тока
    bool m_isHardFault = false; // Блокировка до перезагрузки по питанию

    // Переменные таймера защиты по току
    uint32_t m_overcurrentStartMs = 0;
    
    // Вспомогательные методы
    void checkOvercurrent();
    float readCurrentSensor();

};