#include "TINY_GPS.h"
#include <math.h>

GPSLocalization::GPSLocalization(HardwareSerial &serialPort, uint32_t baudRate,
                                   int rxPin, int txPin)
    : gpsSerial(serialPort), ref_lat(0.0), ref_lon(0.0), ref_ready(false)
{
    gpsSerial.begin(baudRate, SERIAL_8N1, rxPin, txPin);
}

void GPSLocalization::update()
{
    while (gpsSerial.available() > 0)
    {
        gps.encode(gpsSerial.read());
    }
}

double GPSLocalization::getLatitude()
{
    return gps.location.isValid() ? gps.location.lat() : 0.0;
}

double GPSLocalization::getLongitude()
{
    return gps.location.isValid() ? gps.location.lng() : 0.0;
}

int GPSLocalization::getSatellites()
{
    return gps.satellites.value();
}

void GPSLocalization::setReference(int sat_threshold, double max_variation, int n_init_samples)
{
    double lat_sum = 0.0;
    double lon_sum = 0.0;
    double prev_lat = 0.0, prev_lon = 0.0;

    int samples = 0;

    while (!ref_ready)
    {
        update();

        if (!gps.location.isUpdated())
            continue;

        double lat = gps.location.lat();
        double lon = gps.location.lng();
        int sats = gps.satellites.value();

        bool stable = (sats >= sat_threshold) &&
                      (abs(lat - prev_lat) < max_variation) &&
                      (abs(lon - prev_lon) < max_variation);

        if (stable)
        {
            lat_sum += lat;
            lon_sum += lon;
            samples++;

            if (samples >= n_init_samples)
            {
                ref_lat = lat_sum / samples;
                ref_lon = lon_sum / samples;
                ref_ready = true;
            }
        }
        else
        {
            lat_sum = 0.0;
            lon_sum = 0.0;
            samples = 0;
        }

        prev_lat = lat;
        prev_lon = lon;
    }
}

void GPSLocalization::getXY(double &x, double &y)
{
    if (!ref_ready)
    {
        x = y = 0.0;
        return;
    }

    double lat = getLatitude();
    double lon = getLongitude();

    latLonToXY(ref_lat, ref_lon, lat, lon, x, y);
}

double GPSLocalization::degreesToRadians(double deg)
{
    return deg * M_PI / 180.0;
}

void GPSLocalization::latLonToXY(double latRef, double lonRef,
                                  double lat, double lon,
                                  double &x, double &y)
{
    const double R = 6371000.0;

    double dLat = degreesToRadians(lat - latRef);
    double dLon = degreesToRadians(lon - lonRef);
    double latRefRad = degreesToRadians(latRef);

    x = dLon * R * cos(latRefRad);
    y = dLat * R;
}
