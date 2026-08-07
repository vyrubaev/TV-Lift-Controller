#include <Arduino.h>

#include "Logger/Logger.h"

Logger logger;

void setup()
{
    logger.init();

    logger.info("System started");
    logger.warning("This is a warning");
    logger.error("This is an error");
}

void loop()
{
}