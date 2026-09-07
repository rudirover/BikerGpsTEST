#pragma once

#include <Arduino.h>

#define BUZZER_PIN 8
#define BTN_BEEP_FREQ 2500
#define BTN_BEEP_TIME 250

#define TPT_BEEP_FREQ 3000
#define TPT_BEEP_TIME 250

#define WPT_BEEP_FREQ 4000
#define WPT_BEEP_TIME 500

class Buzzer
{
public:
    Buzzer();
    void init();
    void btnBeep();
    void tptBeep();
    void wptBeep();
    void run();

private:
    bool beeping = false;
unsigned long beepTimer;   
int beepCount; 
};

extern Buzzer buzzer;