#pragma once

#include <Arduino.h>

class Logger
{
public:

    void init();

    void info(const char* message);
    void warning(const char* message);
    void error(const char* message);
};