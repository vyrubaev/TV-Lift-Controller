#include "Logger.h"

#include <Arduino.h>


bool Logger::init()
{
    Serial.begin(115200);

    Serial.println();

    Serial.println("Logger initialized");

    return true;
}


void Logger::loop()
{

}


void Logger::shutdown()
{

}


void Logger::info(const char* message)
{
    Serial.print("[INFO] ");

    Serial.println(message);
}


void Logger::error(const char* message)
{
    Serial.print("[ERROR] ");

    Serial.println(message);
}