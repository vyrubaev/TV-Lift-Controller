#pragma once

#include <stdint.h>

namespace DeviceConfig
{

constexpr bool IS_MASTER = true; // true = master, false = slave

constexpr uint8_t NODE_ID = 1; 

constexpr uint8_t MOTOR_SPEED = 180; // range 0-255 


constexpr uint32_t FORWARD_LIMIT_RUN_ON_MS = 0; // Время добега после срабатывания концевика в мсек. 0 = остановить мотор сразу.
constexpr uint32_t REVERSE_LIMIT_RUN_ON_MS = 2000; // ремя добега после срабатывания концевика в мсек. 0 = остановить мотор сразу.

// --- НАСТРОЙКИ ТОКА И ЗАЩИТЫ --- ПРИМЕР! (требуется проверка и калибровка под конкретный мотор и драйвер) !!!
// Коэффициент чувствительности датчика тока (Ампер на Вольт или мВ/А)
// Например, для ACS712-05B это 185 мВ/А (0.185 В/А)
constexpr float CURRENT_SENSOR_SENSITIVITY = 0.185f; 
constexpr float CURRENT_SENSOR_OFFSET_V = 1.65f;     // Напряжение при 0А (половина 3.3V)

// Динамические константы (будут сохраняться в Flash/NVS):
inline float maxMotorCurrentAmps = 4.5f;       // Порог тока (Ампер)
inline uint32_t overcurrentTimeoutMs = 300;    // Время превышения до аварии (мс)

}

