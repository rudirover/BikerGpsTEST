#include "button.hpp"

Button button;

Button::Button() {};

void Button::init()
{
    lastReading = HIGH;
    debouncedState = HIGH;
    lastDebounceTime = 0;
}

bool Button::isPressed()
{
    int reading = digitalRead(BUTTON_PIN);

    // Handle debouncing
    if (reading != lastReading)
    {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_MS)
    {
        if (reading != debouncedState)
        {
            debouncedState = reading;
        }
    }
    lastReading = reading;

    return (debouncedState == LOW);
}