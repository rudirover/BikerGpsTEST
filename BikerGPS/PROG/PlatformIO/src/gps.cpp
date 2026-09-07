#include "gps.hpp"


Gps gps;

#ifdef SIMULATION
Gps::Gps() {};
#else
Gps::Gps() : gpsComm(1) {};
#endif

// Gps::Gps() {};

void Gps::init()
{
  pinMode(GPS_ENABLE_PIN, OUTPUT);
  digitalWrite(GPS_ENABLE_PIN, HIGH);

  tickTime = millis();
  hasValidLocation = false;
  hasValidCog = false;

#ifdef SIMULATION
  Serial.setRxBufferSize(1024);
#else
  gpsComm.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.printf("[GPS] ATGM336H started on RX=%d TX=%d at %d baud\n", GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD_RATE);
#endif

  initialized = true;
}

void Gps::run()
{
#ifdef SIMULATION
  // 2. Read all available serial bytes continuously
  while (Serial.available() > 0)
  // while (Serial.available() > 0)
  {
    tinyGps.encode(Serial.read());
  }

  // 3. Update state IMMEDIATELY when TinyGPS finishes decoding a field
  if (tinyGps.location.isUpdated())
  {
    hasValidLocation = tinyGps.location.isValid();
    if (hasValidLocation)
    {
      latitude = tinyGps.location.lat();
      longitude = tinyGps.location.lng();
    }
  }
#else
  // 2. Read all available serial bytes continuously
  while (gpsComm.available() > 0)
  // while (Serial.available() > 0)
  {
    tinyGps.encode(gpsComm.read());
  }

  if (tinyGps.failedChecksum()) Serial.println("Checksum failed!!!!!");

  // 3. Update state IMMEDIATELY when TinyGPS finishes decoding a field
  if (tinyGps.location.isUpdated())
  {
    hasValidLocation = tinyGps.location.isValid();
    if (hasValidLocation)
    {
      latitude = tinyGps.location.lat();
      longitude = tinyGps.location.lng();
    }
  }
#endif

  if (tinyGps.speed.isUpdated())
  {
    speedKmph = tinyGps.speed.isValid() ? tinyGps.speed.kmph() : 0.0;
  }

  if (tinyGps.course.isUpdated())
  {
    if (tinyGps.course.isValid() && speedKmph >= MIN_VALID_COG_SPEED)
    {
      cogDegrees = tinyGps.course.deg();
      hasValidCog = true;
    }
    else
    {
      hasValidCog = false;
    }
  }

  // 4. Keep the timer ONLY for periodic debug logging output
  if ((millis() - tickTime) >= 1000)
  {
    tickTime = millis();

    if (hasValidLocation)
    {
      Serial.printf("[GPS] Lat: %.6f, Lng: %.6f, Speed: %.2f km/h, COG: %s, Sats: %d\n",
                    latitude,
                    longitude,
                    speedKmph,
                    hasValidCog ? String(cogDegrees, 1).c_str() : "N/A",
                    tinyGps.satellites.isValid() ? tinyGps.satellites.value() : 0);
    }
    else
    {
      //DBG_EXT(DBG_INFO, " Waiting for valid gps fix...");
      //buzzer.tptBeep();

    }
  }
}

double Gps::bearing(double targetLat, double targetLon)
{
  // 1. Check if heading data is valid (requires motion)
  if (!tinyGps.course.isValid())
  {
    return 0.0; // Return 0 or handle invalid state gracefully
  }

  // 2. Calculate absolute headings (0 to 360)
  double targetHeading = tinyGps.courseTo(latitude, longitude, targetLat, targetLon);
  double currentHeading = tinyGps.course.deg();

  // 3. Compute relative angle (-180 to +180)
  double bearing = targetHeading - currentHeading;

  if (bearing > 180.0)
  {
    bearing -= 360.0;
  }
  else if (bearing < -180.0)
  {
    bearing += 360.0;
  }

  return bearing;
}

bool Gps::satelliteFix()
{
  return tinyGps.location.isValid();
}

double Gps::distance(double targetLat, double targetLon)
{
  return tinyGps.distanceBetween(latitude, longitude, targetLat, targetLon);
}

bool Gps::initDone()
{
  return initialized;
}

void Gps::sleep()
{
  digitalWrite(GPS_ENABLE_PIN, LOW);
}