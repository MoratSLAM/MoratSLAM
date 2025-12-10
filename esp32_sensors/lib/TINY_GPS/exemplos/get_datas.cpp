#include <TINY_GPS.h>

HardwareSerial GPS_Serial(2);
GPSLocalization gpsLoc(GPS_Serial, 115200, 16, 17);

void setup()
{
    Serial.begin(115200);

    Serial.println("Definindo referência...");
    gpsLoc.setReference(10, 0.00001, 50);
    Serial.println("Referência OK");
}

void loop()
{
    gpsLoc.update();

    double x, y;
    gpsLoc.getXY(x, y);

    Serial.print("Lat: "); Serial.println(gpsLoc.getLatitude(), 7);
    Serial.print("Lon: "); Serial.println(gpsLoc.getLongitude(), 7);
    Serial.print("Sats: "); Serial.println(gpsLoc.getSatellites());
    Serial.print("X: "); Serial.println(x, 3);
    Serial.print("Y: "); Serial.println(y, 3);
    Serial.println("-------------");

    delay(200);
}
