#include "Motor.h"
#include <Arduino.h>
#include <cstring>
#include "../Config/BoardConfig.h"
#include "../Config/DeviceConfig.h"
#include "Logger/Logger.h"
#include <soc/gpio_struct.h>
#include <rom/gpio.h>

std::atomic<bool> Motor::s_hardwareFault{false}; 
std::atomic<int32_t> Motor::s_encoderPosition{0};

Motor::Motor()
    : m_state(MotorState::STOPPED)
{
}

void Motor::init()
{
    pinMode(BoardConfig::MOTOR1_INA, OUTPUT);
    pinMode(BoardConfig::MOTOR1_INB, OUTPUT);
    pinMode(BoardConfig::MOTOR1_DIAG, INPUT_PULLUP);
    pinMode(BoardConfig::IR_TRANSMITTER_PIN, OUTPUT);
    digitalWrite(BoardConfig::IR_TRANSMITTER_PIN, LOW);

    pinMode(BoardConfig::ENC_A, INPUT);
    pinMode(BoardConfig::ENC_B, INPUT);
    pinMode(BoardConfig::MOTOR1_CURR_SENS, INPUT);
    analogReadResolution(12);

    // Инициализация PWM (совместимо с ESP32 Core v3.x)
    ledcAttach(
        BoardConfig::MOTOR1_PWM,
        PWM_FREQUENCY,
        PWM_RESOLUTION
    );
    
    stop();

    if (digitalRead(BoardConfig::MOTOR1_DIAG) == LOW) 
    {
        emergencyStopFromISR();
    }
    
    attachInterrupt(
        digitalPinToInterrupt(BoardConfig::MOTOR1_DIAG),
        Motor::emergencyStopFromISR,
        FALLING
    );

    attachInterrupt(
        digitalPinToInterrupt(BoardConfig::ENC_A),
        Motor::encoderISR,
        RISING
    );

    setMaxEncoderTicks(DeviceConfig::MAX_LIFT_ENCODER_TICKS); 
}

void IRAM_ATTR Motor::emergencyStopFromISR()
{
    // Отключение HW PWM и мгновенный сброс управляющих выводов
    gpio_matrix_out(BoardConfig::MOTOR1_PWM, SIG_GPIO_OUT_IDX, false, false);

    uint32_t lowMask = 0;
    if (BoardConfig::MOTOR1_INA < 32) lowMask |= (1UL << BoardConfig::MOTOR1_INA);
    if (BoardConfig::MOTOR1_INB < 32) lowMask |= (1UL << BoardConfig::MOTOR1_INB);
    if (BoardConfig::MOTOR1_PWM < 32) lowMask |= (1UL << BoardConfig::MOTOR1_PWM);

    if (lowMask > 0) GPIO.out_w1tc = lowMask;

    s_hardwareFault.store(true, std::memory_order_relaxed);
}

void Motor::update() {
    const uint32_t now = millis();

    // 1. Блокировка при аварии
    if (isEmergency()) {
        // Если была попытка запустить мотор (m_targetState не STOPPED)
        if (m_targetState != MotorState::STOPPED || m_state != MotorState::STOPPED) {
            Logger::warning("Motor action ignored: Active EMERGENCY condition!");
            stop(); // Полный сброс PWM и целевого состояния
        }
        return; 
    }

    // 2. Dead Time и запуск движения
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

    // 3. Управление в процессе движения
    if (m_state == MotorState::FORWARD || m_state == MotorState::REVERSE) {

        // Ограничения по энкодеру
        if (m_maxEncoderTicks > 0) {
            int32_t currentPos = getEncoderPosition();

            if (m_state == MotorState::FORWARD && currentPos >= m_maxEncoderTicks) {
                stop();
                Logger::warning("Motor STOPPED: Reached MAX Encoder Soft Limit!");
                return;
            }

            if (m_state == MotorState::REVERSE && currentPos <= 0) {
                stop();
                Logger::warning("Motor STOPPED: Reached MIN (0) Encoder Soft Limit!");
                return;
            }
        }

        // Плавный разгон (Soft Start)
        if (m_currentPwm < DeviceConfig::MOTOR_SPEED) {
            if (now - m_lastRampMs >= DeviceConfig::SOFT_START_STEP_MS) {
                m_lastRampMs = now;
                int nextPwm = m_currentPwm + DeviceConfig::SOFT_START_STEP_PWM;
                if (nextPwm > DeviceConfig::MOTOR_SPEED) nextPwm = DeviceConfig::MOTOR_SPEED;
                if (nextPwm > 255) nextPwm = 255;
                
                m_currentPwm = (uint8_t)nextPwm;
                ledcWrite(BoardConfig::MOTOR1_PWM, m_currentPwm);
            }
        }

        // Защита по току
        checkOvercurrent();

        if (DeviceConfig::DEBUG_ENABLED) {
            if (now - m_lastCurrentLogMs >= 500) {
                m_lastCurrentLogMs = now;
                Serial.println(getCurrentAmps(), 2); 
            }
        }
    }
}

float Motor::getCurrentAmps() {
    return readCurrentSensor();
}

float Motor::readCurrentSensor() {
    // Делаем ровно ОДИН замер за тик (занимает ~10 мкс вместо ~100 мкс)
    uint16_t newSample = analogRead(BoardConfig::MOTOR1_CURR_SENS);

    // Обновляем кольцевой буфер (Moving Average)
    m_adcSum -= m_adcBuffer[m_adcIndex];
    m_adcBuffer[m_adcIndex] = newSample;
    m_adcSum += newSample;
    m_adcIndex = (m_adcIndex + 1) % ADC_SAMPLES_COUNT;

    float avgAdc = (float)m_adcSum / ADC_SAMPLES_COUNT;
    float voltageV = (avgAdc * 3.3f) / 4095.0f;
    float netVoltageV = voltageV - DeviceConfig::CURRENT_SENSOR_OFFSET_V;
    
    if (netVoltageV <= 0.0f) return 0.0f;

    return netVoltageV / DeviceConfig::CURRENT_SENSOR_SENSITIVITY;
}

void Motor::checkOvercurrent() {
    const uint32_t now = millis();
    float currentAmps = readCurrentSensor();

    bool isStarting = (now - m_moveStartMs < DeviceConfig::startCurrentTimeoutMs);
    float activeLimit = isStarting ? (DeviceConfig::maxMotorCurrentAmps * 1.5f) 
                                   : DeviceConfig::maxMotorCurrentAmps;

    if (currentAmps >= activeLimit) {
        if (m_overcurrentStartMs == 0) {
            m_overcurrentStartMs = now;
        } 
        else if (now - m_overcurrentStartMs >= DeviceConfig::overcurrentTimeoutMs) {
            stop(); // Корректная остановка мотора

            m_isEmergency = true;
            m_isOvercurrentFault = true;
            setOvercurrentLED(true); 

            char logBuf[128];
            snprintf(logBuf, sizeof(logBuf), "OVERCURRENT FAULT! Current: %.2fA (Limit: %.2fA)", 
                     currentAmps, activeLimit);
            Logger::error(logBuf);
        }
    } else {
        m_overcurrentStartMs = 0;
    }
}

void Motor::forward() { 
    if (m_targetState == MotorState::FORWARD) return;

    if (m_state == MotorState::REVERSE) {
        stop(); 
    }

    m_targetState = MotorState::FORWARD;
    Logger::debug("Motor target set to FORWARD");
}

void Motor::reverse() {   
    if (m_targetState == MotorState::REVERSE) return;

    if (m_state == MotorState::FORWARD) {
        stop(); 
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
    m_deadTimeStartMs = millis(); // Зафиксировали время для паузы (Dead Time)
    m_targetState = MotorState::STOPPED;
    m_state = MotorState::STOPPED;

    if (isEmergency()) {
        registerStopClick();
    }
    
    Logger::debug("Motor stop executed");
}

void Motor::registerStopClick() {
    uint32_t now = millis();
    m_stopClickTimes[m_stopClickIndex] = now;
    m_stopClickIndex = (m_stopClickIndex + 1) % 3;

    uint32_t oldestClick = m_stopClickTimes[m_stopClickIndex];

    if (oldestClick > 0 && (now - oldestClick <= 2000)) {
        Logger::warning("Motor: 3x STOP detected within 2 seconds! Processing fault recovery...");
        tryClearFault();
    }
}

bool Motor::tryClearFault() {
    // 1. Проверка аппаратного FAULT: если пин DIAG всё еще LOW, сбросить нельзя
    if (digitalRead(BoardConfig::MOTOR1_DIAG) == LOW) {
        Logger::error("CLEAR REJECTED: Driver Hardware Fault line is still active (DIAG LOW)!");
        return false;
    }
    
    // Если линия DIAG восстановилась в HIGH — снимаем флаг HW Fault
    s_hardwareFault.store(false, std::memory_order_relaxed);

    // 2. Сброс OVERCURRENT
    if (m_isOvercurrentFault) {
        uint32_t now = millis();

        if (m_firstOvercurrentMs == 0 || (now - m_firstOvercurrentMs > 60000)) {
            m_firstOvercurrentMs = now;
            m_overcurrentResetCount = 0;
        }

        if (m_overcurrentResetCount >= 3) {
            Logger::error("HARD LOCK: Exceeded 3 Overcurrent resets in 1 minute! Reboot required.");
            return false;
        }

        m_overcurrentResetCount++;
        m_isEmergency = false;
        m_isOvercurrentFault = false;
        setOvercurrentLED(false);

        memset(m_stopClickTimes, 0, sizeof(m_stopClickTimes));

        char logBuf[128];
        snprintf(logBuf, sizeof(logBuf), "OVERCURRENT cleared by 3x STOP (%d/3 resets in 60s).", m_overcurrentResetCount);
        Logger::info(logBuf);
        return true;
    }

    return false;
}

void IRAM_ATTR Motor::encoderISR() {
    if (digitalRead(BoardConfig::ENC_B) == HIGH) {
        s_encoderPosition.fetch_add(1, std::memory_order_relaxed);
    } else {
        s_encoderPosition.fetch_sub(1, std::memory_order_relaxed);
    }
}

int32_t Motor::getEncoderPosition() const {
    return s_encoderPosition.load(std::memory_order_relaxed);
}

void Motor::resetEncoder() {
    s_encoderPosition.store(0, std::memory_order_relaxed);
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

void Motor::setOvercurrentLED(bool enable) {
    digitalWrite(BoardConfig::IR_TRANSMITTER_PIN, enable ? HIGH : LOW);
}