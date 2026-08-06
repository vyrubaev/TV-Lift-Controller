#pragma once

#include "../core/Module.h"

class Logger : public Module
{
public:

    bool init() override;

    void loop() override;

    void shutdown() override;


    void info(const char* message);

    void error(const char* message);
};