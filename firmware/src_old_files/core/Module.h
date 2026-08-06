#pragma once

class Module
{
public:

    virtual ~Module() = default;

    virtual bool init() = 0;

    virtual void loop() = 0;

    virtual void shutdown() = 0;
};