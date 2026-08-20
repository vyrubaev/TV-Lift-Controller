#include "Motor.h"
#include <Arduino.h>
#include <string>
#include <cmath>
#include "../Config/BoardConfig.h"
#include "../Config/DeviceConfig.h"
#include "Logger/Logger.h"
#include <soc/gpio_struct.h>

std::atomic<bool> Motor::s_isEmergency{false};

Motor::Motor()
    : m_speed(DeviceConfig::MOTOR_SPEED),
      m_state(MotorState::STOPPED)
{
}

void Motor::init()
{
    pinMode(BoardConfig::MOTOR1_INA, OUTPUT);
    pinMode(BoardConfig::MOTOR1_INB, OUTPUT);

    ledcAttach(
        BoardConfig::MOTOR1_PWM,
        PWM_FREQUENCY,
        PWM_RESOLUTION
    );

    stop();

    pinMode(BoardConfig::MOTOR1_DIAG, INPUT);

    if (digitalRead(BoardConfig::MOTOR1_DIAG) == LOW) 
    {
        emergencyStopFromISR();
    }
    
    attachInterrupt(
        digitalPinToInterrupt(BoardConfig::MOTOR1_DIAG),
        Motor::emergencyStopFromISR,
        FALLING
    );

    pinMode(BoardConfig::MOTOR1_CURR_SENS, INPUT);
    analogReadResolution(12);
}

void Motor::update() {
    // 1. Проверяем аппаратную аварию от драйвера (DIAG пин)
    if (isEmergency()) {
        if (m_state != MotorState::EMERGENCY_STOP) {
            m_state = MotorState::EMERGENCY_STOP;
            Logger::error("HARD FAULT: Motor driver reported DIAG ERROR! Lockout active.");
        }
        return;
    }

    // 2. Если мотор движется — проверяем ток и выводим логи
    if (m_state == MotorState::FORWARD || m_state == MotorState::REVERSE) {
        
        // Сначала проверяем перегрузку
        checkOvercurrent();

        // Вывод силы тока раз в секунду (только если мотор всё ещё в движении)
        if (m_state == MotorState::FORWARD || m_state == MotorState::REVERSE) {
            uint32_t now = millis();
            if (now - m_lastCurrentLogMs >= 3000) {
                m_lastCurrentLogMs = now;
                float currentAmps = getCurrentAmps();
                
                char logBuffer[64];
                snprintf(logBuffer, sizeof(logBuffer), "Motor current: %.2f A", currentAmps);
                
                Logger::info(logBuffer);
            }
        }
    }
}

float Motor::readCurrentSensor() {
    uint32_t raw = analogRead(BoardConfig::MOTOR1_CURR_SENS);
   
    float voltage = (raw / 4095.0f) * 3.3f;
    float current = (voltage - DeviceConfig::CURRENT_SENSOR_OFFSET_V) / DeviceConfig::CURRENT_SENSOR_SENSITIVITY;

    return std::abs(current);
}

float Motor::getCurrentAmps() {
    return readCurrentSensor();
}

void Motor::checkOvercurrent() {

    // Игнорируем замер сразу после запуска мотора в течении DeviceConfig::startCurrentTimeoutMs мс, т.к. стартовый ток может быть выше порога
    if (millis() - m_moveStartMs < DeviceConfig::startCurrentTimeoutMs) {
        m_overcurrentStartMs = 0;
        return;
    }
    float currentAmps = readCurrentSensor();

    if (currentAmps >= DeviceConfig::maxMotorCurrentAmps) {
        if (m_overcurrentStartMs == 0) {
            m_overcurrentStartMs = millis();
        } 
        else if (millis() - m_overcurrentStartMs >= DeviceConfig::overcurrentTimeoutMs) {
            // Выключаем аппаратные выходы напрямую без вызова stop()
            ledcWrite(BoardConfig::MOTOR1_PWM, 0);
            digitalWrite(BoardConfig::MOTOR1_INA, LOW);
            digitalWrite(BoardConfig::MOTOR1_INB, LOW);

            m_state = MotorState::OVERCURRENT;
            setFaultLED(true); 

            const uint32_t now = millis();

            // Окно в 1 минуту (60 000 мс)
            if (m_firstOvercurrentMs == 0 || (now - m_firstOvercurrentMs > 60000)) {
                m_firstOvercurrentMs = now;
                m_overcurrentRetryCount = 1;
            } else {
                m_overcurrentRetryCount++;
            }

            Logger::error("OVERCURRENT FAULT: Motor current exceeded limit!");

            // Если превысили 3 попытки за 1 минуту — жёсткая блокировка (как от драйвера)
            if (m_overcurrentRetryCount > 3) {
                m_isHardFault = true;
                Logger::error("CRITICAL FAULT: Too many overcurrent events in 1 min! Power reboot required.");
            }
        }
    } else {
        m_overcurrentStartMs = 0;
    }
}

void Motor::setFaultLED(bool enable) {
    if (enable) {
        detachInterrupt(digitalPinToInterrupt(BoardConfig::MOTOR1_DIAG)); // Снимаем ISR
        pinMode(BoardConfig::MOTOR1_DIAG, OUTPUT);
        digitalWrite(BoardConfig::MOTOR1_DIAG, LOW);
    } else {
        pinMode(BoardConfig::MOTOR1_DIAG, INPUT_PULLUP);
        attachInterrupt(
            digitalPinToInterrupt(BoardConfig::MOTOR1_DIAG),
            Motor::emergencyStopFromISR,
            FALLING
        ); // Возвращаем ISR
    }
}

void Motor::clearOverCurrent() {
    // Если ушли в фатальную блокировку — сброс запрещён (только ребут)
    if (m_isHardFault || isEmergency()) {
        Logger::warning("Cannot clear overcurrent: HARD FAULT requires power reboot!");
        return;
    }

    if (m_state == MotorState::OVERCURRENT) {
        setFaultLED(false);
        m_state = MotorState::STOPPED;
        m_overcurrentStartMs = 0;
        Logger::info("OVERCURRENT cleared by user command.");
    }
}

void Motor::reverse()
{   if (m_state == MotorState::REVERSE) {
        Logger::debug("Motor is already moving reverse");
        return;
    }

    // Блокируем движение, если мотор уже в OVERCURRENT или EMERGENCY
    if (m_state == MotorState::OVERCURRENT || isEmergency()) {
        Logger::warning("Motor forward blocked: active OVERCURRENT or EMERGENCY fault!");
        return;
    }

    stop();
    m_moveStartMs = millis(); // Фиксируем время пуска
    m_overcurrentStartMs = 0; // Очищаем старые замеры

    digitalWrite(BoardConfig::MOTOR1_INA, LOW);
    digitalWrite(BoardConfig::MOTOR1_INB, HIGH);
    ledcWrite(BoardConfig::MOTOR1_PWM, m_speed);

    m_state = MotorState::REVERSE;    
    Logger::debug("Motor reverse");
}

void Motor::forward()
{ 
    if (m_state == MotorState::FORWARD) {
        Logger::debug("Motor is already moving forward");
        return;
    }

    // Блокируем движение, если мотор уже в OVERCURRENT или EMERGENCY
    if (m_state == MotorState::OVERCURRENT || isEmergency()) {
        Logger::warning("Motor forward blocked: active OVERCURRENT or EMERGENCY fault!");
        return;
    }

    stop();
    m_moveStartMs = millis(); // Фиксируем время пуска
    m_overcurrentStartMs = 0; // Очищаем старые замеры

    digitalWrite(BoardConfig::MOTOR1_INB, LOW);
    digitalWrite(BoardConfig::MOTOR1_INA, HIGH);
    ledcWrite(BoardConfig::MOTOR1_PWM, m_speed);
    
    m_state = MotorState::FORWARD;  
    Logger::debug("Motor forward");
}

void Motor::stop()
{    
    ledcWrite(BoardConfig::MOTOR1_PWM, 0);
    digitalWrite(BoardConfig::MOTOR1_INA, LOW);
    digitalWrite(BoardConfig::MOTOR1_INB, LOW);

    // Авто-сброс по току при отправке команды stop (если попытки не исчерпаны)
    if (m_state == MotorState::OVERCURRENT) {
        clearOverCurrent();
        return;
    }
    
    if (s_isEmergency.load(std::memory_order_relaxed)) {
        m_state = MotorState::EMERGENCY_STOP;
    } else if (m_state != MotorState::OVERCURRENT) {
        m_state = MotorState::STOPPED;
    }
    
    Logger::debug("Motor stop");
}

void IRAM_ATTR Motor::emergencyStopFromISR()
{
    constexpr uint32_t MOTOR_DIRECTION_MASK =
        (1UL << BoardConfig::MOTOR1_INA) |
        (1UL << BoardConfig::MOTOR1_INB);

    GPIO.out_w1tc = MOTOR_DIRECTION_MASK;
    GPIO.out_w1tc = (1UL << BoardConfig::MOTOR1_PWM);

    s_isEmergency.store(true, std::memory_order_relaxed);
}

bool Motor::isEmergency() const
{
    return s_isEmergency.load(std::memory_order_relaxed);
}

void Motor::clearEmergency()
{   // Если на пине всё еще физический LOW — драйвер не восстановился
    if (digitalRead(BoardConfig::MOTOR1_DIAG) == LOW) {
        Logger::warning("Cannot clear emergency: motor driver Error! (DIAG pin is still LOW)");
        return;
    }

    // Сброс аппаратной аварии запрещён вручную, если включён hard fault
    if (!m_isHardFault) {
        s_isEmergency.store(false, std::memory_order_relaxed);
    }
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