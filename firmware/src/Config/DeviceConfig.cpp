#include "DeviceConfig.h"

namespace DeviceConfig {

void loadDefaults() {
    MOUNT_TYPE = Defaults::MOUNT_TYPE;
    IS_MASTER = Defaults::IS_MASTER;
    NODE_ID = Defaults::NODE_ID;
    MOTOR_SPEED = Defaults::MOTOR_SPEED;
    SOFT_START_MIN_PWM = Defaults::SOFT_START_MIN_PWM;
    SOFT_START_STEP_MS = Defaults::SOFT_START_STEP_MS;
    SOFT_START_STEP_PWM = Defaults::SOFT_START_STEP_PWM;
    CURRENT_SENSOR_SENSITIVITY = Defaults::CURRENT_SENSOR_SENSITIVITY;
    CURRENT_SENSOR_OFFSET_V = Defaults::CURRENT_SENSOR_OFFSET_V;
    startCurrentTimeoutMs = Defaults::START_CURRENT_TIMEOUT_MS;
    maxMotorCurrentAmps = Defaults::MAX_MOTOR_CURRENT_AMPS;
    overcurrentTimeoutMs = Defaults::OVERCURRENT_TIMEOUT_MS;
    MAX_FORWARD_TIME_MS = Defaults::MAX_FORWARD_TIME_MS;
    MAX_REVERSE_TIME_MS = Defaults::MAX_REVERSE_TIME_MS;
    FORWARD_LIMIT_RUN_ON_MS = Defaults::FORWARD_LIMIT_RUN_ON_MS;
    REVERSE_LIMIT_RUN_ON_MS = Defaults::REVERSE_LIMIT_RUN_ON_MS;
    MAX_LIFT_ENCODER_TICKS = Defaults::MAX_LIFT_ENCODER_TICKS;
    IR_CODE_UP = Defaults::IR_CODE_UP;
    IR_CODE_DOWN = Defaults::IR_CODE_DOWN;
    IR_CODE_STOP = Defaults::IR_CODE_STOP;
    IR_CODE_REPEAT = Defaults::IR_CODE_REPEAT;
    snprintf(otaUrl, sizeof(otaUrl), "%s", Defaults::OTA_URL);
    otaUpdateIntervalMs = Defaults::OTA_UPDATE_INTERVAL_MS;
}

void load() {
    Preferences prefs;
    prefs.begin("dev-config", true);

    MOUNT_TYPE = prefs.getUChar("MOUNT_TYPE", MOUNT_TYPE);
    IS_MASTER  = prefs.getBool("IS_MASTER", IS_MASTER);
    NODE_ID    = prefs.getUChar("NODE_ID", NODE_ID);

    MOTOR_SPEED        = prefs.getUChar("MOTOR_SPEED", MOTOR_SPEED);
    SOFT_START_MIN_PWM = prefs.getUChar("SS_MIN_PWM", SOFT_START_MIN_PWM);
    SOFT_START_STEP_MS = prefs.getULong("SS_STEP_MS", SOFT_START_STEP_MS);
    SOFT_START_STEP_PWM= prefs.getUChar("SS_STEP_PWM", SOFT_START_STEP_PWM);

    CURRENT_SENSOR_SENSITIVITY = prefs.getFloat("CUR_SENS", CURRENT_SENSOR_SENSITIVITY);
    CURRENT_SENSOR_OFFSET_V   = prefs.getFloat("CUR_OFF", CURRENT_SENSOR_OFFSET_V);
    startCurrentTimeoutMs     = prefs.getFloat("ST_CUR_TO", startCurrentTimeoutMs);
    maxMotorCurrentAmps       = prefs.getFloat("MAX_CUR_A", maxMotorCurrentAmps);
    overcurrentTimeoutMs      = prefs.getULong("OVER_TO_MS", overcurrentTimeoutMs);

    MAX_FORWARD_TIME_MS     = prefs.getULong("MAX_FWD_MS", MAX_FORWARD_TIME_MS);
    MAX_REVERSE_TIME_MS     = prefs.getULong("MAX_REV_MS", MAX_REVERSE_TIME_MS);
    FORWARD_LIMIT_RUN_ON_MS = prefs.getULong("FWD_RUN_ON", FORWARD_LIMIT_RUN_ON_MS);
    REVERSE_LIMIT_RUN_ON_MS = prefs.getULong("REV_RUN_ON", REVERSE_LIMIT_RUN_ON_MS);
    MAX_LIFT_ENCODER_TICKS  = prefs.getULong("MAX_ENCODER", MAX_LIFT_ENCODER_TICKS);

    IR_CODE_UP     = prefs.getULong("IR_UP", IR_CODE_UP);
    IR_CODE_DOWN   = prefs.getULong("IR_DOWN", IR_CODE_DOWN);
    IR_CODE_STOP   = prefs.getULong("IR_STOP", IR_CODE_STOP);
    IR_CODE_REPEAT = prefs.getULong("IR_REP", IR_CODE_REPEAT);

    String savedOta = prefs.getString("OTA_URL", otaUrl);
    snprintf(otaUrl, sizeof(otaUrl), "%s", savedOta.c_str());
    otaUpdateIntervalMs = prefs.getULong("OTA_INT", otaUpdateIntervalMs);

    prefs.end();
}

void save() {
    Preferences prefs;
    prefs.begin("dev-config", false);

    prefs.putUChar("MOUNT_TYPE", MOUNT_TYPE);
    prefs.putBool("IS_MASTER", IS_MASTER);
    prefs.putUChar("NODE_ID", NODE_ID);

    prefs.putUChar("MOTOR_SPEED", MOTOR_SPEED);
    prefs.putUChar("SS_MIN_PWM", SOFT_START_MIN_PWM);
    prefs.putULong("SS_STEP_MS", SOFT_START_STEP_MS);
    prefs.putUChar("SS_STEP_PWM", SOFT_START_STEP_PWM);

    prefs.putFloat("CUR_SENS", CURRENT_SENSOR_SENSITIVITY);
    prefs.putFloat("CUR_OFF", CURRENT_SENSOR_OFFSET_V);
    prefs.putFloat("ST_CUR_TO", startCurrentTimeoutMs);
    prefs.putFloat("MAX_CUR_A", maxMotorCurrentAmps);
    prefs.putULong("OVER_TO_MS", overcurrentTimeoutMs);

    prefs.putULong("MAX_FWD_MS", MAX_FORWARD_TIME_MS);
    prefs.putULong("MAX_REV_MS", MAX_REVERSE_TIME_MS);
    prefs.putULong("FWD_RUN_ON", FORWARD_LIMIT_RUN_ON_MS);
    prefs.putULong("REV_RUN_ON", REVERSE_LIMIT_RUN_ON_MS);
    prefs.putULong("MAX_ENCODER", MAX_LIFT_ENCODER_TICKS);

    prefs.putULong("IR_UP", IR_CODE_UP);
    prefs.putULong("IR_DOWN", IR_CODE_DOWN);
    prefs.putULong("IR_STOP", IR_CODE_STOP);
    prefs.putULong("IR_REP", IR_CODE_REPEAT);

    prefs.putString("OTA_URL", otaUrl);
    prefs.putULong("OTA_INT", otaUpdateIntervalMs);

    prefs.end();
}

}