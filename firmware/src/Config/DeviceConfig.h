#pragma once

#include <stdint.h>

namespace DeviceConfig
{
// Динамические константы (будут сохраняться в Flash/NVS):

// --- КОНФИГУРАЦИЯ УСТАНОВКИ ЛИФТА (ПЛАТЫ УПРАВЛЕНИЯ) ---    
constexpr u_int8_t MOUNT_TYPE = 1; // 0 = FLOOR, 1 = CEILING, 2 = WALL  Настройка типа лифта (по умолчанию CEILING, т.к. чаще всего используется потолочный вариант)

constexpr bool IS_MASTER = true; // true = master, false = slave

constexpr uint8_t NODE_ID = 1; 

// --- СКОРОСТЬ МОТОРА ---
constexpr uint8_t MOTOR_SPEED = 180; // range 0-255 

constexpr uint8_t  SOFT_START_MIN_PWM = 1;  // Минимальный ШИМ, при котором мотор начинает крутиться
constexpr uint32_t SOFT_START_STEP_MS = 100;  // Интервал увеличения ШИМ (мс)
constexpr uint8_t  SOFT_START_STEP_PWM = 5;  // Шаг прибавки ШИМ



// --- НАСТРОЙКИ ТОКА И ЗАЩИТЫ --- ПРИМЕР! (требуется проверка и калибровка под конкретный мотор и драйвер) !!!
// Коэффициент чувствительности датчика тока (Ампер на Вольт или мВ/А)
// Например, для ACS712-05B это 185 мВ/А (0.185 В/А)
constexpr float CURRENT_SENSOR_SENSITIVITY = 0.5f;  // Чувствительность датчика тока (Ампер на Вольт) - необходимо уточнить при калибровке под конкретный мотор и драйвер
constexpr float CURRENT_SENSOR_OFFSET_V = 0.0f;     // Напряжение при 0А 

inline float startCurrentTimeoutMs = 300; // Время превышения максимального тока при старте (мс)
inline float maxMotorCurrentAmps = 2.0f;       // Порог тока (Ампер)
inline uint32_t overcurrentTimeoutMs = 300;    // Время превышения до аварии (мс)

// Максимальное время работы мотора для каждого направления
constexpr uint32_t MAX_FORWARD_TIME_MS = 30000; // 30 секунд на подъем (вперед) (мс)
constexpr uint32_t MAX_REVERSE_TIME_MS = 30000; // 30 секунд на спуск (назад) (мс)

// Время добега после срабатывания концевика в мсек. 0 = остановить мотор сразу.
constexpr uint32_t FORWARD_LIMIT_RUN_ON_MS = 1000; 
constexpr uint32_t REVERSE_LIMIT_RUN_ON_MS = 1000; 
// Конфигурация счетчика оборотов мотора 
constexpr uint32_t MAX_LIFT_ENCODER_TICKS = 0; // 


// Настройки управления лифтом инфракрасными пультом
// HEX-коды вашей трехкнопочной системы (замените на коды вашего пульта после первого запуска)
constexpr uint32_t IR_CODE_UP     = 0x11EEA857; 
constexpr uint32_t IR_CODE_DOWN   = 0x11EE6897; 
constexpr uint32_t IR_CODE_STOP   = 0x11EE9867; 
constexpr uint32_t IR_CODE_REPEAT = 0xFFFFFFFF; // Код зажатия/повтора кнопки


// Адрес для обновления прошивки OTA 
 
constexpr char otaUrl[128] = "http://192.168.88.33:3000/firmware/version.json";
}

