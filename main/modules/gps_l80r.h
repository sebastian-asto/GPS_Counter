#ifndef GPS_L80R_H
#define GPS_L80R_H

#include <stdbool.h>
#include <stdint.h>

// ==================== CONFIGURACIÓN DE HARDWARE ====================
#define GPS_UART_NUM      UART_NUM_1
#define GPS_TX_PIN        GPIO_NUM_25
#define GPS_RX_PIN        GPIO_NUM_26
#define GPS_RST_PIN       GPIO_NUM_23
#define GPS_BUFFER_SIZE   1024

// ==================== ESTRUCTURAS DE DATOS ====================

// Datos extraídos de la trama GPRMC
typedef struct {
    double latitude;        // Latitud en grados decimales
    double longitude;       // Longitud en grados decimales
    double speed_kmh;       // Velocidad (km/h)
    double course_deg;      // Rumbo (grados)
    char time[16];          // Hora UTC (hhmmss)
    char date[8];           // Fecha (ddmmyy)
    bool valid;             // True si el fix es válido ('A')
} gps_data_t;

// Datos extraídos de la trama GPGGA
typedef struct {
    double altitude;        // Altitud sobre el nivel del mar (m)
    int satellites;         // Satélites usados
    double hdop;            // Precisión horizontal
} gps_quality_t;

// Filtro Kalman unidimensional (para velocidad)
typedef struct {
    double estimate;
    double P;
    double Q;
    double R;
} kalman_t;

// ==================== FUNCIONES PÚBLICAS ====================
void init_uart_gps_l80r(void);
void mostrar_data_NMEA(void);
void mostrar_data_NMEA_filtrada(void);

// Parsers
bool gps_parse_gprmc(const char *nmea_sentence, gps_data_t *gps);
bool gps_parse_gpgga(const char *nmea_sentence, gps_quality_t *quality);

// Checksum
bool nmea_verify_checksum(const char *sentence);

// Kalman
void kalman_init(kalman_t *kf, double Q, double R);
double kalman_update(kalman_t *kf, double measurement);

// Tareas
void task_gps_read_and_parse(void *pvParameters);


// SETTERS
void gps_set_umbral_movimiento(double v);

// GETTERS
double gps_get_speed_kmh(void);
double gps_get_umbral_movimiento(void);
double gps_get_latitude(void);
double gps_get_longitude(void);
double gps_get_altitude(void);
int gps_get_satellites(void);
double gps_get_hdop(void);
bool gps_is_valid(void);

// Configuración de mensajes NMEA
void gps_set_update_rate_hz(int hz);
void gps_enable_rmc_gga_only(void);
void gps_enable_only_rmc(void);
void gps_enable_only_gga(void);

void gps_restore_default(void);

#endif // GPS_L80R_H
