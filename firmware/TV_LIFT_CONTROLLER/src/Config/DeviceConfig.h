#pragma once

#include <stdint.h>

namespace DeviceConfig
{
// --- КОНФИГУРАЦИЯ УСТАНОВКИ ЛИФТА (ПЛАТЫ УПРАВЛЕНИЯ) ---    
constexpr u_int8_t MOUNT_TYPE = 1; // 0 = FLOOR, 1 = CEILING, 2 = WALL  Настройка типа лифта (по умолчанию CEILING, т.к. чаще всего используется потолочный вариант)

constexpr bool IS_MASTER = true; // true = master, false = slave

constexpr uint8_t NODE_ID = 1; 

// --- СКОРОСТЬ МОТОРА ---
constexpr uint8_t MOTOR_SPEED = 180; // range 0-255 

// --- НАСТРОЙКИ ТОКА И ЗАЩИТЫ --- ПРИМЕР! (требуется проверка и калибровка под конкретный мотор и драйвер) !!!
// Коэффициент чувствительности датчика тока (Ампер на Вольт или мВ/А)
// Например, для ACS712-05B это 185 мВ/А (0.185 В/А)
constexpr float CURRENT_SENSOR_SENSITIVITY = 0.5f;  // Чувствительность датчика тока (Ампер на Вольт) - необходимо уточнить при калибровке под конкретный мотор и драйвер
constexpr float CURRENT_SENSOR_OFFSET_V = 0.0f;     // Напряжение при 0А 

// Динамические константы (будут сохраняться в Flash/NVS):
inline float startCurrentTimeoutMs = 300; // Время превышения тока при старте (мс)
inline float maxMotorCurrentAmps = 2.0f;       // Порог тока (Ампер)
inline uint32_t overcurrentTimeoutMs = 300;    // Время превышения до аварии (мс)

// Максимальное время работы мотора для каждого направления
constexpr uint32_t MAX_FORWARD_TIME_MS = 7000; // 5 секунд на подъем (вперед)
constexpr uint32_t MAX_REVERSE_TIME_MS = 4000; // 20 секунд на спуск (назад)

// Время добега после срабатывания концевика в мсек. 0 = остановить мотор сразу.
constexpr uint32_t FORWARD_LIMIT_RUN_ON_MS = 1000; 
constexpr uint32_t REVERSE_LIMIT_RUN_ON_MS = 2000; 


// Настройки управления лифтом инфракрасными пультом
// HEX-коды вашей трехкнопочной системы (замените на коды вашего пульта после первого запуска)
constexpr uint32_t IR_CODE_UP     = 0x11EEA857; 
constexpr uint32_t IR_CODE_DOWN   = 0x11EE6897; 
constexpr uint32_t IR_CODE_STOP   = 0x11EE9867; 
constexpr uint32_t IR_CODE_REPEAT = 0xFFFFFFFF; // Код зажатия/повтора кнопки
}

