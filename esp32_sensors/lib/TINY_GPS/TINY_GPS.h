#ifndef TINY_GPS
#define TINY_GPS

#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

class GPSLocalization
{
public:
    GPSLocalization(HardwareSerial &serialPort, uint32_t baudRate = 115200,
                     int rxPin = 16, int txPin = 17);

    void update();
    
    void setReference(int sat_threshold, double max_variation, int n_init_samples);

    double getLatitude();
    double getLongitude();
    int getSatellites();

    void getXY(double &x, double &y);

private:
    HardwareSerial &gpsSerial;
    TinyGPSPlus gps;

    double ref_lat;
    double ref_lon;
    bool ref_ready;

    double degreesToRadians(double deg);
    void latLonToXY(double latRef, double lonRef, double lat, double lon, double &x, double &y);
};

#endif
