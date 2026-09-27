#pragma once

#include <Arduino.h>

#define BUTTON_PIN 0 // GPIO0 on ESP32-S3 (Boot button)
#define BUTTON_DEBOUNCE_MS 50


class Button
{
public:
    Button();
    void init();
    bool isPressed();

private:
    int lastReading = HIGH;
    int debouncedState = HIGH;
    unsigned long lastDebounceTime = 0;
};

extern Button button;