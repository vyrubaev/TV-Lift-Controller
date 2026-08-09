#include "BenchTest.h"


#include "../Drivers/Motor.h"
#include "../Logger/Logger.h"


Motor motor;


void BenchTest::init()
{
    motor.init();

    Logger::info("Bench test started");
}



void BenchTest::run()
{

    Logger::info("FORWARD");

    motor.setSpeed(255);
    Logger::debug("Motor speed set to 255");

    motor.forward();

    if(motor.getState() == MotorState::FORWARD)
{
    Logger::debug("State check: FORWARD");
}

    delay(3000);



    Logger::info("STOP");

    motor.stop();

    delay(1000);



    Logger::info("REVERSE");

    motor.setSpeed(20);
    Logger::debug("Motor speed set to 20");
    
    motor.reverse();

    delay(3000);

    Logger::info("STOP");

    motor.stop();
    

    delay(1000);

}