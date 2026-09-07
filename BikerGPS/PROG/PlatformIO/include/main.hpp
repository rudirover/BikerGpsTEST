#pragma once

#include "debug.hpp"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "display.hpp"
#include "touch.hpp"
#include "route.hpp"
#include "ble.hpp"
#include "gps.hpp"

#define BLE_TIMEOUT 60000
#define SCROLLER_TIMEOUT 10000
#define DISPLAY_DIM_TIMEOUT 300000  // 5 minutes
#define DISPLAY_OFF_TIMEOUT 1500000 // 15 minutes
#define POWER_OFF_TIME_OUT 5000
//#define PERIPHERAL_CONTROL_PIN 15
#define BUTTON_PIN  0
#define SLEEP_HOLD_TIME_MS 3000 // 3 seconds
#define WAKE_HOLD_TIME_MS 1000 // 1 second (we already have initial delay)

enum class AppState : uint8_t
{
    NONE,
    BOOT_DONE,
    NO_SAVED_ROUTE_EXISTS,
    BLE_RECEIVE_ROUTE,
    BLE_ROUTE_RECEIVED,
    SAVED_ROUTE_EXISTS,
    MANAGE_ROUTE,
    SCROLL_UP,
    SCROLL_DOWN,
    SCROLL_INTO_VIEW,
    SET_ROUTE,
    REQ_POWER_OFF,
    ACK_POWER_OFF
};

enum class PowerState : uint8_t
{
    NONE,
    DISPLAY_ON,
    DISPLAY_DIM,
    DISPLAY_OFF,
    POWER_OFF
};

AppState currentAppState = AppState::BOOT_DONE;
AppState previousAppState = AppState::NONE;
PowerState currentPowerState = PowerState::DISPLAY_ON;
PowerState previousPowerState = PowerState::NONE;
TouchData touching;
unsigned long bleTimeOut;
unsigned long scrollerTimeOut;
unsigned long displayTimeOut;
unsigned long powerOffTimeOut;
bool waitingForFirstRelease = false;
void runPower();
void exitPowerState(PowerState state);
void enterPowerState(PowerState state);
void executePowerState(PowerState state);
void changePowerState(PowerState state);
void runApp();
void exitAppState(AppState state);
void enterAppState(AppState state);
void executeAppState(AppState state);
void changeAppState(AppState state);
void wakeUp();
void gotoSleep();
//void flushTouch();
