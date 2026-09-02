#pragma once

#include <Arduino.h>

class Logger
{
public:
    static void init();
    static void info(const char* message);
    static void warning(const char* message);
    static void error(const char* message);
    static void debug(const char* message);
};