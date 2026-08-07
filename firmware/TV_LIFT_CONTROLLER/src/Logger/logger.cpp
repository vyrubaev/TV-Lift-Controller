#include "Logger.h"

void Logger::init()
{
    Serial.begin(115200);

    delay(100);

    Serial.println();
    Serial.println("================================");
    Serial.println("TV Lift Controller");
    Serial.println("Logger initialized");
    Serial.println("================================");
}

void Logger::info(const char* message)
{
    Serial.print("[INFO] ");
    Serial.println(message);
}

void Logger::warning(const char* message)
{
    Serial.print("[WARNING] ");
    Serial.println(message);
}

void Logger::error(const char* message)
{
    Serial.print("[ERROR] ");
    Serial.println(message);
}