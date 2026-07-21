                  +----------------------+
                  |      Application     |
                  |  (логика устройства) |
                  +----------+-----------+
                             |
      +----------+-----------+-----------+----------+
      |          |           |           |          |
+-----v----+ +---v----+ +----v----+ +----v----+ +---v----+
| Elevator | |Network | | Storage | |   OTA   | | Logger |
+----------+ +--------+ +---------+ +---------+ +--------+
      |           |           |           |
      +-----------+-----------+-----------+
                      |
              +-------v--------+
              |      HAL       |
              | Hardware Layer |
              +-------+--------+
                      |
         +------------+-------------+
         | GPIO I²C SPI UART CAN IR |
         +--------------------------+

---

Полная карта модулей прошивки V.1

Application
│
├── System
│   ├── BootManager
│   ├── TaskManager
│   ├── Watchdog
│   ├── TimeService
│   └── EventBus
│
├── Configuration
│   ├── ConfigManager
│   ├── DeviceConfig
│   ├── NetworkConfig
│   └── UserConfig
│
├── Storage
│   ├── NVS
│   ├── LittleFS
│   └── Backup
│
├── Network
│   ├── NetworkManager
│   ├── WiFiManager
│   ├── EthernetManager
│   ├── DNS
│   ├── DHCP
│   ├── HTTP Client
│   ├── HTTPS Client
│   ├── WebServer
│   └── CaptivePortal
│
├── OTA
│   ├── UpdateManager
│   ├── FirmwareDownloader
│   ├── FirmwareVerifier
│   └── RollbackManager
│
├── Security
│   ├── TLS
│   ├── Certificates
│   ├── DeviceIdentity
│   └── Authentication
│
├── API
│   ├── REST API
│   ├── JSON
│   └── API Dispatcher
│
├── Logger
│   ├── Console
│   ├── FlashLog
│   └── RemoteLog
│
├── Diagnostics
│   ├── SelfTest
│   ├── Statistics
│   ├── MemoryMonitor
│   └── CrashReporter
│
├── Elevator
│   ├── LiftController
│   ├── MotorController
│   ├── SafetyController
│   ├── Calibration
│   ├── Inputs
│   ├── Outputs
│   ├── Position
│   └── Limits
│
└── HAL
    ├── GPIO
    ├── ADC
    ├── PWM
    ├── Timers
    ├── UART
    ├── SPI
    ├── I2C
    ├── CAN
    ├── IR
    ├── Ethernet
    ├── WiFi
    ├── Flash
    └── RTC

    ---

    Полная карта модулей прошивки V.2

    Application
│
├── Core
│
├── Config
│
├── Network
│
├── OTA
│
├── API
│
├── Logger
│
├── Elevator
│
├── Drivers
│
└── HAL