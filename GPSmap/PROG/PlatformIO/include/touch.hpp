#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "display.hpp"

#define SWIPE_THRESHOLD  60
#define TAP_TIMEOUT_MS  250 // Max duration to qualify as a quick tap
#define TAP_DEADZONE  15  // Max drift allowed during a tap action

// Enum representing the final computed motion action
enum GestureType {
    GESTURE_NONE,
    GESTURE_TAP,
    GESTURE_SWIPE_LEFT,
    GESTURE_SWIPE_RIGHT,
    GESTURE_SWIPE_UP,
    GESTURE_SWIPE_DOWN
};

// The structured data format returned on every query
struct TouchData {
    GestureType gesture;
    uint8_t fingerCount;
    int32_t xPos;
    int32_t yPos;
};

class Touch {
private:
    LGFX& _lcd; // Reference to your active LovyanGFX display instance
    
    // Tracking States
    bool _isTracking = false;
    bool _gestureFired = false;
    uint32_t _pressStartTime = 0;
    int32_t _startX = 0;
    int32_t _startY = 0;
    int32_t _lastX = 0;
    int32_t _lastY = 0;


public:
    // Pass your LGFX setup reference on creation
    Touch(LGFX& lcdInstance);

    // Call this inside your main loop() continuously to update states non-blockingly
    void run();

    // Query this at any time to grab the active position and single-fire gestures
    TouchData getTouch();
};


extern Touch touch;