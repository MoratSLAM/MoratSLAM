#ifndef TINY_GPS
#define TINY_GPS

#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

class GPS_localization
{
    public:
        GPS_localization(HardwareSerial &serialPort, uint32_t baudRate = 115200,
                        int rxPin = 16, int txPin = 17);

        void update();
        
        void set_reference(int sat_threshold, double max_variation, int n_init_samples);

        double get_latitude();
        double get_longitude();
        int get_satellites();

        void get_xy(double &x, double &y);

    private:
        HardwareSerial &gpsSerial;
        TinyGPSPlus gps;

        double ref_lat;
        double ref_lon;
        bool ref_ready;

        double degrees_to_radians(double deg);
        void lat_lon_to_xy(double latRef, double lonRef, double lat, double lon, double &x, double &y);
};

#endif
