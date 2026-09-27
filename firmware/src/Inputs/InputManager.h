#pragma once

#include <stdint.h>

class InputManager
{
public:

    void init();
    void update();

    bool forwardTriggered();
    bool reverseTriggered();
    bool stopTriggered();

    bool forwardLimit();
    bool reverseLimit();

private:

    static constexpr uint32_t DEBOUNCE_TIME_MS = 30;

    // Стабильное, подтверждённое состояние контактов
    bool m_forwardState = false;
    bool m_reverseState = false;
    bool m_forwardLimit = false;
    bool m_reverseLimit = false;

    // Состояние, которое сейчас наблюдаем на GPIO
    bool m_forwardRawState = false;
    bool m_reverseRawState = false;
    bool m_forwardLimitRawState = false;
    bool m_reverseLimitRawState = false;

    // Предыдущее стабильное состояние
    bool m_previousForwardState = false;
    bool m_previousReverseState = false;
    bool m_previousConflictState = false;


    // Время, когда обнаружили потенциальное изменение
    uint32_t m_forwardChangeTime = 0;
    uint32_t m_reverseChangeTime = 0;
    uint32_t m_forwardLimitChangeTime = 0;
    uint32_t m_reverseLimitChangeTime = 0;

    // Одноразовые события
    bool m_forwardTrigger = false;
    bool m_reverseTrigger = false;
    bool m_stopTrigger = false;

};