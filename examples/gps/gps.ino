#include "Quectel_LTE.h"
#include <TinyGPS++.h>  // https://github.com/mikalhart/TinyGPSPlus

// The TinyGPS++ object
TinyGPSPlus gps;

Quectel_LTE LTE;

void setup() {  
  Serial.begin(115200);  
  delay(4000);

  Serial.println("## Demo GPS receive");
  
  LTE.initialize();
  if(!LTE.isAlive(20000))
  {
    Serial.println("### Module not alived");
    return;
  }  

  if(!LTE.gpsOn()) 
  {
    Serial.println("## ERROOR:Can't turn on GPS"); 
    while(true);
  }
}

void loop() {
  char gpsStream[128] = {'\0'};
  uint16_t index = 0;

  while(!LTE.gpsLocRawData(gpsStream, sizeof(gpsStream)))
  {
    Serial.println("## ERROOR:GPS not fixed");
    delay(2000);
  }
      
  while ('\0' != gpsStream[index])
    if (gps.encode(gpsStream[index++]))
      displayInfo();

  Serial.println();
  delay(2000);
}


void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}

