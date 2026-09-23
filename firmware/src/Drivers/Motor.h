#pragma once

#include <Arduino.h>
#include <atomic>

enum class MotorState
{
    STOPPED,
    FORWARD,
    REVERSE,
    DEAD_TIME
};

class Motor
{
public:
    Motor();

    void init();
    void update();
    void forward();
    void reverse();
    void stop();

    MotorState getState() const { return m_state; }

    void setSpeed(uint8_t speed);
    uint8_t getSpeed();
    
    // Энкодер
    int32_t getEncoderPosition() const;
    int32_t getMaxEncoderTicks() const;
    void setMaxEncoderTicks(int32_t maxTicks);
    void resetEncoder();

    // Защита и ток
    IRAM_ATTR static void emergencyStopFromISR();
    float getCurrentAmps();

    // ЕДИНЫЙ ФЛАГ АВАРИИ ДЛЯ ВНЕШНИХ МОДУЛЕЙ (Elevator)
    bool isEmergency() const { return m_isEmergency || s_hardwareFault.load(std::memory_order_relaxed); }

private:
    MotorState m_state = MotorState::STOPPED;
    MotorState m_targetState = MotorState::STOPPED;

    static constexpr uint32_t PWM_FREQUENCY = 20000;
    static constexpr uint8_t  PWM_RESOLUTION = 8;

    uint8_t  m_currentPwm{0};
    uint32_t m_lastRampMs{0};
    uint32_t m_deadTimeStartMs{0};
    uint32_t m_lastCurrentLogMs = 0;
    
    // Защита по току
    uint32_t m_moveStartMs = 0;
    uint32_t m_overcurrentStartMs = 0;
    uint32_t m_firstOvercurrentMs = 0;
    uint8_t  m_overcurrentResetCount = 0; // Сбросов токовой защиты

    // Паттерн 3х STOP за 2 сек
    uint32_t m_stopClickTimes[3] = {0, 0, 0};
    uint8_t  m_stopClickIndex = 0;

    // Энкодер
    static std::atomic<int32_t> s_encoderPosition; 
    static void IRAM_ATTR encoderISR();
    int32_t m_maxEncoderTicks = 0;
   
    // Флаги аварий
    static std::atomic<bool> s_hardwareFault; // Аппаратная авария по DIAG (от ISR)
    bool m_isEmergency = false;               // Общий флаг аварии (в т.ч. по току)
    bool m_isOvercurrentFault = false;        // Маркер, что авария именно токовая (для сброса)

    void setOvercurrentLED(bool enable);
    void checkOvercurrent(float);
    float readCurrentSensor();
    void registerStopClick();
    bool tryClearFault();

    // переменные вычисления силы тока
    static constexpr uint8_t ADC_SAMPLES_COUNT = 8;
    uint16_t m_adcBuffer[ADC_SAMPLES_COUNT] = {0};
    uint32_t m_adcSum = 0;
    uint8_t m_adcIndex = 0;
    float m_cachedCurrentAmps = 0.0f;
};