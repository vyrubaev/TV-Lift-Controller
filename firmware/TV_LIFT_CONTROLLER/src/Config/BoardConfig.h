#pragma once

#include <stdint.h>

namespace BoardConfig
{
    // Board name and revision
    constexpr char BOARD_NAME[] = "TV Lift Controller";
    constexpr char BOARD_REVISION[] = "REV_A";

    // Частота процессора ESP32
    constexpr uint32_t CPU_FREQUENCY_MHZ = 240;

    // -------------------------
    // Motor 1
    // -------------------------

    constexpr uint8_t MOTOR1_INA = 16;
    constexpr uint8_t MOTOR1_INB = 17;
    constexpr uint8_t MOTOR1_PWM= 12; // WARNING - This pin is used by the ESP32 for bootstrapping. Using it may cause issues during boot.
    constexpr uint8_t MOTOR1_DIAG = 33; // пин DIAG с драйвера мотора, который сигнализирует о неисправности драйвера
    constexpr uint8_t MOTOR1_CURR_SENS = 32; //пин для измерения тока мотора с выхода драйвера мотора

    constexpr uint8_t MOTOR_DEFAULT_SPEED = 180; // range 0-255
    

    // -------------------------
    // ENCODERS
    // -------------------------

    constexpr uint8_t ENC_A = 36; // Physical pin 4 of ESP32-WROOM-32D
    constexpr uint8_t ENC_B = 39; // Physical pin 5 of ESP32-WROOM-32D


    // -------------------------
    // Dry contacts
    // -------------------------

    constexpr uint8_t DRY_CONTACT_DOWN_PIN = 34; 
    constexpr uint8_t DRY_CONTACT_UP_PIN = 35;

    // -------------------------
    // Limit switches
    // -------------------------

    constexpr uint8_t LIMIT_SWITCH_DOWN_PIN = 13;
    constexpr uint8_t LIMIT_SWITCH_UP_PIN = 4;

    // -------------------------
    // Communication
    // -------------------------

    constexpr uint8_t UART_TX_PIN = 1;
    constexpr uint8_t UART_RX_PIN = 3;

    constexpr uint8_t CAN_TX_PIN = 5; // WARNING - This pin is used by the ESP32 for bootstrapping. Using it may cause issues during boot.
    constexpr uint8_t CAN_RX_PIN = 15; // WARNING - This pin is used by the ESP32 for bootstrapping. Using it may cause issues during boot.

    constexpr uint8_t IR_RECEIVER_PIN = 14;
    constexpr uint8_t IR_TRANSMITTER_PIN = 2;

    // -------------------------
    // Ethernet
    // -------------------------

    constexpr uint8_t ETH_RETCLK = 0;
    constexpr uint8_t ETH_MDC = 23;
    constexpr uint8_t ETH_MDIO = 18;
    constexpr uint8_t ETH_RX1 = 26;
    constexpr uint8_t ETH_TX1 = 22;
    constexpr uint8_t ETH_CRS_DV = 27;
    constexpr uint8_t ETH_TX_EN = 21;
    constexpr uint8_t ETH_RX0 = 25;
    constexpr uint8_t ETH_TX0 = 19;

}