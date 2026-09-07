#pragma once

#include <driver/rtc_io.h>
#include "display.hpp"
#include "debug.hpp"
#include "buzzer.hpp"
#include "touch.hpp"

const int BUTTON_PIN = 0;
const unsigned long SLEEP_HOLD_TIME_MS = 3000; // 3 seconds
const unsigned long WAKE_HOLD_TIME_MS = 1000; // 1 second (we already have initial delay)