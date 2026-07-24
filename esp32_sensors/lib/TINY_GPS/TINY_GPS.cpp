#include "TINY_GPS.h"
#include <math.h>

// Constructor for the GPS_localization class
GPS_localization::GPS_localization(HardwareSerial &serialPort, uint32_t baudRate,
                                   int rxPin, int txPin)
    : gpsSerial(serialPort), ref_lat(0.0), ref_lon(0.0), ref_ready(false)
{
    gpsSerial.begin(baudRate, SERIAL_8N1, rxPin, txPin);
}

// Update the GPS data by reading from the serial port and encoding it
void GPS_localization::update()
{
    while (gpsSerial.available() > 0)
    {
        gps.encode(gpsSerial.read());
    }
}

// Get the current latitude, ensuring that it is valid and within the acceptable range
double GPS_localization::get_latitude()
{
    // Check if the GPS location is valid before accessing it
    if(gps.location.isValid())
    {
        double current_lat = gps.location.lat();

        // If this is the first reading, set the last valid latitude to the current reading
        if(first_reading_lat && current_lat != 0.0)
        {
            last_valid_lat = current_lat;
            first_reading_lat = false;
        }
        // If the current reading is valid and within the acceptable range, update the last valid latitude
        else if(fabs(current_lat - last_valid_lat) < MAX_DELTA && current_lat != 0.0)
        {
            last_valid_lat = current_lat;
        }
    }
    return last_valid_lat;
}

// Get the current longitude, ensuring that it is valid and within the acceptable range
double GPS_localization::get_longitude()
{
    if(gps.location.isValid())
    {
        double current_lon = gps.location.lng();

        if(first_reading_lon && current_lon != 0.0)
        {
            last_valid_lon = current_lon;
            first_reading_lon = false;
        }
        else if(fabs(current_lon - last_valid_lon) < MAX_DELTA && current_lon != 0.0)
        {
            last_valid_lon = current_lon;
        }
    }
    return last_valid_lon;
}

// Get the number of satellites currently in view
int GPS_localization::get_satellites()
{
    return gps.satellites.value();
}

// Set the reference location based on a series of GPS readings, ensuring stability and accuracy
void GPS_localization::set_reference(int sat_threshold, double max_variation, int n_init_samples)
{
    double lat_sum = 0.0;
    double lon_sum = 0.0;
    double prev_lat = 0.0, prev_lon = 0.0;

    int samples = 0;

    while (!ref_ready)
    {
        update();
        if (!gps.location.isUpdated())
        {
            ref_lat =  0;
            ref_lon = 0;
            for (int i = 0; i < 10; i++)
            {
                digitalWrite(2, LOW);
                delay(50);
                digitalWrite(2, HIGH);
                delay(800);
            }
            break;
        }

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

// Get the Cartesian coordinates (x, y) relative to the reference location
void GPS_localization::get_xy(double &x, double &y)
{
    if (!ref_ready)
    {
        x = y = 0.0;
        return;
    }

    double lat = get_latitude();
    double lon = get_longitude();

    lat_lon_to_xy(ref_lat, ref_lon, lat, lon, x, y);
}

// Convert degrees to radians
double GPS_localization::degrees_to_radians(double deg)
{
    return deg * M_PI / 180.0;
}

// Convert latitude and longitude to Cartesian coordinates (x, y) relative to a reference point
void GPS_localization::lat_lon_to_xy(double latRef, double lonRef, double lat, double lon, double &x, double &y)
{
    const double R = 6371000.0;

    double dLat = degrees_to_radians(lat - latRef);
    double dLon = degrees_to_radians(lon - lonRef);
    double latRefRad = degrees_to_radians(latRef);

    x = dLon * R * cos(latRefRad);
    y = dLat * R;
}
