#include "Core.h"

#include "../Logger/Logger.h"


bool Core::init()
{
    Logger::init();

    Logger::info("Core initialization");

    return true;
}



void Core::loop()
{

}



void Core::reboot()
{

}