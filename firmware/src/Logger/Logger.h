#pragma once

class Logger {
public:
    static void init();
    static void info(const char* message);
    static void warning(const char* message);
    static void error(const char* message);
    static void debug(const char* message);
    
    // Метод для проверки, включен ли дебаг (пригодится для мотора)
    //static bool isDebugEnabled() { return DeviceConfig::DEBUG_ENABLED; }

private:
    //static bool m_debugEnabled; // Просто объявляем переменную, не инициализируя её здесь
};
