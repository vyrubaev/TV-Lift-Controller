#include "Motor.h"
#include <Arduino.h>
#include <string>
#include <cmath>
#include <algorithm>
#include "../Config/BoardConfig.h"
#include "../Config/DeviceConfig.h"
#include "Logger/Logger.h"
#include <soc/gpio_struct.h>
#include <rom/gpio.h>

std::atomic<bool> Motor::s_isEmergency{false}; 
volatile int32_t Motor::s_encoderPosition = 0; // Инициализация счетчика

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

    pinMode(BoardConfig::MOTOR1_DIAG, INPUT_PULLUP);

    if (digitalRead(BoardConfig::MOTOR1_DIAG) == LOW) 
    {
        emergencyStopFromISR();
    }
    
    attachInterrupt(
        digitalPinToInterrupt(BoardConfig::MOTOR1_DIAG),
        Motor::emergencyStopFromISR,
        FALLING
    );

    pinMode(BoardConfig::MOTOR1_CURR_SENS, INPUT); // пин для чтения данных по силе тока
    analogReadResolution(12);

    // Настройка пинов энкодера
    pinMode(BoardConfig::ENC_A, INPUT); // Важно: INPUT, т.к. GPIO 36/39 не поддерживают INPUT_PULLUP
    pinMode(BoardConfig::ENC_B, INPUT);

    attachInterrupt(
        digitalPinToInterrupt(BoardConfig::ENC_A),
        Motor::encoderISR,
        RISING
    );

    setMaxEncoderTicks(DeviceConfig::MAX_LIFT_ENCODER_TICKS); 
}

void Motor::update() {
    const uint32_t now = millis(); // Единый снимок времени на итерацию 

    // 1. Аппаратная авария с фильтрацией помех (5 мс)
    if (isEmergency()) {
        if (digitalRead(BoardConfig::MOTOR1_DIAG) == HIGH) {
            s_isEmergency.store(false, std::memory_order_relaxed);
            m_diagFaultStartMs = 0;
        } 
        else {
            if (m_diagFaultStartMs == 0) {
                m_diagFaultStartMs = now;
            } 
            else if (now - m_diagFaultStartMs >= 5) {
                if (m_state != MotorState::EMERGENCY_STOP) {
                    m_state = MotorState::EMERGENCY_STOP;
                    Logger::error("HARD FAULT: Motor driver reported DIAG ERROR (confirmed 5ms)!");
                    return;
                }
            }
        }
    } else {
        m_diagFaultStartMs = 0;
    }

    // 2. Dead Time и запуск
    if (m_targetState != MotorState::STOPPED && m_state != m_targetState) {

        bool isDeadTimePassed = (m_deadTimeStartMs == 0) || 
                                ((now - m_deadTimeStartMs) >= BoardConfig::MOTOR_DEAD_TIME_MS);

        if (isDeadTimePassed) {
            m_moveStartMs = now;
            m_overcurrentStartMs = 0;

            if (m_targetState == MotorState::FORWARD) {
                digitalWrite(BoardConfig::MOTOR1_INB, LOW);
                digitalWrite(BoardConfig::MOTOR1_INA, HIGH);
                m_state = MotorState::FORWARD;
                Logger::debug("Motor FORWARD");
            } 
            else if (m_targetState == MotorState::REVERSE) {
                digitalWrite(BoardConfig::MOTOR1_INA, LOW);
                digitalWrite(BoardConfig::MOTOR1_INB, HIGH);
                m_state = MotorState::REVERSE;
                Logger::debug("Motor REVERSE");
            }

            m_currentPwm = DeviceConfig::SOFT_START_MIN_PWM;
            ledcWrite(BoardConfig::MOTOR1_PWM, m_currentPwm);
            m_lastRampMs = now;

            Logger::debug("Motor STARTED successfully.");
        }
        return; 
    }



    // 3. Управление в движении

    if (m_state == MotorState::FORWARD || m_state == MotorState::REVERSE) {

        // --- ПРОВЕРКА ПРОГРАММНЫХ ОГРАНИЧИТЕЛЕЙ (SOFT LIMITS) ---
        if (m_maxEncoderTicks > 0) {
            int32_t currentPos = getEncoderPosition();

            // Если едем ВПЕРЕД и достигли/превысили максимум — останавливаемся
            if (m_state == MotorState::FORWARD && currentPos >= m_maxEncoderTicks) {
                stop();
                Logger::warning("Motor STOPPED: Reached MAX Encoder Soft Limit!");
                return;
            }

            // Если едем НАЗАД и ушли ниже или в 0 — останавливаемся
            if (m_state == MotorState::REVERSE && currentPos <= 0) {
                stop();
                Logger::warning("Motor STOPPED: Reached MIN (0) Encoder Soft Limit!");
                return;
            }
        }
        // Плавный разгон
        if (m_currentPwm < m_speed) {
            if (now - m_lastRampMs >= DeviceConfig::SOFT_START_STEP_MS) {
                m_lastRampMs = now;
                m_currentPwm = std::min<uint8_t>(m_speed, m_currentPwm + DeviceConfig::SOFT_START_STEP_PWM);
                ledcWrite(BoardConfig::MOTOR1_PWM, m_currentPwm);
            }
        }

        // Защита по току
        checkOvercurrent();
    
        // Логирование раз в секунду
        if (now - m_lastCurrentLogMs >= 1000) {
            m_lastCurrentLogMs = now;
            float currentAmps = getCurrentAmps();
            
            char logBuffer[64];
            snprintf(logBuffer, sizeof(logBuffer), "Motor current: %.2f A", currentAmps);
            Logger::info(logBuffer);
        }
    }
}

float Motor::readCurrentSensor() 
{
    constexpr uint8_t SAMPLES_COUNT = 8;
    uint32_t rawSum = 0;

    for (uint8_t i = 0; i < SAMPLES_COUNT; ++i) {
        rawSum += analogRead(BoardConfig::MOTOR1_CURR_SENS);
    }
    uint32_t rawAverage = rawSum / SAMPLES_COUNT;

    uint32_t voltageMv = (rawAverage * 3300) / 4095;
    float voltageV = voltageMv / 1000.0f;
    float netVoltageV = voltageV - DeviceConfig::CURRENT_SENSOR_OFFSET_V;

    if (netVoltageV <= 0.0f) {
        return 0.0f;
    }

    return netVoltageV / DeviceConfig::CURRENT_SENSOR_SENSITIVITY;
}

float Motor::getCurrentAmps() {
    return readCurrentSensor();
}

void Motor::checkOvercurrent() {
    const uint32_t now = millis();

    static uint32_t lastCheckMs = 0;
    if (now - lastCheckMs < 10) return;
    lastCheckMs = now;

    float currentAmps = readCurrentSensor();

    bool isStarting = (now - m_moveStartMs < DeviceConfig::startCurrentTimeoutMs);
    float activeLimit = isStarting ? (DeviceConfig::maxMotorCurrentAmps * 1.5f) 
                                   : DeviceConfig::maxMotorCurrentAmps;

    if (currentAmps >= activeLimit) {
        if (m_overcurrentStartMs == 0) {
            m_overcurrentStartMs = now;
        } 
        else if (now - m_overcurrentStartMs >= DeviceConfig::overcurrentTimeoutMs) {
            ledcWrite(BoardConfig::MOTOR1_PWM, 0);
            digitalWrite(BoardConfig::MOTOR1_INA, LOW);
            digitalWrite(BoardConfig::MOTOR1_INB, LOW);

            m_state = MotorState::OVERCURRENT;
            m_targetState = MotorState::STOPPED;
            setFaultLED(true); 

            if (m_firstOvercurrentMs == 0 || (now - m_firstOvercurrentMs > 60000)) {
                m_firstOvercurrentMs = now;
                m_overcurrentRetryCount = 1;
            } else {
                m_overcurrentRetryCount++;
            }

            Logger::error("OVERCURRENT FAULT: Motor current exceeded limit!");

            if (m_overcurrentRetryCount > 3) {
                m_isHardFault = true;
                Logger::error("CRITICAL FAULT: Too many overcurrent events in 1 min!");
            }
        }
    } else {
        m_overcurrentStartMs = 0;
    }
}

void Motor::setFaultLED(bool enable) {
    if (enable) {
        detachInterrupt(digitalPinToInterrupt(BoardConfig::MOTOR1_DIAG));
        pinMode(BoardConfig::MOTOR1_DIAG, OUTPUT);
        digitalWrite(BoardConfig::MOTOR1_DIAG, LOW);
    } else {
        pinMode(BoardConfig::MOTOR1_DIAG, INPUT_PULLUP);
        attachInterrupt(
            digitalPinToInterrupt(BoardConfig::MOTOR1_DIAG),
            Motor::emergencyStopFromISR,
            FALLING
        );
    }
}

void Motor::clearOverCurrent() {
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

void Motor::forward() { 
    if (m_targetState == MotorState::FORWARD) return;

    if (m_state == MotorState::OVERCURRENT || isEmergency()) {
        Logger::warning("Motor forward blocked: active FAULT!");
        return;
    }

    if (m_state == MotorState::REVERSE) {
        stop(); 
        m_deadTimeStartMs = millis(); 
    } else {
        m_deadTimeStartMs = 0; 
    }

    m_targetState = MotorState::FORWARD;
    Logger::debug("Motor target set to FORWARD");
}

void Motor::reverse() {   
    if (m_targetState == MotorState::REVERSE) return;

    if (m_state == MotorState::OVERCURRENT || isEmergency()) {
        Logger::warning("Motor reverse blocked: active FAULT!");
        return;
    }

    if (m_state == MotorState::FORWARD) {
        stop(); 
        m_deadTimeStartMs = millis(); 
    } else {
        m_deadTimeStartMs = 0; 
    }

    m_targetState = MotorState::REVERSE;
    Logger::debug("Motor target set to REVERSE");
}

void Motor::stop()
{    
    ledcWrite(BoardConfig::MOTOR1_PWM, 0);
    digitalWrite(BoardConfig::MOTOR1_INA, LOW);
    digitalWrite(BoardConfig::MOTOR1_INB, LOW);

    m_currentPwm = 0;
    m_moveStartMs = 0;
    m_targetState = MotorState::STOPPED;

    if (s_isEmergency.load(std::memory_order_relaxed)) {
        m_state = MotorState::EMERGENCY_STOP;
    } 
    else if (m_state != MotorState::OVERCURRENT) {
        m_state = MotorState::STOPPED;
    }
    
    Logger::debug("Motor stop executed");
}

void IRAM_ATTR Motor::emergencyStopFromISR()
{
    gpio_matrix_out(BoardConfig::MOTOR1_PWM, SIG_GPIO_OUT_IDX, false, false);

    uint32_t lowMask = 0;
    uint32_t highMask = 0;

    const uint8_t pins[3] = { BoardConfig::MOTOR1_INA, BoardConfig::MOTOR1_INB, BoardConfig::MOTOR1_PWM };
    
    for (uint8_t i = 0; i < 3; i++) {
        uint8_t pin = pins[i];
        if (pin < 32) {
            lowMask |= (1UL << pin);
        } else if (pin < 40) {
            highMask |= (1UL << (pin - 32));
        }
    }

    if (lowMask > 0)  GPIO.out_w1tc = lowMask;
    if (highMask > 0) GPIO.out1_w1tc.val = highMask;

    s_isEmergency.store(true, std::memory_order_relaxed);
}

bool Motor::isEmergency() const
{
    return s_isEmergency.load(std::memory_order_relaxed);
}

void Motor::clearEmergency()
{   
    // Перед проверкой уровня сбрасываем режимы пинов
    setFaultLED(false);

    if (digitalRead(BoardConfig::MOTOR1_DIAG) == LOW) {
        Logger::warning("Cannot clear emergency: motor driver Error! (DIAG pin is still LOW)");
        return;
    }

    if (!m_isHardFault) {
        s_isEmergency.store(false, std::memory_order_relaxed);

        ledcAttach(
            BoardConfig::MOTOR1_PWM,
            PWM_FREQUENCY,
            PWM_RESOLUTION
        );

        if (m_state == MotorState::EMERGENCY_STOP) {
            m_state = MotorState::STOPPED;
        }

        Logger::info("Emergency status cleared and PWM re-attached.");
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

void IRAM_ATTR Motor::encoderISR() {
    // Читаем B-фазу для определения направления
    if (digitalRead(BoardConfig::ENC_B) == HIGH) {
        s_encoderPosition = s_encoderPosition + 1;
    } else {
        s_encoderPosition = s_encoderPosition - 1;
    }
}

void Motor::resetEncoder() { 
    s_encoderPosition = 0; // сброс счетчика энкодера при достижении определенного концевика
    Logger::info("Encoder position reset to 0.");
}

int32_t Motor::getEncoderPosition() const {
    return s_encoderPosition;
}

void Motor::setMaxEncoderTicks(int32_t maxTicks) {
    m_maxEncoderTicks = maxTicks;
    char logBuffer[64];
    snprintf(logBuffer, sizeof(logBuffer), "Motor max encoder limit set to: %ld ticks", (long)maxTicks);
    Logger::info(logBuffer);
}

int32_t Motor::getMaxEncoderTicks() const {
    return m_maxEncoderTicks;
}