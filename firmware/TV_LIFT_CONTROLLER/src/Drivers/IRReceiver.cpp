#include "Drivers/IRReceiver.h"
#include "Logger/Logger.h"

IRReceiver::IRReceiver() 
    : m_irrecv(BoardConfig::IR_RECEIVER_PIN) {}

void IRReceiver::init() {
    m_irrecv.enableIRIn(); // Старт приема
    Logger::info("IR_Receiver initialized");
}

void IRReceiver::update() {
    if (m_irrecv.decode(&m_results)) {
        uint32_t rawCode = m_results.value;

        // Если пришел сигнал "повтор кнопки" (пользователь удерживает кнопку)
        if (rawCode == DeviceConfig::IR_CODE_REPEAT) {
            rawCode = m_lastRawCode; 
        } else {
            m_lastRawCode = rawCode;
        }

        // Логируем RAW-код в формате HEX
        char hexBuf[32];
        snprintf(hexBuf, sizeof(hexBuf), "0x%X", rawCode);
        
        IRCommand cmd = parseCode(rawCode);

        // Логируем только если распознали команду или получили новый код
        if (cmd != IRCommand::NONE) {
            String logMessage = "IR Signal received: " + String(hexBuf);
            Logger::debug(logMessage.c_str());
            m_lastCommand = cmd; // Сохраняем последнюю команду для получения через getCommand()
        } else {
            String logMessage = "Unknown IR Code: " + String(hexBuf); //Раскомментируйте для отладки неопознанных кнопок:
            Logger::debug(logMessage.c_str()); // Выводим в лог для отладки
        }

        m_irrecv.resume(); // Готовим к приему следующего сигнала
    }
}

IRCommand IRReceiver::parseCode(uint32_t code) {
    if (code == DeviceConfig::IR_CODE_FORWARD) {
        Logger::info("IR Command Parsed -> FORWARD");
        return IRCommand::MOVE_UP;
    } 
    else if (code == DeviceConfig::IR_CODE_REVERSE) {
        Logger::info("IR Command Parsed -> REVERSE");
        return IRCommand::MOVE_DOWN;
    } 
    else if (code == DeviceConfig::IR_CODE_STOP) {
        Logger::info("IR Command Parsed -> STOP");
        return IRCommand::STOP;
    }

    return IRCommand::NONE;
}

IRCommand IRReceiver::getCommand() {
    IRCommand cmd = m_lastCommand;
    m_lastCommand = IRCommand::NONE; // Забираем команду единоразово (сброс флага)
    return cmd;
}