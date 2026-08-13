#pragma once

#include <stdint.h>

namespace DeviceConfig
{

constexpr bool IS_MASTER = true;

constexpr uint8_t NODE_ID = 1;

constexpr uint8_t MOTOR_SPEED = 180; // range 0-255 


constexpr uint32_t FORWARD_LIMIT_RUN_ON_MS = 0; // Время добега после срабатывания концевика в мсек. 0 = остановить мотор сразу.
constexpr uint32_t REVERSE_LIMIT_RUN_ON_MS = 2000; // 

}

