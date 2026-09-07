#include "main.hpp"
#include <driver/rtc_io.h>

double animAngle = 0.0;
bool waitingForFirstRelease = false; // Tracks if we need to clear the initial wake press

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(false);
    Debug.formatTimestampOn();
    Debug.setDebugLevel(DBG_INFO);
    delay(2000);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Check why we booted up
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
    {
        unsigned long startTime = millis();
        bool heldForThreeSeconds = false;

        // Check if the button remains held down for 3 seconds upon waking
        while (digitalRead(BUTTON_PIN) == LOW)
        {
            if (millis() - startTime >= WAKE_HOLD_TIME_MS)
            {
                heldForThreeSeconds = true;
                break;
            }
            delay(10);
        }

        // If released early, go right back to sleep safely
        if (!heldForThreeSeconds)
        {
            Serial.println("Released too early. Returning to deep sleep.");
            Serial.flush();

            rtc_gpio_pullup_en((gpio_num_t)BUTTON_PIN);
            rtc_gpio_pulldown_dis((gpio_num_t)BUTTON_PIN);

            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
            esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
            esp_deep_sleep_start();
        }
        else
        {
            Serial.println("Woke up! Held for 3 seconds.");
            // Successfully woke up! Proceed straight to boot without blocking,
            // but flag that we need to see a button release first in the loop.
            waitingForFirstRelease = true;
        }
    }
    else
    {
        Serial.println("Fresh boot or reset.");
        waitingForFirstRelease = false;
    }

    // Clear any lingering wakeup configurations
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    // Ensure RTC pull-up is active for deep sleep state
    rtc_gpio_pullup_en((gpio_num_t)BUTTON_PIN);
    rtc_gpio_pulldown_dis((gpio_num_t)BUTTON_PIN);

    // Configure EXT0 wakeup source for the next sleep cycle (0 = LOW level)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);

    DBG_EXT(DBG_INFO, " ===== BOOT =====");
    DBG_EXT(DBG_INFO, "Power rails enabled");
    DBG_EXT(DBG_INFO, " Serial ready");

    display.init();
    DBG_EXT(DBG_INFO, "Display initialized");

    buzzer.init();
    DBG_EXT(DBG_INFO, "Buzzer initialized");

    buzzer.btnBeep();

    if (!display.loadMapData("/map.bin"))
    {
        DBG_EXT(DBG_WARNING, "Failed to load /map.bin, displaying fallback notice");
        display.showNoRoute();
    }
}

void loop()
{
    // If we just woke up, wait until the user lifts their finger before enabling sleep detection
    if (waitingForFirstRelease)
    {
        if (digitalRead(BUTTON_PIN) == HIGH)
        {
            waitingForFirstRelease = false; // Finger has been lifted, normal operation resumes
        }
    }
    else
    {
        // Check if button is held for 3 seconds to go to sleep
        if (digitalRead(BUTTON_PIN) == LOW)
        {
            unsigned long pressStartTime = millis();
            bool longPressDetected = false;

            while (digitalRead(BUTTON_PIN) == LOW)
            {
                if (millis() - pressStartTime >= SLEEP_HOLD_TIME_MS)
                {
                    longPressDetected = true;
                    break;
                }
                delay(10);
            }

            if (longPressDetected)
            {
                Serial.println("Entering deep sleep...");

                // Instantly kill the backlight for immediate visual feedback
                display.gotoSleep();

                // Wait for button release so it doesn't instantly re-wake
                while (digitalRead(BUTTON_PIN) == LOW)
                {
                    delay(10);
                }

                Serial.flush();

                // Enable RTC pull-up so the pin doesn't float during deep sleep
                rtc_gpio_pullup_en((gpio_num_t)BUTTON_PIN);
                rtc_gpio_pulldown_dis((gpio_num_t)BUTTON_PIN);

                esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
                esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
                esp_deep_sleep_start();
            }
        }
    }

    // Your main program logic goes here
    animAngle += 0.002;
    double simOffsetX = 900000.0 * cos(animAngle);
    double simOffsetY = 900000.0 * sin(animAngle);

    static double lastX = 0;
    static double lastY = 0;

    double dx = simOffsetX - lastX;
    double dy = simOffsetY - lastY;

    double heading = 0.0;
    if (fabs(dx) > 0.001 || fabs(dy) > 0.001)
    {
        double math_angle = atan2(dy, dx);
        heading = (M_PI / 2.0) - math_angle;
    }

    lastX = simOffsetX;
    lastY = simOffsetY;

    display.renderMap(simOffsetX, simOffsetY, heading);

touch.run();
    TouchData data = touch.getTouch();

    if (data.gesture == GESTURE_TAP)
    {
        Serial.printf("Tapped at X: %d, Y: %d\n", data.xPos, data.yPos);
    }
    else if (data.gesture == GESTURE_SWIPE_LEFT)
    {
        Serial.println("Swiped Left!");
    }
    else if (data.gesture == GESTURE_SWIPE_RIGHT)
    {
        Serial.println("Swiped Right!");
    }
    else if (data.gesture == GESTURE_SWIPE_UP)
    {
        Serial.println("Swiped Up!");
    }
    else if (data.gesture == GESTURE_SWIPE_DOWN)
    {
        Serial.println("Swiped Down!");
    }
}