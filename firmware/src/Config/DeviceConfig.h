#pragma once

#include <stdint.h>
#include <Preferences.h>

namespace DeviceConfig
{
// Версия прошивки
inline const char* VERSION = "1.0.15"; // Обновите версию при каждом изменении прошивки
    
// Динамические константы (будут сохраняться в Flash/NVS):

// --- КОНФИГУРАЦИЯ УСТАНОВКИ ЛИФТА (ПЛАТЫ УПРАВЛЕНИЯ) ---    
inline uint8_t MOUNT_TYPE = 1; // 0 = FLOOR, 1 = CEILING, 2 = WALL  Настройка типа лифта (по умолчанию CEILING, т.к. чаще всего используется потолочный вариант)

inline bool IS_MASTER = true; // true = master, false = slave

inline uint8_t NODE_ID = 1; // Идентификатор узла (для master/slave конфигурации, по умолчанию 1) для мультимоторных систем, где несколько плат управляют разными моторами лифта. Каждый узел должен иметь уникальный идентификатор (1, 2, 3 и т.д.).

// --- СКОРОСТЬ МОТОРА ---
inline uint8_t MOTOR_SPEED = 180; // range 0-255 

inline uint8_t  SOFT_START_MIN_PWM = 1;  // Минимальный ШИМ, при котором мотор начинает крутиться
inline uint32_t SOFT_START_STEP_MS = 100;  // Интервал увеличения ШИМ (мс)
inline uint8_t  SOFT_START_STEP_PWM = 5;  // Шаг прибавки ШИМ

// --- НАСТРОЙКИ ТОКА И ЗАЩИТЫ --- ПРИМЕР! (требуется проверка и калибровка под конкретный мотор и драйвер) !!!
// Коэффициент чувствительности датчика тока (Ампер на Вольт или мВ/А)
// Например, для ACS712-05B это 185 мВ/А (0.185 В/А)
inline float CURRENT_SENSOR_SENSITIVITY = 0.5f;  // Чувствительность датчика тока (Ампер на Вольт) - необходимо уточнить при калибровке под конкретный мотор и драйвер
inline float CURRENT_SENSOR_OFFSET_V = 0.0f;     // Напряжение при 0А 

inline float startCurrentTimeoutMs = 300; // Время превышения максимального тока при старте (мс)
inline float maxMotorCurrentAmps = 2.0f;       // Порог тока (Ампер)
inline uint32_t overcurrentTimeoutMs = 300;    // Время превышения до аварии (мс)

// Максимальное время работы мотора для каждого направления
inline uint32_t MAX_FORWARD_TIME_MS = 30000; // 30 секунд на подъем (вперед) (мс)
inline uint32_t MAX_REVERSE_TIME_MS= 30000; // 30 секунд на спуск (назад) (мс)

// Время добега после срабатывания концевика в мсек. 0 = остановить мотор сразу.
inline uint32_t FORWARD_LIMIT_RUN_ON_MS = 1000; 
inline uint32_t REVERSE_LIMIT_RUN_ON_MS = 1000; 
// Конфигурация счетчика оборотов мотора 
inline uint32_t MAX_LIFT_ENCODER_TICKS = 0; // 

// Настройки управления лифтом инфракрасными пультом
// HEX-коды вашей трехкнопочной системы (замените на коды вашего пульта после первого запуска)
inline uint32_t IR_CODE_UP     = 0x11EEA857; 
inline uint32_t IR_CODE_DOWN   = 0x11EE6897; 
inline uint32_t IR_CODE_STOP   = 0x11EE9867; 
inline uint32_t IR_CODE_REPEAT = 0xFFFFFFFF; // Код зажатия/повтора кнопки

// Адрес для обновления прошивки OTA 
inline char otaUrl[128] = "http://192.168.88.33:3000/firmware/version.json"; // URL для проверки обновлений прошивки (можно указать локальный сервер или внешний URL)
inline uint32_t otaUpdateIntervalMs = 5000; // Интервал проверки обновлений (мс) 


// --- ФУНКЦИИ РАБОТЫ С NVS ПАМЯТЬЮ ---

inline void load() {
    Preferences prefs;
    prefs.begin("dev-config", true);

    MOUNT_TYPE = prefs.getUChar("MOUNT_TYPE", MOUNT_TYPE);
    IS_MASTER  = prefs.getBool("IS_MASTER", IS_MASTER);
    NODE_ID    = prefs.getUChar("NODE_ID", NODE_ID);

    MOTOR_SPEED        = prefs.getUChar("MOTOR_SPEED", MOTOR_SPEED);
    SOFT_START_MIN_PWM = prefs.getUChar("SS_MIN_PWM", SOFT_START_MIN_PWM);
    SOFT_START_STEP_MS = prefs.getULong("SS_STEP_MS", SOFT_START_STEP_MS); // ULong для uint32_t
    SOFT_START_STEP_PWM= prefs.getUChar("SS_STEP_PWM", SOFT_START_STEP_PWM);

    CURRENT_SENSOR_SENSITIVITY = prefs.getFloat("CUR_SENS", CURRENT_SENSOR_SENSITIVITY);
    CURRENT_SENSOR_OFFSET_V   = prefs.getFloat("CUR_OFF", CURRENT_SENSOR_OFFSET_V);
    startCurrentTimeoutMs     = prefs.getFloat("ST_CUR_TO", startCurrentTimeoutMs);
    maxMotorCurrentAmps       = prefs.getFloat("MAX_CUR_A", maxMotorCurrentAmps);
    overcurrentTimeoutMs      = prefs.getULong("OVER_TO_MS", overcurrentTimeoutMs); // ULong

    MAX_FORWARD_TIME_MS     = prefs.getULong("MAX_FWD_MS", MAX_FORWARD_TIME_MS);   // ULong
    MAX_REVERSE_TIME_MS     = prefs.getULong("MAX_REV_MS", MAX_REVERSE_TIME_MS);   // ULong
    FORWARD_LIMIT_RUN_ON_MS = prefs.getULong("FWD_RUN_ON", FORWARD_LIMIT_RUN_ON_MS); // ULong
    REVERSE_LIMIT_RUN_ON_MS = prefs.getULong("REV_RUN_ON", REVERSE_LIMIT_RUN_ON_MS); // ULong
    MAX_LIFT_ENCODER_TICKS  = prefs.getULong("MAX_ENCODER", MAX_LIFT_ENCODER_TICKS); // ULong

    IR_CODE_UP     = prefs.getULong("IR_UP", IR_CODE_UP);     // ULong
    IR_CODE_DOWN   = prefs.getULong("IR_DOWN", IR_CODE_DOWN); // ULong
    IR_CODE_STOP   = prefs.getULong("IR_STOP", IR_CODE_STOP); // ULong
    IR_CODE_REPEAT = prefs.getULong("IR_REP", IR_CODE_REPEAT); // ULong

    String savedOta = prefs.getString("OTA_URL", otaUrl);
    snprintf(otaUrl, sizeof(otaUrl), "%s", savedOta.c_str());
    otaUpdateIntervalMs = prefs.getULong("OTA_INT", otaUpdateIntervalMs); // ULong

    prefs.end();
}

inline void save() {
    Preferences prefs;
    prefs.begin("dev-config", false);

    prefs.putUChar("MOUNT_TYPE", MOUNT_TYPE);
    prefs.putBool("IS_MASTER", IS_MASTER);
    prefs.putUChar("NODE_ID", NODE_ID);

    prefs.putUChar("MOTOR_SPEED", MOTOR_SPEED);
    prefs.putUChar("SS_MIN_PWM", SOFT_START_MIN_PWM);
    prefs.putULong("SS_STEP_MS", SOFT_START_STEP_MS); // putULong
    prefs.putUChar("SS_STEP_PWM", SOFT_START_STEP_PWM);

    prefs.putFloat("CUR_SENS", CURRENT_SENSOR_SENSITIVITY);
    prefs.putFloat("CUR_OFF", CURRENT_SENSOR_OFFSET_V);
    prefs.putFloat("ST_CUR_TO", startCurrentTimeoutMs);
    prefs.putFloat("MAX_CUR_A", maxMotorCurrentAmps);
    prefs.putULong("OVER_TO_MS", overcurrentTimeoutMs); // putULong

    prefs.putULong("MAX_FWD_MS", MAX_FORWARD_TIME_MS);   // putULong
    prefs.putULong("MAX_REV_MS", MAX_REVERSE_TIME_MS);   // putULong
    prefs.putULong("FWD_RUN_ON", FORWARD_LIMIT_RUN_ON_MS); // putULong
    prefs.putULong("REV_RUN_ON", REVERSE_LIMIT_RUN_ON_MS); // putULong
    prefs.putULong("MAX_ENCODER", MAX_LIFT_ENCODER_TICKS); // putULong

    prefs.putULong("IR_UP", IR_CODE_UP);     // putULong
    prefs.putULong("IR_DOWN", IR_CODE_DOWN); // putULong
    prefs.putULong("IR_STOP", IR_CODE_STOP); // putULong
    prefs.putULong("IR_REP", IR_CODE_REPEAT); // putULong

    prefs.putString("OTA_URL", otaUrl);
    prefs.putULong("OTA_INT", otaUpdateIntervalMs); // putULong

    prefs.end();
}
}