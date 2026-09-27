#include "buzzer.hpp"

Buzzer buzzer;

Buzzer::Buzzer() {};

void Buzzer::init()
{
    pinMode(BUZZER_PIN, OUTPUT);
    beeping = false;
}

void Buzzer::run()
{
    if (beeping)
    {
        switch (beepCount)
        {
        case 0:
            beepTimer = millis();
            tone(BUZZER_PIN, WPT_BEEP_FREQ);
            beepCount = 1;
            break;
        case 1:
            if ((millis() - beepTimer) > WPT_BEEP_TIME)
                beepCount = 2;
            break;
        case 2:
            beepTimer = millis();
            tone(BUZZER_PIN, TPT_BEEP_FREQ);
            beepCount = 3;
            break;
        case 3:
            if ((millis() - beepTimer) > WPT_BEEP_TIME)
                beepCount = 4;
            break;
        case 4:
            noTone(BUZZER_PIN);
            beeping = false;
            break;
        }
    }
}

void Buzzer::btnBeep()
{
    tone(BUZZER_PIN, BTN_BEEP_FREQ, BTN_BEEP_TIME);
}

void Buzzer::tptBeep()
{
    tone(BUZZER_PIN, TPT_BEEP_FREQ, TPT_BEEP_TIME);
}

void Buzzer::wptBeep()
{
    tone(BUZZER_PIN, WPT_BEEP_FREQ, WPT_BEEP_TIME);
}