#pragma once

#include <stdint.h>
#include <Preferences.h>

namespace DeviceConfig
{
// Версия прошивки
inline const char* VERSION = "1.0.20"; // Обновите версию при каждом изменении прошивки
    
// Единый источник дефолтных значений для сброса и инициализации
namespace Defaults {
    // --- КОНФИГУРАЦИЯ УСТАНОВКИ ЛИФТА (ПЛАТЫ УПРАВЛЕНИЯ) ---    
    constexpr uint8_t MOUNT_TYPE = 1; // 0 = FLOOR, 1 = CEILING, 2 = WALL  Настройка типа лифта (по умолчанию CEILING, т.к. чаще всего используется потолочный вариант)
    constexpr bool IS_MASTER = true; // true = master, false = slave
    constexpr uint8_t NODE_ID = 1; // Идентификатор узла (для master/slave конфигурации, по умолчанию 1) для мультимоторных систем, где несколько плат управляют разными моторами лифта. Каждый узел должен иметь уникальный идентификатор (1, 2, 3 и т.д.).

    // --- СКОРОСТЬ МОТОРА ---
    constexpr uint8_t MOTOR_SPEED = 180; // range 0-255 
    constexpr uint8_t SOFT_START_MIN_PWM = 1;  // Минимальный ШИМ, при котором мотор начинает крутиться
    constexpr uint32_t SOFT_START_STEP_MS = 100;  // Интервал увеличения ШИМ (мс)
    constexpr uint8_t SOFT_START_STEP_PWM = 5;  // Шаг прибавки ШИМ

    // --- НАСТРОЙКИ ТОКА И ЗАЩИТЫ --- ПРИМЕР! (требуется проверка и калибровка под конкретный мотор и драйвер) !!!
    constexpr float CURRENT_SENSOR_SENSITIVITY = 0.5f;  // Чувствительность датчика тока (Ампер на Вольт) - необходимо уточнить при калибровке под конкретный мотор и драйвер
    constexpr float CURRENT_SENSOR_OFFSET_V = 0.0f;     // Напряжение при 0А 
    constexpr float START_CURRENT_TIMEOUT_MS = 300; // Время превышения максимального тока при старте (мс)
    constexpr float MAX_MOTOR_CURRENT_AMPS = 2.0f;       // Порог тока (Ампер)
    constexpr uint32_t OVERCURRENT_TIMEOUT_MS = 300;    // Время превышения до аварии (мс)

    // Максимальное время работы мотора для каждого направления
    constexpr uint32_t MAX_FORWARD_TIME_MS = 30000; // 30 секунд на подъем (вперед) (мс)
    constexpr uint32_t MAX_REVERSE_TIME_MS = 30000; // 30 секунд на спуск (назад) (мс)

    // Время добега после срабатывания концевика в мсек. 0 = остановить мотор сразу.
    constexpr uint32_t FORWARD_LIMIT_RUN_ON_MS = 1000; 
    constexpr uint32_t REVERSE_LIMIT_RUN_ON_MS = 1000; 
    
    // Конфигурация счетчика оборотов мотора 
    constexpr uint32_t MAX_LIFT_ENCODER_TICKS = 0; // 

    // Настройки управления лифтом инфракрасными пультом
    constexpr uint32_t IR_CODE_UP     = 0x11EEA857; 
    constexpr uint32_t IR_CODE_DOWN   = 0x11EE6897; 
    constexpr uint32_t IR_CODE_STOP   = 0x11EE9867; 
    constexpr uint32_t IR_CODE_REPEAT = 0xFFFFFFFF; // Код зажатия/повтора кнопки

    // Адрес для обновления прошивки OTA 
    const char OTA_URL[] = "http://192.168.88.33:3000/firmware/version.json"; // URL для проверки обновлений прошивки (можно указать локальный сервер или внешний URL)
    constexpr uint32_t OTA_UPDATE_INTERVAL_MS = 5000; // Интервал проверки обновлений (мс) 
}

// Рабочие переменные (используются по всему коду как DeviceConfig::...)
inline uint8_t MOUNT_TYPE = Defaults::MOUNT_TYPE;
inline bool IS_MASTER = Defaults::IS_MASTER;
inline uint8_t NODE_ID = Defaults::NODE_ID;
inline uint8_t MOTOR_SPEED = Defaults::MOTOR_SPEED;
inline uint8_t SOFT_START_MIN_PWM = Defaults::SOFT_START_MIN_PWM;
inline uint32_t SOFT_START_STEP_MS = Defaults::SOFT_START_STEP_MS;
inline uint8_t SOFT_START_STEP_PWM = Defaults::SOFT_START_STEP_PWM;
inline float CURRENT_SENSOR_SENSITIVITY = Defaults::CURRENT_SENSOR_SENSITIVITY;
inline float CURRENT_SENSOR_OFFSET_V = Defaults::CURRENT_SENSOR_OFFSET_V;
inline float startCurrentTimeoutMs = Defaults::START_CURRENT_TIMEOUT_MS;
inline float maxMotorCurrentAmps = Defaults::MAX_MOTOR_CURRENT_AMPS;
inline uint32_t overcurrentTimeoutMs = Defaults::OVERCURRENT_TIMEOUT_MS;
inline uint32_t MAX_FORWARD_TIME_MS = Defaults::MAX_FORWARD_TIME_MS;
inline uint32_t MAX_REVERSE_TIME_MS = Defaults::MAX_REVERSE_TIME_MS;
inline uint32_t FORWARD_LIMIT_RUN_ON_MS = Defaults::FORWARD_LIMIT_RUN_ON_MS;
inline uint32_t REVERSE_LIMIT_RUN_ON_MS = Defaults::REVERSE_LIMIT_RUN_ON_MS;
inline uint32_t MAX_LIFT_ENCODER_TICKS = Defaults::MAX_LIFT_ENCODER_TICKS;
inline uint32_t IR_CODE_UP = Defaults::IR_CODE_UP;
inline uint32_t IR_CODE_DOWN = Defaults::IR_CODE_DOWN;
inline uint32_t IR_CODE_STOP = Defaults::IR_CODE_STOP;
inline uint32_t IR_CODE_REPEAT = Defaults::IR_CODE_REPEAT;
inline char otaUrl[128] = "http://192.168.88.33:3000/firmware/version.json";
inline uint32_t otaUpdateIntervalMs = Defaults::OTA_UPDATE_INTERVAL_MS;

// Прототипы функций
void loadDefaults();
void load();
void save();
}