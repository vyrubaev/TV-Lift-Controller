#include <Arduino.h>

#include "Core/Core.h"

#include "Tests/BenchTest.h" // тут он для тестов, в будущем будет удален
#include "Logger/Logger.h" // тут он для тестов, в будущем будет удален

BenchTest bench;

void setup()
{
    Logger::init(); // тут он для тестов, в будущем будет удален

    Core::init();

    bench.init(); // тестовый стенд для проверки работы мотора, в будущем будет удален
}



void loop()
{
    Core::loop();

    bench.run(); // тестовый стенд для проверки работы мотора, в будущем будет удален
    delay(100); // тестовый стенд для проверки работы мотора, в будущем будет удален
}