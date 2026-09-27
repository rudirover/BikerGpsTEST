#include "route.hpp"

Route route;

Route::Route() {};

void Route::init()
{
    if (!LittleFS.begin())
    {
        Serial.print("LittleFS failed");
    }
    initialized = true;
}

bool Route::exists()
{
    return LittleFS.exists(SAVED_ROUTE);
}

void Route::save(std::string receivedRoute)
{
    File file = LittleFS.open(SAVED_ROUTE, "w");

    if (file)
        file.print(receivedRoute.c_str());
}

void Route::prepare()
{
    File file = LittleFS.open(SAVED_ROUTE, "r");

    if (file)
        deserializeJson(jsonRoute, file);
    /*
    Serial.println("--- JSON ROUTE ---");
    serializeJsonPretty(jsonRoute, Serial);
    Serial.println("/n------------------");
    */

    activeWptIndex = 0;
    activeTptIndex = 0;
    selectedWptIndex = 0;
    nextWptIndex = 0;
    startWptReached = false;
    travelDist = 0;
}

bool Route::isValidRouteIndex(int index)
{
    return (index >= 0) && (index < static_cast<int>(jsonRoute.size()));
}

double Route::getHeading()
{
    double bearing = gps.bearing(targetLat, targetLon);

    return bearing; // - cog;
}

int Route::getWptDistancePct()
{
    int pct = (int)((travelDist * 100) / targetDist);

    //DBG_EXT(DBG_INFO, "realDist = %f", travelDist);
    //DBG_EXT(DBG_INFO, "targetDist = %f", (targetDist * 1000));
    //DBG_EXT(DBG_INFO, "percentage = %d", pct);

    return pct;
}

void Route::run()
{
    double distance;
    bool wptReached = false;

   // DBG_EXT(DBG_INFO, "startWptReached = %d", startWptReached);
    //DBG_EXT(DBG_INFO, "activeWptIndex = %d", activeTptIndex);

    if (!startWptReached)
    {
        targetLat = jsonRoute[activeWptIndex]["lat"];
        targetLon = jsonRoute[activeWptIndex]["lon"];
        targetDist = 0;
        distance = gps.distance(targetLat, targetLon);
        if (distance < AUTO_WPT_DIST_METERS)
        {
          //buzzer.wptBeep();            
            travelDist = 0;
            nextWptIndex++;
            startWptReached = true;
        }
    }

    if (startWptReached)
    {
        // check if we reached a wpt
        targetLat = jsonRoute[nextWptIndex]["lat"];
        targetLon = jsonRoute[nextWptIndex]["lon"];
        distance = gps.distance(targetLat, targetLon);
        if (distance < AUTO_WPT_DIST_METERS)
        {
            //buzzer.wptBeep();
            travelDist = 0;
            activeTptIndex = 0;
            activeWptIndex++;
            nextWptIndex++;

            // still need to decide what to do if end reached
            targetLat = jsonRoute[activeWptIndex]["tpt"][activeTptIndex]["lat"];
            targetLon = jsonRoute[activeWptIndex]["tpt"][activeTptIndex]["lon"];
            targetDist = jsonRoute[activeWptIndex]["dist"];

            wptReached = true;
        }

        // check if we reached a tpt
        if (!wptReached)
        {
            targetLat = jsonRoute[activeWptIndex]["tpt"][activeTptIndex]["lat"];
            targetLon = jsonRoute[activeWptIndex]["tpt"][activeTptIndex]["lon"];
            distance = gps.distance(targetLat, targetLon);
            if (distance < AUTO_TPT_DIST_METERS)
            {
                //buzzer.tptBeep();
                travelDist = travelDist + (double)jsonRoute[activeWptIndex]["tpt"][activeTptIndex]["dist"];
                activeTptIndex++;
                if (activeTptIndex >= jsonRoute[activeWptIndex]["tpt"].size())
                {
                    travelDist = 0;
                    activeTptIndex = 0;
                    activeWptIndex++;
                    nextWptIndex++;
                }

                // still need to decide what to do if end reached
                targetLat = jsonRoute[activeWptIndex]["tpt"][activeTptIndex]["lat"];
                targetLon = jsonRoute[activeWptIndex]["tpt"][activeTptIndex]["lon"];
                targetDist = jsonRoute[activeWptIndex]["dist"];
            }
        }
    }
}

void Route::setRoute()
{
    activeWptIndex = selectedWptIndex;
    activeTptIndex = 0;
    nextWptIndex = activeWptIndex;
    startWptReached = false;
    travelDist = 0;
}

bool Route::initDone()
{
    return initialized;
}