#include "touch.hpp"

// Active frame container mapped internally
static TouchData currentFrameState = {GESTURE_NONE, 0, -1, -1};

Touch touch(display.getCanvas());

Touch::Touch(LGFX &lcdInstance) : _lcd(lcdInstance) {}

void Touch::init()
{

    _isTracking = false;
    _gestureFired = false;
    _pressStartTime = 0;
    _startX = 0;
    _startY = 0;
    _lastX = 0;
    _lastY = 0;
    _lastTapTime = 0;
    _lastTapX = 0;
    _lastTapY = 0;
}

TouchData Touch::getTouch()
{
    int32_t curX, curY;

    // LovyanGFX native touch confirmation pipeline
    bool isTouching = _lcd.getTouch(&curX, &curY);
    uint32_t now = millis();

    if (isTouching)
    {
        currentFrameState.fingerCount = 1;
        currentFrameState.xPos = curX;
        currentFrameState.yPos = curY;

        if (!_isTracking)
        {
            _isTracking = true;
            _gestureFired = false;
            _pressStartTime = millis();
            _startX = curX;
            _startY = curY;
            Serial.printf("Touch Down -> X: %d, Y: %d\n", curX, curY);
        }

        _lastX = curX;
        _lastY = curY;

        int32_t deltaX = curX - _startX;
        int32_t deltaY = curY - _startY;

        // Print movement details to check if axes are swapped or inverted
        Serial.printf("Tracking -> deltaX: %d, deltaY: %d\n", deltaX, deltaY);

        if (!_gestureFired)
        {
            if (abs(deltaX) >= SWIPE_THRESHOLD || abs(deltaY) >= SWIPE_THRESHOLD)
            {
                if (abs(deltaX) > abs(deltaY))
                {
                    currentFrameState.gesture = (deltaX > 0) ? GESTURE_SWIPE_RIGHT : GESTURE_SWIPE_LEFT;
                }
                else
                {
                    currentFrameState.gesture = (deltaY > 0) ? GESTURE_SWIPE_DOWN : GESTURE_SWIPE_UP;
                }
                _gestureFired = true;
            }
        }
    }
    else
    {
        // Handle natural touch release transitions
        if (_isTracking)
        {
            uint32_t pressDuration = now - _pressStartTime;

            // If a swipe didn't trigger, evaluate if the lift was a quick Tap
            if (!_gestureFired && (pressDuration <= TAP_TIMEOUT_MS))
            {
                int32_t driftX = abs(_lastX - _startX);
                int32_t driftY = abs(_lastY - _startY);

                if (driftX < TAP_DEADZONE && driftY < TAP_DEADZONE)
                {
                    // Check for Double Tap
                    if ((now - _lastTapTime <= DOUBLE_TAP_TIME_MS) &&
                        (abs(_lastX - _lastTapX) < TAP_DEADZONE) &&
                        (abs(_lastY - _lastTapY) < TAP_DEADZONE))
                    {
                        currentFrameState.gesture = GESTURE_DOUBLE_TAP;
                        Serial.println("Double Tap Detected!");
                        // Reset last tap time so a triple tap doesn't trigger another double tap immediately
                        _lastTapTime = 0;
                    }
                    else
                    {
                        currentFrameState.gesture = GESTURE_SINGLE_TAP;
                        _lastTapTime = now;
                        _lastTapX = _lastX;
                        _lastTapY = _lastY;
                    }
                }
            }
            _isTracking = false;
        }
    }
    // 1. Snapshot the payload
    TouchData snapshot = currentFrameState;

    // 2. Clear out gesture states immediately after consumption (Single-fire execution)
    currentFrameState.gesture = GESTURE_NONE;
    currentFrameState.fingerCount = 0;

    return snapshot;
}

/*
TouchData Touch::getTouch()
{
    // 1. Snapshot the payload
    TouchData snapshot = currentFrameState;

    // 2. Clear out gesture states immediately after consumption (Single-fire execution)
    currentFrameState.gesture = GESTURE_NONE;
    currentFrameState.fingerCount = 0;

    return snapshot;
}
    */
