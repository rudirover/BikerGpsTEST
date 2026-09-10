#include "gps.hpp"

Gps gps;

#ifdef SIMULATION
Gps::Gps() {};
#else
Gps::Gps() : gpsComm(1) {};
#endif

void Gps::init()
{
  gpio_hold_dis((gpio_num_t)GPS_ENABLE_PIN);

  // 2. TURN POWER ON: Driving the NPN Base HIGH connects the GPS ground to system ground
  pinMode(GPS_ENABLE_PIN, OUTPUT);
  digitalWrite(GPS_ENABLE_PIN, HIGH);

  // 3. Re-initialize your serial port pins to talk to the module
  pinMode(GPS_RX_PIN, INPUT);
  pinMode(GPS_TX_PIN, OUTPUT);

  // Give the ATGM336H 200ms to boot its radio frontend before sending commands
  delay(200);

  tickTime = millis();

#ifdef SIMULATION
  Serial.setRxBufferSize(1024);
#else
  gpsComm.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.printf("[GPS] ATGM336H started on RX=%d TX=%d at %d baud\n", GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD_RATE);
  gpsComm.print("xxx"); // dummy to wake up
#endif

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
  // Serial.print("Bytes available from GPS = ");
  // Serial.println(gpsComm.available());
  while (gpsComm.available() > 0)
  // while (Serial.available() > 0)
  {
    tinyGps.encode(gpsComm.read());
  }

  if (tinyGps.failedChecksum())
    Serial.println("Checksum failed!!!!!");

  // 3. Update state IMMEDIATELY when TinyGPS finishes decoding a field
  if (tinyGps.location.isUpdated() && tinyGps.location.isValid())
  {
    latitude = tinyGps.location.lat();
    longitude = tinyGps.location.lng();
  }
#endif

  if (tinyGps.speed.isUpdated() && tinyGps.speed.isValid())
  {
    speedKmph = tinyGps.speed.kmph();
  }

  if (tinyGps.course.isUpdated())
  {
    if (tinyGps.course.isValid() && speedKmph >= MIN_VALID_COG_SPEED)
    {
      cogDegrees = tinyGps.course.deg();
      cogValid = true;
    }
    else
    {
      cogValid = false;
    }
  }

  // 4. Keep the timer ONLY for periodic debug logging output
  if ((millis() - tickTime) >= 1000)
  {
    tickTime = millis();
    if (tinyGps.satellites.value() !=0)
    {
      Serial.printf("[GPS] Lat: %.6f, Lng: %.6f, Speed: %.2f km/h, COG: %s, Sats: %d, SatFix: %d\n",
                    latitude,
                    longitude,
                    speedKmph,
                    cogValid ? String(cogDegrees, 1).c_str() : "N/A",
                    tinyGps.satellites.value(),
                    tinyGps.satellites.isValid());
    }
    else
    {
      DBG_EXT(DBG_INFO, " Waiting for valid gps fix...");
      // buzzer.tptBeep();
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

bool Gps::satellitesIsAvailable()
{
  return ((tinyGps.satellites.value() > 0) && (tinyGps.satellites.age() < 2000));
}

bool Gps::locationIsAvailable()
{
  return tinyGps.location.isValid();
}

bool Gps::courseIsAvailable()
{
  return tinyGps.course.isValid();
}

bool Gps::speedIsAvailable()
{
  return tinyGps.speed.isValid();
}

bool Gps::cogIsAvailable()
{
  return cogValid;
}

double Gps::distance(double targetLat, double targetLon)
{
  return tinyGps.distanceBetween(latitude, longitude, targetLat, targetLon);
}

void Gps::sleep()
{
  // 1. TURN POWER OFF: Driving the NPN Base LOW breaks the ground connection (0mA draw)
  pinMode(GPS_ENABLE_PIN, OUTPUT);
  digitalWrite(GPS_ENABLE_PIN, LOW);

  // 2. CRUCIAL FOR LOW-SIDE SWITCHING: Disconnect UART pins!
  // Because the GPS ground is floating, if the ESP32 keeps its TX pin HIGH,
  // current will flow backward through the serial lines and "phantom power" the chip.
  pinMode(GPS_RX_PIN, INPUT);
  pinMode(GPS_TX_PIN, INPUT);

  // 3. LOCK THE PIN LOW: Tell the ESP32-S3 to hold the pin LOW during deep sleep
  gpio_hold_en((gpio_num_t)GPS_ENABLE_PIN);
  gpio_deep_sleep_hold_en();
}
