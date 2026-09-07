#include "main.hpp"

void setup()
{
    /*
    // set the peripheral power control pin high
    pinMode(PERIPHERAL_CONTROL_PIN, OUTPUT);
    digitalWrite(PERIPHERAL_CONTROL_PIN, HIGH);
    */

    Serial.begin(115200);
    Serial.setDebugOutput(false);
    Debug.formatTimestampOn();
    Debug.setDebugLevel(DBG_INFO);
    delay(2000);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    //wakeUp();

    DBG_EXT(DBG_INFO, " ===== BOOT =====");
    DBG_EXT(DBG_INFO, "Power rails enabled");
    DBG_EXT(DBG_INFO, " Serial ready");

    display.init();
    DBG_EXT(DBG_INFO, "Display initialized");

    touch.init();
    DBG_EXT(DBG_INFO, "Touch initialized");

    route.init();
    DBG_EXT(DBG_INFO, "Route initialized");

    ble.init();
    DBG_EXT(DBG_INFO, "Ble initialized");

    gps.init();
    DBG_EXT(DBG_INFO, "Gps initialized");

    buzzer.init();
    DBG_EXT(DBG_INFO, "Buzzer initialized");    
}

void loop()
{
    // 1. Run display updates and query current touch frame
    display.run();
    gps.run();
    route.run();
    buzzer.run();

    touching = touch.getTouch();

    IF_DBG(DBG_DEBUG)
    {
        static TouchData oldTouch;
        if (oldTouch.gesture != touching.gesture)
        {
            DBG_EXT(DBG_DEBUG, "===== GETTOUCH =====");
            DBG_EXT(DBG_DEBUG, "gesture = %d", (int)touching.gesture);
            DBG_EXT(DBG_DEBUG, "fingerNum = %d", touching.fingerCount);
            oldTouch = touching;
        }
    }

    runPower();
}

void runPower()
{
    if (currentPowerState != previousPowerState)
    {
        exitPowerState(previousPowerState);
        enterPowerState(currentPowerState);
        previousPowerState = currentPowerState;
    }

    executePowerState(currentPowerState);
}

void enterPowerState(PowerState state)
{
    switch (state)
    {
    case PowerState::NONE:
        DBG_EXT(DBG_DEBUG, "PowerState::NONE");
        /* code */
        break;

    case PowerState::DISPLAY_ON:
        DBG_EXT(DBG_DEBUG, "PowerState::DISPLAY_ON");
        /* code */
        displayTimeOut = millis();
        display.setBackLight(BACKLIGHT_ON_LEVEL);
        break;

    case PowerState::DISPLAY_DIM:
        DBG_EXT(DBG_DEBUG, "PowerState::DISPLAY_DIM");
        /* code */
        displayTimeOut = millis();
        display.fadeBackLight(BACKLIGHT_DIM_LEVEL);
        break;

    case PowerState::DISPLAY_OFF:
        DBG_EXT(DBG_DEBUG, "PowerState::DISPLAY_ON");
        /* code */
        display.fadeBackLight(BACKLIGHT_OFF_LEVEL);
        break;

    case PowerState::POWER_OFF:
        DBG_EXT(DBG_DEBUG, "PowerState::POWER_OFF");
        /* code */
        break;

    default:
        break;
    }
}

void exitPowerState(PowerState state)
{
    switch (state)
    {
    case PowerState::NONE:
        DBG_EXT(DBG_DEBUG, "PowerState::NONE");
        /* code */
        break;

    case PowerState::DISPLAY_ON:
        DBG_EXT(DBG_DEBUG, "PowerState::DISPLAY_ON");
        /* code */
        break;

    case PowerState::DISPLAY_DIM:
        DBG_EXT(DBG_DEBUG, "PowerState::DISPLAY_DIM");
        /* code */
        break;

    case PowerState::DISPLAY_OFF:
        DBG_EXT(DBG_DEBUG, "PowerState::DISPLAY_OFF");
        /* code */
        break;

    case PowerState::POWER_OFF:
        DBG_EXT(DBG_DEBUG, "PowerState::POWER_OFF");
        /* code */
        break;

    default:
        break;
    }
}

void executePowerState(PowerState state)
{
    static PowerState oldState = PowerState::NONE;

    switch (state)
    {
    case PowerState::NONE:
        if (oldState != state)
        DBG_EXT(DBG_DEBUG, "PowerState::NONE");
        /* code */
        break;

    case PowerState::DISPLAY_ON:
        if (oldState != state)
        DBG_EXT(DBG_DEBUG, "PowerState::DISPLAY_ON");
        /* code */
        if (touching.gesture != GestureType::GESTURE_NONE)
        {
            displayTimeOut = millis();
        }

        if (((millis() - displayTimeOut) > DISPLAY_DIM_TIMEOUT) && (currentAppState == AppState::MANAGE_ROUTE))
        {
            changePowerState(PowerState::DISPLAY_DIM);
        }
        else
        {
            runApp();
        }
        break;

    case PowerState::DISPLAY_DIM:
        if (oldState != state)
        DBG_EXT(DBG_DEBUG, "PowerState::DISPLAY_DIM");
        /* code */
        if (((millis() - displayTimeOut) > DISPLAY_OFF_TIMEOUT) && (currentAppState == AppState::MANAGE_ROUTE))
        {
            changePowerState(PowerState::DISPLAY_OFF);
        }
        if (touching.gesture == GestureType::GESTURE_SINGLE_TAP)
        {
            changePowerState(PowerState::DISPLAY_ON);
        }
        break;

    case PowerState::DISPLAY_OFF:
        if (oldState != state)
        DBG_EXT(DBG_DEBUG, "PowerState::DISPLAY_OFF");
        /* code */
        if (touching.gesture == GestureType::GESTURE_SINGLE_TAP)
        {
            changePowerState(PowerState::DISPLAY_ON);
        }
        break;

    case PowerState::POWER_OFF:
        if (oldState != state)
        DBG_EXT(DBG_DEBUG, "PowerState::POWER_OFF");
        /* code */
        gotoSleep();
        break;

    default:
        break;
    }

    oldState = state;
}

void changePowerState(PowerState state)
{
    currentPowerState = state;
}

void runApp()
{
    if (currentAppState != previousAppState)
    {
        exitAppState(previousAppState);
        enterAppState(currentAppState);
        previousAppState = currentAppState;
    }

    executeAppState(currentAppState);
}

void enterAppState(AppState state)
{
    //flushTouch();
    switch (state)
    {
    case AppState::NONE:
        DBG_EXT(DBG_DEBUG, "AppState::NONE");
        /* code */
        break;

    case AppState::BOOT_DONE:
        DBG_EXT(DBG_DEBUG, "AppState::BOOT_DONE");
        /* code */
        break;

    case AppState::NO_SAVED_ROUTE_EXISTS:
        DBG_EXT(DBG_DEBUG, "AppState::NO_SAVED_ROUTE_EXISTS");
        /* code */
        display.showNoRoute();
        break;

    case AppState::BLE_RECEIVE_ROUTE:
        DBG_EXT(DBG_DEBUG, "AppState::RECEIVE_ROUTE");
        /* code */
        display.showReceiveRoute();
        ble.enable();
        bleTimeOut = millis();
        break;

    case AppState::BLE_ROUTE_RECEIVED:
        DBG_EXT(DBG_DEBUG, "AppState::BLE_ROUTE_RECEIVED");
        /* code */
        route.save(ble.receivedRoute);
        break;

    case AppState::SAVED_ROUTE_EXISTS:
        DBG_EXT(DBG_DEBUG, "AppState::SAVED_ROUTE_EXISTS");
        /* code */
        route.prepare();
        break;

    case AppState::MANAGE_ROUTE:
        DBG_EXT(DBG_DEBUG, "AppState::MANAGE_ROUTE");
        /* code */
        display.showRoute();
        break;

    case AppState::SCROLL_UP:
        DBG_EXT(DBG_DEBUG, "AppState::SCROLL_UP");
        /* code */
        display.scrollUp();
        break;

    case AppState::SCROLL_DOWN:
        DBG_EXT(DBG_DEBUG, "AppState::SCROLL_DOWN");
        /* code */
        display.scrollDown();
        break;

    case AppState::SCROLL_INTO_VIEW:
        DBG_EXT(DBG_DEBUG, "AppState::SCROLL_INTO_VIEW");
        /* code */
        display.scrollIntoView();
        break;

    case AppState::SET_ROUTE:
        DBG_EXT(DBG_DEBUG, "AppState::SET_ROUTE");
        /* code */
        route.setRoute();
        break;

    case AppState::REQ_POWER_OFF:
        DBG_EXT(DBG_DEBUG, "AppState::REQ_POWER_OFF");
        /* code */
        display.showPowerOff();
        break;

    case AppState::ACK_POWER_OFF:
        DBG_EXT(DBG_DEBUG, "AppState::ACK_POWER_OFF");
        /* code */
        powerOffTimeOut = millis();
        break;

    default:
        break;
    }
}

void exitAppState(AppState state)
{
    switch (state)
    {
    case AppState::NONE:
        DBG_EXT(DBG_DEBUG, "AppState::NONE");
        /* code */
        break;

    case AppState::BOOT_DONE:
        DBG_EXT(DBG_DEBUG, "AppState::BOOT_DONE");
        /* code */
        break;

    case AppState::NO_SAVED_ROUTE_EXISTS:
        DBG_EXT(DBG_DEBUG, "AppState::NO_SAVED_ROUTE_EXISTS");
        /* code */
        break;

    case AppState::BLE_RECEIVE_ROUTE:
        DBG_EXT(DBG_DEBUG, "AppState::BLE_RECEIVE_ROUTE");
        /* code */
        ble.disable();
        break;

    case AppState::BLE_ROUTE_RECEIVED:
        DBG_EXT(DBG_DEBUG, "AppState::BLE_ROUTE_RECIVED");
        /* code */
        break;

    case AppState::SAVED_ROUTE_EXISTS:
        DBG_EXT(DBG_DEBUG, "AppState::SAVED_ROUTE_EXISTS");
        /* code */
        break;

    case AppState::MANAGE_ROUTE:
        DBG_EXT(DBG_DEBUG, "AppState::MANAGE_ROUTE");
        /* code */
        break;

    case AppState::SCROLL_UP:
        DBG_EXT(DBG_DEBUG, "AppState::SCROLL_UP");
        /* code */
        break;

    case AppState::SCROLL_DOWN:
        DBG_EXT(DBG_DEBUG, "AppState::SCROLL_DOWN");
        /* code */
        break;

    case AppState::SCROLL_INTO_VIEW:
        DBG_EXT(DBG_DEBUG, "AppState::SCROLL_INTO_VIEW");
        /* code */
        break;

    case AppState::SET_ROUTE:
        DBG_EXT(DBG_DEBUG, "AppState::SET_ROUTE");
        /* code */
        break;

    case AppState::REQ_POWER_OFF:
        DBG_EXT(DBG_DEBUG, "AppState::REQ_POWER_OFF");
        /* code */
        break;

    case AppState::ACK_POWER_OFF:
        DBG_EXT(DBG_DEBUG, "AppState::ACK_POWER_OFF");
        /* code */
        break;

    default:
        break;
    }
}

void executeAppState(AppState state)
{
    static AppState oldState = AppState::NONE;

    switch (state)
    {
    case AppState::NONE:
        if (oldState != state)
        DBG_EXT(DBG_DEBUG, "AppState::NONE");
        /* code */
        changeAppState(AppState::BOOT_DONE);
        break;

    case AppState::BOOT_DONE:
        if (oldState != state)
        DBG_EXT(DBG_DEBUG, "AppState::BOOT_DONE");
        /* code */
        if (route.exists())
        {
            DBG_EXT(DBG_DEBUG, "AppState::BOOT_DONE - > SAVED_ROUTE_EXISTS");
            changeAppState(AppState::SAVED_ROUTE_EXISTS);
            break;
        }
        else
        {
            DBG_EXT(DBG_DEBUG, "AppState::BOOT_DONE - > NO_SAVED_ROUTE_EXISTS");            
            changeAppState(AppState::NO_SAVED_ROUTE_EXISTS);
            break;
        }
        break;

    case AppState::NO_SAVED_ROUTE_EXISTS:
        if (oldState != state)
        DBG_EXT(DBG_DEBUG, "AppState::NO_ROUTE_EXISTS");
        /* code */
        if (touching.gesture == GestureType::GESTURE_SWIPE_RIGHT)
        {
            DBG_EXT(DBG_DEBUG, "AppState::BOOT_DONE - > SWIPE_RIGHT");            
            changeAppState(AppState::BLE_RECEIVE_ROUTE);
            break;
        }
        break;

    case AppState::BLE_RECEIVE_ROUTE:
        if (oldState != state)
        DBG_EXT(DBG_DEBUG, "AppState::BLE_RECEIVE_ROUTE");
        /* code */
        if (((millis() - bleTimeOut) > BLE_TIMEOUT) && !ble.deviceConnected)
        {
            DBG_EXT(DBG_DEBUG, "AppState::BOOT_DONE - > BLE_TIMEOUT");            
            changeAppState(AppState::BOOT_DONE);
            break;
        }
        if (touching.gesture == GestureType::GESTURE_SWIPE_LEFT)
        {
            DBG_EXT(DBG_DEBUG, "AppState::BOOT_DONE - > SWIPE_LEFT");            
            changeAppState(AppState::BOOT_DONE);
            break;
        }
        if (ble.routeAvailable)
        {
            DBG_EXT(DBG_DEBUG, "AppState::BOOT_DONE - > BLE_ROUTE_AVAILABLE");            
            changeAppState(AppState::BLE_ROUTE_RECEIVED);
            break;
        }
        break;

    case AppState::BLE_ROUTE_RECEIVED:
        if (oldState != state)
        DBG_EXT(DBG_DEBUG, "AppState::BLE_ROUTE_RECEIVED");
        /* code */
        changeAppState(AppState::SAVED_ROUTE_EXISTS);
        break;

    case AppState::SAVED_ROUTE_EXISTS:
        if (oldState != state)
        DBG_EXT(DBG_DEBUG, "AppState::SAVED_ROUTE_EXISTS");
        /* code */
        changeAppState(AppState::MANAGE_ROUTE);
        break;

    case AppState::MANAGE_ROUTE:
        if (oldState != state)
        DBG_EXT(DBG_DEBUG, "AppState::MANAGE_ROUTE");
        /* code */
        if (touching.gesture == GestureType::GESTURE_SWIPE_RIGHT)
        {
            DBG_EXT(DBG_DEBUG, "AppState::MANAGE_ROUTE - > SWIPE_RIGHT");
            changeAppState(AppState::BLE_RECEIVE_ROUTE);
            break;
        }

        if (touching.gesture == GestureType::GESTURE_SWIPE_DOWN)
        {
            DBG_EXT(DBG_DEBUG, "AppState::MANAGE_ROUTE - > SWIPE_DOWN");
            changeAppState(AppState::SCROLL_DOWN);
            break;
        }

        if (touching.gesture == GestureType::GESTURE_SWIPE_UP)
        {
            DBG_EXT(DBG_DEBUG, "AppState::MANAGE_ROUTE - > SWIPE_UP");
            changeAppState(AppState::SCROLL_UP);
            break;
        }

        if (touching.gesture == GestureType::GESTURE_DOUBLE_TAP)
        {
            DBG_EXT(DBG_DEBUG, "AppState::MANAGE_ROUTE - > DOUBLE_TAP");
            changeAppState(AppState::SET_ROUTE);
            break;
        }

        if ((touching.gesture == GestureType::GESTURE_BUTTON_TAP) /*&& (touching.y > 300)*/)
        {
            DBG_EXT(DBG_DEBUG, "AppState::MANAGE_ROUTE - > BUTTON_TAP");
            changeAppState(AppState::REQ_POWER_OFF);
            break;
        }

        if (!display.inView())
        {
            if ((millis() - scrollerTimeOut) >= SCROLLER_TIMEOUT)
            {
            DBG_EXT(DBG_DEBUG, "AppState::MANAGE_ROUTE - > SCROLLER_TIMEOUT");
                changeAppState(AppState::SCROLL_INTO_VIEW);
            }
            break;
        }

        break;

    case AppState::SCROLL_UP:
        if (oldState != state)
            DBG_EXT(DBG_DEBUG, "AppState::SCROLL_UP");
        /* code */
        scrollerTimeOut = millis();
        changeAppState(AppState::MANAGE_ROUTE);
        break;

    case AppState::SCROLL_DOWN:
        if (oldState != state)
            DBG_EXT(DBG_DEBUG, "AppState::SCROLL_DOWN");
        /* code */
        scrollerTimeOut = millis();
        changeAppState(AppState::MANAGE_ROUTE);
        break;

    case AppState::SCROLL_INTO_VIEW:
        if (oldState != state)
            DBG_EXT(DBG_DEBUG, "AppState::SCROLL_INTO_VIEW");
        /* code */
        changeAppState(AppState::MANAGE_ROUTE);
        break;

    case AppState::SET_ROUTE:
        if (oldState != state)
            DBG_EXT(DBG_DEBUG, "AppState::SET_ROUTE");
        /* code */
        changeAppState(AppState::MANAGE_ROUTE);
        break;

    case AppState::REQ_POWER_OFF:
        if (oldState != state)
            DBG_EXT(DBG_DEBUG, "AppState::REQ_POWER_OFF");
        /* code */
        changeAppState(AppState::ACK_POWER_OFF);
        break;

    case AppState::ACK_POWER_OFF:
        if (oldState != state)
            DBG_EXT(DBG_DEBUG, "AppState::ACK_POWER_OFF");
        /* code */
        if ((millis() - powerOffTimeOut) >= POWER_OFF_TIME_OUT)
        {
            DBG_EXT(DBG_DEBUG, "AppState::ACK_POWER_OFF -> POWER_OFF_TIME_OUT");
            changeAppState(AppState::MANAGE_ROUTE);
            break;
        }
        if (touching.gesture == GestureType::GESTURE_BUTTON_TAP)
        {
            DBG_EXT(DBG_DEBUG, "AppState::ACK_POWER_OFF -> BUTTON_TAP");
            changePowerState(PowerState::POWER_OFF);
            break;
        }
        break;

    default:
        break;
    }
    oldState = state;
}

void changeAppState(AppState state)
{
    currentAppState = state;
}

void gotoSleep()
{
            Serial.flush();

            rtc_gpio_pullup_en((gpio_num_t)BUTTON_PIN);
            rtc_gpio_pulldown_dis((gpio_num_t)BUTTON_PIN);
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
            esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
            esp_deep_sleep_start();         
}

void wakeUp()
{
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
            gotoSleep();
            Serial.println("Released too early. Returning to deep sleep.");
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

    /*

    // Clear any lingering wakeup configurations
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    // Ensure RTC pull-up is active for deep sleep state
    rtc_gpio_pullup_en((gpio_num_t)BUTTON_PIN);
    rtc_gpio_pulldown_dis((gpio_num_t)BUTTON_PIN);

    // Configure EXT0 wakeup source for the next sleep cycle (0 = LOW level)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);   
    */ 
}