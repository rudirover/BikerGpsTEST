#include "display.hpp"
#include "FreeSans7pt7b.h"

Display display;

Display::Display() {};

void Display::init()
{
    // 1. Initialize LCD Screen
    canvas.init();
    canvas.setSwapBytes(true);
    canvas.setBrightness(BACKLIGHT_OFF_LEVEL);
    canvas.setRotation(1);
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE);

    sprite.createSprite(canvas.width(), canvas.height());
    sprite.setSwapBytes(true);

    oldBackLightLevel = BACKLIGHT_OFF_LEVEL;
    newBackLigthLevel = BACKLIGHT_OFF_LEVEL;
    blinkerTime = millis();
    initialized = true;
}

void Display::run()
{
    static bool oldBlinker = false;

    // Backlight level handler
    if (oldBackLightLevel != newBackLigthLevel)
    {
        onFadeBackLight();
    }

    if ((millis() - blinkerTime) >= BLINKER_TIME)
    {
        blinker = !blinker;
        blinkerTime = millis();
    }

    if (oldBlinker != blinker)
    {
        oldBlinker = blinker;
        if (showingRoute)
        {
            drawRoute();
        }
    }
}

void Display::showNoRoute()
{
    showingRoute = false;
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextDatum(CC_DATUM);
    canvas.setFont(&fonts::FreeSans12pt7b);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("NO ROUTE", canvas.width() / 2, (canvas.height() / 2) - 20);
    canvas.drawString("AVAILABLE!", canvas.width() / 2, (canvas.height() / 2) + 25);

    canvas.setFont(&FreeSans7pt7b);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("SWIPE RIGHT TO LOAD", canvas.width() / 2, 305);
    return;
}

void Display::showReceiveRoute()
{
    showingRoute = false;
    canvas.fillScreen(TFT_BLACK);

    // Draw Bluetooth icon at the top - blue, large
    drawBluetoothIcon(canvas.width() / 2, 80, 120, TFT_BLUE);

    // Add text below icon
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.setFont(&fonts::FreeSans12pt7b); // Use FreeSans 12pt font
    canvas.setTextDatum(CC_DATUM);          // Center alignment
    canvas.drawString("WAITING FOR", canvas.width() / 2, (canvas.height() / 2) + 20);
    canvas.drawString("NEW ROUTE", canvas.width() / 2, (canvas.height() / 2) + 60);

    canvas.setFont(&FreeSans7pt7b);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("SWIPE LEFT TO EXIT", canvas.width() / 2, 305);
}

void Display::showPowerOff()
{
    showingRoute = false;
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextDatum(CC_DATUM);
    canvas.setFont(&fonts::FreeSans12pt7b);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("REALLY", canvas.width() / 2, (canvas.height() / 2) - 20);
    canvas.drawString("POWER OFF?", canvas.width() / 2, (canvas.height() / 2) + 25);

    canvas.setFont(&fonts::FreeSans9pt7b);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("YES", canvas.width() / 2, 305);

    return;
}

void Display::onFadeBackLight()
{
    /*
    Serial.println("onFadeBackLight");
    */
    if ((millis() - fadeTimer) > fadeTime)
    {
        fadeTimer = millis();
        if (oldBackLightLevel > newBackLigthLevel)
        {
            oldBackLightLevel--;
        }
        else
        {
            oldBackLightLevel++;
        }
        canvas.setBrightness(oldBackLightLevel);
    }
    /*
   canvas.setBrightness(newBackLigthLevel);
   oldBackLightLevel = newBackLigthLevel;
   */
}

void Display::fadeBackLight(uint8_t backLightLevel)
{
    if (backLightLevel != oldBackLightLevel)
    {
        fadeTimer = millis();
        newBackLigthLevel = backLightLevel;
        fadeTime = BACKLIGHT_FADE_TIME / abs(newBackLigthLevel - oldBackLightLevel);
        /*
        Serial.print("fadeTime=");
        Serial.println(fadeTime);
        */
    }
}

void Display::setBackLight(uint8_t backLightLevel)
{
    canvas.setBrightness(backLightLevel);
    oldBackLightLevel = backLightLevel;
    newBackLigthLevel = backLightLevel;
}

void Display::drawBluetoothIcon(int centerX, int centerY, int size, uint16_t color)
{
    // Simplified Bluetooth indicator - thick bold lines
    int cx = centerX;
    int cy = centerY;
    int r = size / 2;

    // Draw outer circle (very thick - triple lines)
    for (int i = 0; i < 5; i++)
    {
        canvas.drawCircle(cx, cy, r - i, color);
    }

    // Draw simple Bluetooth symbol: vertical line with triangles
    int h = r - 20; // Height of symbol

    // Center vertical stem (very thick)
    for (int offset = 0; offset < 5; offset++)
    {
        canvas.drawLine(cx + offset, cy - h, cx + offset, cy + h, color);
    }

    // Top-left triangle (upper half, left side)
    for (int offset = 0; offset < 5; offset++)
    {
        canvas.drawLine(cx - r / 2 + offset, cy - h / 2, cx + offset, cy, color);
    }

    // Top-right triangle (upper half, right side)
    for (int offset = 0; offset < 5; offset++)
    {
        canvas.drawLine(cx + offset, cy - h, cx + r / 2 + offset, cy - h / 2, color);
    }
    for (int offset = 0; offset < 5; offset++)
    {
        canvas.drawLine(cx + r / 2 + offset, cy - h / 2, cx + offset, cy, color);
    }

    // Bottom-left triangle (lower half, left side)
    for (int offset = 0; offset < 5; offset++)
    {
        canvas.drawLine(cx + offset, cy, cx - r / 2 + offset, cy + h / 2, color);
    }

    // Bottom-right triangle (lower half, right side)
    for (int offset = 0; offset < 5; offset++)
    {
        canvas.drawLine(cx + offset, cy, cx + r / 2 + offset, cy + h / 2, color);
    }
    for (int offset = 0; offset < 5; offset++)
    {
        canvas.drawLine(cx + r / 2 + offset, cy + h / 2, cx + offset, cy + h, color);
    }
}

void Display::showRoute()
{

    showingRoute = true;
}

void Display::drawRoute()
{
    drawWpts();
    drawDirectionDot();
    drawWptDistanceBar();
    drawSatelliteFixBar();
    sprite.pushSprite(&canvas, 0, 0);
}

void Display::drawWpts(int offset)
{

    // Draw directly on the main canvas to avoid large intermediate sprite allocation.
    sprite.fillScreen(TFT_BLACK);

    const int centerX = (canvas.width() / 2) + CIRCLE_CENTER_X_OFFSET;
    const int centerY = (canvas.height() / 2) + offset;

    /*
    sprite.setTextDatum(MC_DATUM);
    sprite.setFont(&fonts::FreeSansBold24pt7b);
    sprite.setTextColor(TFT_WHITE);
    */

    for (int rowOffset = -2; rowOffset <= 2; rowOffset++)
    {
        const int rowIndex = route.selectedWptIndex + rowOffset;

        if (route.isValidRouteIndex(rowIndex))
        {
            const int rowCenterY = centerY + (rowOffset * CIRCLE_CENTER_OFFSET);

            if (rowIndex == route.nextWptIndex)
            {
                drawRouteCircle(centerX, rowCenterY, ROUTE_CIRCLE_SELECT_COLOR);
            }
            else
            {
                drawRouteCircle(centerX, rowCenterY, ROUTE_CIRCLE_COLOR);
            }
            sprite.setTextDatum(MC_DATUM);
            sprite.setFont(&fonts::FreeSansBold24pt7b);
            sprite.setTextColor(TFT_WHITE);
            sprite.drawNumber(route.jsonRoute[rowIndex]["wpt"], centerX, rowCenterY);
            double dist = route.jsonRoute[rowIndex]["dist"];
            String distStr = String((dist / 1000), 2) + " km";
            drawDistanceLabel(distStr.c_str(), centerX, rowCenterY);
        }
    }
}

void Display::drawRouteCircle(int centerX, int centerY, uint16_t color)
{
    sprite.fillCircle(centerX, centerY, CIRCLE_SIZE + CIRCLE_PEN_SIZE, color);
    sprite.fillCircle(centerX, centerY, CIRCLE_SIZE, TFT_BLACK);
}

void Display::drawDistanceLabel(std::string distance, int centerX, int centerY)
{
    int labelX = 4;
    int labelY = centerY + (CIRCLE_CENTER_OFFSET / 2);

    sprite.setFont(&fonts::FreeSansBold12pt7b);
    sprite.setTextColor(TFT_WHITE, TFT_BLACK);
    sprite.setTextDatum(ML_DATUM);
    sprite.setTextColor(TFT_WHITE, TFT_BLACK);
    sprite.drawString(distance.c_str(), labelX, labelY);
}

void Display::drawDirectionDot()
{
    double heading;
    // first test if selected wpt if visible
    if (route.nextWptIndex == route.selectedWptIndex)
    {
        int centerX = (canvas.width() / 2) + CIRCLE_CENTER_X_OFFSET;
        int centerY = (canvas.height() / 2);

        heading = route.getHeading();

        float radius = CIRCLE_SIZE + (CIRCLE_PEN_SIZE / 2.0);

        int posX = centerX + lround(sin(radians(heading)) * radius);
        int posY = centerY - lround(cos(radians(heading)) * radius);

        sprite.fillCircle(posX, posY, OUTER_DOT_SIZE, TFT_WHITE);
        if (!gps.hasValidCog && blinker)

        {
            sprite.fillCircle(posX, posY, INNER_DOT_SIZE, TFT_BLACK);
        }
        /*
        DBG_EXT(DBG_INFO, "heading = %f", heading);
        DBG_EXT(DBG_INFO, "activeWptIdx = %d", route.activeWptIndex);
        DBG_EXT(DBG_INFO, "activeTptIdx = %d", route.activeTptIndex);
        DBG_EXT(DBG_INFO, "targetLat = %f", route.targetLat);
        DBG_EXT(DBG_INFO, "targetLon = %f", route.targetLon);
        DBG_EXT(DBG_INFO, "targetDist = %f", route.targetDist);  
        */                  
    }
}

void Display::drawWptDistanceBar()
{
    int percent = route.getWptDistancePct();
    int sizeX = WPT_DISTANCE_BAR_WIDTH;
    int sizeY = canvas.height() * percent / 100;
    int posX = canvas.width() - WPT_DISTANCE_BAR_WIDTH - WPT_DISTANCE_BAR_X_OFFSET;
    int posY = canvas.height() - sizeY;

    /*
    Serial.print("percent=");
    Serial.println(percent);
    Serial.print("sizeY=");
    Serial.println(sizeY);
    */
    sprite.fillRect(posX, 0, sizeX, canvas.height(), TFT_GREY);
    sprite.fillRect(posX, posY, sizeX, sizeY, TFT_WHITE);
}



void Display::drawSatelliteFixBar()
{
    if (!gps.satelliteFix())
    {
        if (blinker)
        {
            sprite.fillRect(0, 0, canvas.width(), SATELLITE_FIX_BAR_HEIGHT, TFT_RED);
        }
    }
}

void Display::scrollUp(int speed)
{
    if (route.selectedWptIndex < (route.jsonRoute.size() - 1))
    {
        for (int offset = 0; offset < CIRCLE_CENTER_OFFSET; offset += speed)
        {
            drawWpts(-offset);
            sprite.pushSprite(&canvas, 0, 0);
        }
        route.selectedWptIndex++;
    }
}

void Display::scrollDown(int speed)
{
    if (route.selectedWptIndex > 0)
    {
        for (int offset = 0; offset < CIRCLE_CENTER_OFFSET; offset += speed)
        {
            drawWpts(+offset);
            sprite.pushSprite(&canvas, 0, 0);
        }
        route.selectedWptIndex--;
    }
}

void Display::scrollIntoView()
{
    if (route.nextWptIndex > route.selectedWptIndex)
    {
        while (route.nextWptIndex != route.selectedWptIndex)
            scrollUp(FAST_SCROLL_SPEED);
    }
    else
    {
        while (route.nextWptIndex != route.selectedWptIndex)
            scrollDown(FAST_SCROLL_SPEED);
    }
}

bool Display::inView()
{
    return (route.nextWptIndex == route.selectedWptIndex);
}

void Display::sleep()
{
    canvas.sleep();
}

bool Display::initDone()
{
    return initialized;
}