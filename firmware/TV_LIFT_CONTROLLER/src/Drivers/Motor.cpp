#include "Motor.h"
#include <Arduino.h>
#include <string>
#include "../Config/BoardConfig.h"
#include "../Config/DeviceConfig.h"
#include "Logger/Logger.h" // тут он для тестов, в будущем будет удален
#include <soc/gpio_struct.h>


std::atomic<bool> Motor::s_isEmergency{false}; // Инициализация статического атомарного флага

Motor::Motor()
    : m_speed(DeviceConfig::MOTOR_SPEED),
      m_state(MotorState::STOPPED)
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

    pinMode(
        BoardConfig::MOTOR1_DIAG,
        INPUT);


    if (digitalRead(BoardConfig::MOTOR1_DIAG) == LOW) // Если пин DIAG уже в LOW при старте — значит мотор ушел в аварию
    {
        emergencyStopFromISR(); // Устанавливаем флаг аварии и останавливаем мотор
    }
    
    // Настраиваем прерывание на пине DIAG для обработки аварийной остановки
    attachInterrupt(
        digitalPinToInterrupt(BoardConfig::MOTOR1_DIAG),
        Motor::emergencyStopFromISR,
        FALLING
    );

    // Настройка АЦП ESP32 для датчика тока
    pinMode(BoardConfig::MOTOR1_CURR_SENS, INPUT);
    analogReadResolution(12); // 12 бит (0..4095)

}

void Motor::update() {
    // Если мотор движется, проверяем ток
    if (m_state == MotorState::FORWARD || m_state == MotorState::REVERSE) {
        checkOvercurrent();
    }
}

float Motor::readCurrentSensor() { // Считываем ток с датчика тока через АЦП - необходимо уточнить при калибровке под конкретный мотор и драйвер
    // Считываем АЦП (фильтрация усреднением из 5 измерений для стабильности)
    uint32_t rawSum = 0;
    for (int i = 0; i < 5; i++) {
        rawSum += analogRead(BoardConfig::MOTOR1_CURR_SENS);
    }
    float rawAvg = rawSum / 5.0f; // Среднее значение АЦП

    // Перевод из попугаев АЦП (0..4095) в Вольты (0..3.3V)
    float voltage = (rawAvg / 4095.0f) * 3.3f;

    // Перевод Вольт в Амперы с учетом смещения и чувствительности
    float current = (voltage - DeviceConfig::CURRENT_SENSOR_OFFSET_V) / DeviceConfig::CURRENT_SENSOR_SENSITIVITY;


    // код для отладки и проверки работы датчика тока
    //std::string logMessageRAW = "Raw data sensor reading: " + std::to_string(rawAvg);
    std::string logMessageVolt = "Voltage aproximate: " + std::to_string(voltage) + " V";
    std::string logMessage = "Current sensor reading: " + std::to_string(current) + " A";
    //Logger::debug(logMessageRAW.c_str()); // Выводим в лог для отладки
    Logger::debug(logMessageVolt.c_str()); // Выводим в лог для отладки
    Logger::debug(logMessage.c_str()); // Выводим в лог для отладки
    Logger::debug("--------------------------------------------------"); // Разделитель для удобства чтения лога
    // конец кода для отладки и проверки работы датчика тока
    return abs(current); // Возвращаем абсолютное значение тока
}

float Motor::getCurrentAmps() {
    return readCurrentSensor();
}

void Motor::checkOvercurrent() {
    float currentAmps = readCurrentSensor();

    if (currentAmps >= DeviceConfig::maxMotorCurrentAmps) {
        // Если это первое превышение — запоминаем время
        if (m_overcurrentStartMs == 0) {
            m_overcurrentStartMs = millis();
        } 
        // Если ток превышен непрерывно дольше лимита — ВЫЗЫВАЕМ АВАРИЮ
        else if (millis() - m_overcurrentStartMs >= DeviceConfig::overcurrentTimeoutMs) {
            stop();
            m_state = MotorState::OVERCURRENT; // Устанавливаем состояние аварии по току
            
            // Включаем светодиод аварии на том же пине!
            setFaultLED(true); 

            Logger::error("OVERCURRENT FAULT: Motor current exceeded limit for too long!");
        }
    } else {
        // Ток упал ниже порога — сбрасываем таймер (пусковой ток успешно пройден)
        m_overcurrentStartMs = 0;
    }
}

void Motor::setFaultLED(bool enable) {
    if (enable) {
        // Переключаем пин на ВЫХОД и подаем LOW (зажигаем LED)
        pinMode(BoardConfig::MOTOR1_DIAG, OUTPUT);
        digitalWrite(BoardConfig::MOTOR1_DIAG, LOW);
    } else {
        // Возвращаем пин в режим ВХОДА для чтения аварий драйвера
        pinMode(BoardConfig::MOTOR1_DIAG, INPUT_PULLUP);
    }
}

void Motor::clearFault() { // не уверен что это нужно, но пусть будет для отладки
    setFaultLED(false); // Гасим LED и переводим пин обратно в INPUT_PULLUP
    m_state = MotorState::STOPPED;
    m_overcurrentStartMs = 0;
    Logger::info("OVERCURRENT cleared."); // не уверен что это нужно, но пусть будет для отладки
}

void Motor::reverse()
{
    if(m_state == MotorState::REVERSE)
    {
        return;
    }

    stop();

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

    ledcWrite(
        BoardConfig::MOTOR1_PWM,
        0);

    digitalWrite(
        BoardConfig::MOTOR1_INA,
        LOW);

    digitalWrite(
        BoardConfig::MOTOR1_INB,
        LOW);
    
    // Если мотор ушел в аварию — фиксируем состояние EMERGENCY_STOP, иначе STOPPED
    if (s_isEmergency.load(std::memory_order_relaxed))
        {
            m_state = MotorState::EMERGENCY_STOP;
        }
    else
        {
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

    s_isEmergency.store(true, std::memory_order_relaxed);    // 2. ФЛАГ: Уведомляем систему о событии

}

bool Motor::isEmergency() const
{
    return s_isEmergency.load(std::memory_order_relaxed);
}

void Motor::clearEmergency()
{
    s_isEmergency.store(false, std::memory_order_relaxed);
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

