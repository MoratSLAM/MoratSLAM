#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <math.h>

// Instância do TinyGPS++
TinyGPSPlus gps;

// Define UART2 para GPS (RX=16, TX=17)
HardwareSerial GPS(2);

// Referência
double ref_lat = 0.0;
double ref_lon = 0.0;
bool ref_set = false;

// Parâmetros de inicialização da referência
const int N_INIT_SAMPLES = 500;  // número de amostras para média inicial
const int SAT_THRESHOLD  = 10;  // exigência mínima de satélites
const double MAX_STB_VARIATION = 0.000001;

// Funções
double degreesToRadians(double degrees);
void latLonToXY(double lat_ref, double lon_ref, double lat, double lon, double &x, double &y);
void set_reference(); // função para definir referência

void setup()
{
    Serial.begin(115200);
    Serial.println("Iniciando GPS (UART2) com TinyGPS++...");
    GPS.begin(115200, SERIAL_8N1, 16, 17);
    Serial.println("Definindo posição de referência...");
    
    set_reference(); // define a referência inicial
    Serial.println("Referência definida, iniciando loop principal.");
}

void loop()
{
    while (GPS.available() > 0) 
    {
        char c = GPS.read();
        if (gps.encode(c))
        {
            if (gps.location.isUpdated())
            {
                double lat = gps.location.lat();
                double lon = gps.location.lng();
                int sats = gps.satellites.value();

                Serial.print("Lat: "); Serial.print(lat, 7);
                Serial.print(" Lon: "); Serial.print(lon, 7);
                Serial.print(" Sats: "); Serial.print(sats);

                if (ref_set)
                {
                    double x, y;
                    latLonToXY(ref_lat, ref_lon, lat, lon, x, y);
                    Serial.print("  X : "); Serial.print(x, 3);
                    Serial.print("  Y : "); Serial.println(y, 3);
                    Serial.println("-----------------------------");
                }
            }
        }
    }
}

/** Função para definir referência inicial **/
void set_reference()
{
    double lat = 0.0;
    double previus_lat = 0.0;
    double lat_sum = 0.0;
    double lon = 0.0;
    double previus_lon = 0.0;
    double lon_sum = 0.0;
    int sats = 0;
    int sample_count = 0;

    while (!ref_set)
    {
        while (GPS.available() > 0)
        {
            char c = GPS.read();
            if (gps.encode(c))
            {
                if (gps.location.isUpdated())
                {
                    lat = gps.location.lat();
                    lon = gps.location.lng();
                    sats = gps.satellites.value();

                    if (sats >= SAT_THRESHOLD && abs(lat - previus_lat) < MAX_STB_VARIATION && abs(lon - previus_lon) < MAX_STB_VARIATION)
                    {
                        lat_sum += lat;
                        lon_sum += lon;
                        sample_count++;

                        Serial.print("Amostra inicial aceita (");
                        Serial.print(sample_count);
                        Serial.print("/");
                        Serial.print(N_INIT_SAMPLES);
                        Serial.println(").");

                        if (sample_count >= N_INIT_SAMPLES)
                        {
                            ref_lat = lat_sum / sample_count;
                            ref_lon = lon_sum / sample_count;
                            ref_set = true;

                            Serial.println("Referência definida (média das amostras):");
                            Serial.print("ref_lat = "); Serial.println(ref_lat, 8);
                            Serial.print("ref_lon = "); Serial.println(ref_lon, 8);
                            break; // sai do while GPS.available
                        }
                    }
                    else
                    {
                        Serial.println("Leitura inicial rejeitada (satélites insuficientes ou variação maior do que esperada): ");
                        Serial.print("Sats:");
                        Serial.println(sats);
                        Serial.print("Variação Lat:");
                        Serial.println(lat);
                        Serial.print("Variação Lon:");
                        Serial.println(lon);

                        lat_sum = 0;
                        lon_sum = 0;
                        sample_count = 0;
                    }
                    previus_lat = lat;
                    previus_lon = lon;
                }
            }
        }
    }
}

/** Converte graus em radianos **/
double degreesToRadians(double degrees)
{
    return degrees * M_PI / 180.0;
}

/** Converte lat/lon em X/Y relativos (metros) **/
void latLonToXY(double lat_ref, double lon_ref, double lat, double lon, double &x, double &y) 
{
    const double earth_radius = 6371000.0; // metros
    double dLat = degreesToRadians(lat - lat_ref);
    double dLon = degreesToRadians(lon - lon_ref);
    double lat_ref_rad = degreesToRadians(lat_ref);
    x = dLon * earth_radius * cos(lat_ref_rad);
    y = dLat * earth_radius;
}
