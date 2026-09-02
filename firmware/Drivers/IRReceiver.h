#ifndef IR_RECEIVER_H
#define IR_RECEIVER_H

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include "Config/BoardConfig.h"
#include "Config/DeviceConfig.h"

// Перечисление команд, понятных нашему проекту
enum class IRCommand {
    NONE,
    UP,
    DOWN,
    STOP
};

class IRReceiver {
public:
    IRReceiver();
    void init();
    
    // Вызывается регулярно в loop() или Core::update()
    void update();

    // Возвращает принятую команду и сбрасывает её в NONE
    IRCommand getCommand();

private:
    IRrecv m_irrecv;
    decode_results m_results;
    
    IRCommand m_lastCommand = IRCommand::NONE;
    uint32_t m_lastRawCode = 0;

    IRCommand parseCode(uint32_t code);
};

#endif // IR_RECEIVER_H