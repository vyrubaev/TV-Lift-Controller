#include <Arduino.h>

#include "Core/Core.h"

#include "Tests/BenchTest.h" // тут он для тестов, в будущем будет удален
#include "Logger/Logger.h" // тут он для тестов, в будущем будет удален
#include "Elevator/Elevator.h" 

BenchTest bench; // тестовый стенд для проверки работы мотора, в будущем будет удален
Elevator elevator; // тут он для тестов, в будущем будет удален

void setup()
{
    Logger::init(); // тут он для тестов, в будущем будет удален

    Core::init();

    //bench.init(); // тестовый стенд для проверки работы мотора, в будущем будет удален

    elevator.init(); 
}



void loop()
{
    Core::loop();

    

    //bench.run(); // тестовый стенд для проверки работы мотора, в будущем будет удален
    elevator.update();
}