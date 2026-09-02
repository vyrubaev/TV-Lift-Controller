#include "InputManager.h"

#include <Arduino.h>

#include "../Config/BoardConfig.h"


void InputManager::init()
{
    pinMode(
        BoardConfig::DRY_CONTACT_UP_PIN,
        INPUT
    );

    pinMode(
        BoardConfig::DRY_CONTACT_DOWN_PIN,
        INPUT
    );

    pinMode(
        BoardConfig::LIMIT_SWITCH_UP_PIN,
        INPUT
    );

    pinMode(
        BoardConfig::LIMIT_SWITCH_DOWN_PIN,
        INPUT
    );


    m_forwardRawState =
        digitalRead(
            BoardConfig::DRY_CONTACT_UP_PIN
        ) == LOW;


    m_reverseRawState =
        digitalRead(
            BoardConfig::DRY_CONTACT_DOWN_PIN
        ) == LOW;


    m_forwardState = m_forwardRawState;
    m_reverseState = m_reverseRawState;

    m_previousForwardState = m_forwardState;
    m_previousReverseState = m_reverseState;

    m_previousConflictState =
        m_forwardState &&
        m_reverseState;
}

void InputManager::update()
{
    const uint32_t now = millis();

    // -------------------------------------------------
    // Считываем физическое состояние сухих контактов
    // -------------------------------------------------

    const bool forwardRaw =
        digitalRead(
            BoardConfig::DRY_CONTACT_UP_PIN
        ) == LOW;


    const bool reverseRaw =
        digitalRead(
            BoardConfig::DRY_CONTACT_DOWN_PIN
        ) == LOW;


    // -------------------------------------------------
    // FORWARD debounce
    // -------------------------------------------------

    if (forwardRaw != m_forwardRawState)
    {
        m_forwardRawState = forwardRaw;

        m_forwardChangeTime = now;
    }


    if (
        m_forwardRawState != m_forwardState &&
        (now - m_forwardChangeTime) >= DEBOUNCE_TIME_MS
    )
    {
        m_forwardState = m_forwardRawState;
    }

    // -------------------------------------------------
    // REVERSE debounce
    // -------------------------------------------------

    if (reverseRaw != m_reverseRawState)
    {
        m_reverseRawState = reverseRaw;

        m_reverseChangeTime = now;
    }


    if (
        m_reverseRawState != m_reverseState &&
        (now - m_reverseChangeTime) >= DEBOUNCE_TIME_MS
    )
    {
        m_reverseState = m_reverseRawState;
    }


    // -------------------------------------------------
    // Сбрасываем одноразовые события
    // -------------------------------------------------

    m_forwardTrigger = false;
    m_reverseTrigger = false;
    m_stopTrigger = false;


    // -------------------------------------------------
    // Определяем новые команды
    // -------------------------------------------------

    // -------------------------------------------------
    // Определяем состояние конфликта ( одновременного нажатия FORWARD и REVERSE )
    // -------------------------------------------------

const bool conflict =
    m_forwardState &&
    m_reverseState;

// -------------------------------------------------
// Новое событие STOP
// -------------------------------------------------

if (
    conflict &&
    !m_previousConflictState
)
{
    m_stopTrigger = true;
}

// -------------------------------------------------
// Если конфликта нет — проверяем новые команды
// -------------------------------------------------

if (!conflict)
{
    if (
        m_forwardState &&
        !m_previousForwardState
    )
    {
        m_forwardTrigger = true;
    }


    if (
        m_reverseState &&
        !m_previousReverseState
    )
    {
        m_reverseTrigger = true;
    }
}

    // -------------------------------------------------
    // Запоминаем состояния для следующего update()
    // -------------------------------------------------

    m_previousForwardState =
        m_forwardState;

    m_previousReverseState =
        m_reverseState;

    m_previousConflictState =
    conflict;

    // -------------------------------------------------
    // Концевики
    // -------------------------------------------------

    m_forwardLimit =
        digitalRead(
            BoardConfig::LIMIT_SWITCH_UP_PIN
        ) == HIGH;


    m_reverseLimit =
        digitalRead(
            BoardConfig::LIMIT_SWITCH_DOWN_PIN
        ) == HIGH;
}

bool InputManager::forwardTriggered()
{
    return m_forwardTrigger;
}


bool InputManager::reverseTriggered()
{
    return m_reverseTrigger;
}


bool InputManager::stopTriggered()
{
    return m_stopTrigger;
}


bool InputManager::forwardLimit()
{
    return m_forwardLimit;
}


bool InputManager::reverseLimit()
{
    return m_reverseLimit;
}

