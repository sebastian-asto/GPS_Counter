#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/uart.h>

#include "modules/gps_l80r.h"
#include "screens/display_7seg.h"

static const char *TAG = "L80-R";

static gps_data_t gps; // estructura para almacenar datos GPS
static gps_quality_t quality;

//parametros de calibracion 
static double umbral_movimiento_kmh = 5.0;// velocidad minima para considerar movimiento

// ===========================================================
//  CONFIGURACIÓN UART
// ===========================================================
void init_uart_gps_l80r(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    gpio_config_t reset_cfg = {
        .pin_bit_mask = (1ULL << GPS_RST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&reset_cfg);
    gpio_set_level(GPS_RST_PIN, 1);

    uart_param_config(GPS_UART_NUM, &uart_config);
    uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(GPS_UART_NUM, GPS_BUFFER_SIZE * 2, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "UART GPS inicializado correctamente");
}
// ===========================================================
//  FUNCIONES DE ENVÍO DE COMANDOS AL GPS
// ===========================================================
static void gps_send_cmd(const char *cmd)
{
    const char *ending = "\r\n";
    uart_write_bytes(GPS_UART_NUM, cmd, strlen(cmd));
    uart_write_bytes(GPS_UART_NUM, ending, strlen(ending));

    ESP_LOGI(TAG, "CMD enviado: %s", cmd);
}

// ===========================================================
//  RESTAURAR CONFIGURACIÓN DEFAULT
// ===========================================================
void gps_restore_default(void)
{
    // Frecuencia 1 Hz (default)
    gps_send_cmd("PMTK300,1000,0,0,0,0");
    gps_send_cmd("PMTK220,1000");

    // Activar TODAS las tramas NMEA (modo fábrica)
    // -1 significa "activar todas las sentencias"
    gps_send_cmd("PMTK314,-1");

    ESP_LOGI(TAG, "🔄 GPS restaurado a configuración DEFAULT (1 Hz + todas las tramas)");

    // Importante: limpiar buffer y esperar
    uart_flush(GPS_UART_NUM);
    vTaskDelay(pdMS_TO_TICKS(1200));
}

// ===========================================================
//  AJUSTE DE FRECUENCIA DE ACTUALIZACIÓN
// ===========================================================
void gps_set_update_rate_hz(int hz)
{
    char cmd1[32];
    char cmd2[32];

    switch(hz){
        case 1:
            strcpy(cmd1, "PMTK300,1000,0,0,0,0");
            strcpy(cmd2, "PMTK220,1000");
            break;
        case 2:
            strcpy(cmd1, "PMTK300,500,0,0,0,0");
            strcpy(cmd2, "PMTK220,500");
            break;
        case 3:
            strcpy(cmd1, "PMTK300,333,0,0,0,0");
            strcpy(cmd2, "PMTK220,333");
            break;
        case 4:
            strcpy(cmd1, "PMTK300,250,0,0,0,0");
            strcpy(cmd2, "PMTK220,250");
            break;
        case 5:
            strcpy(cmd1, "PMTK300,200,0,0,0,0");
            strcpy(cmd2, "PMTK220,200");
            break;
        default:
            ESP_LOGE(TAG, "❌ Frecuencia no válida. Use 1-5 Hz");
            return;
    }

    gps_send_cmd(cmd1);
    gps_send_cmd(cmd2);

    ESP_LOGI(TAG, "🟢 Frecuencia del GPS configurada a %d Hz", hz);
}

// ===========================================================
//  CONFIGURACIÓN DE TRAMAS NMEA
// ===========================================================

void gps_enable_rmc_gga_only(void)
{
    // PMTK314 con 19 campos → compatible con L80-R
    const char *cmd = "PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0";
    gps_send_cmd(cmd);
    vTaskDelay(pdMS_TO_TICKS(200));
    gps_send_cmd(cmd); // Enviar dos veces (recomendado)
    
    ESP_LOGI(TAG, "🟢 L80-R solo enviará GPRMC + GPGGA");
}


void gps_enable_only_rmc(void)
{
    gps_send_cmd("PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    ESP_LOGI(TAG, "🟢 GPS enviará solo GPRMC");
}

void gps_enable_only_gga(void)
{
    gps_send_cmd("PMTK314,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    ESP_LOGI(TAG, "🟢 GPS enviará solo GPGGA");
}

// ===========================================================
//  FUNCIONES DE DEPURACIÓN
// ===========================================================
void mostrar_data_NMEA(void)
{
    uint8_t data[GPS_BUFFER_SIZE];
    for (int i = 0; i < 3; i++) {
        int len = uart_read_bytes(GPS_UART_NUM, data, GPS_BUFFER_SIZE - 1, pdMS_TO_TICKS(200));
        if (len > 0) {
            data[len] = '\0';
            ESP_LOGI(TAG, "Trama NMEA %d:\n%s", i, (char *)data);
        }
    }
}

void mostrar_data_NMEA_filtrada(void)
{
    uint8_t data[GPS_BUFFER_SIZE];
    int len = uart_read_bytes(GPS_UART_NUM, data, GPS_BUFFER_SIZE - 1, pdMS_TO_TICKS(1000));
    if (len > 0) {
        data[len] = '\0';
        char *ptr = (char *)data;

        // Solo imprimir GPRMC y GPGGA
        char *gprmc = strstr(ptr, "$GPRMC");
        if (gprmc) ESP_LOGI(TAG, "RMC: %s", gprmc);

        char *gpgga = strstr(ptr, "$GPGGA");
        if (gpgga) ESP_LOGI(TAG, "GGA: %s", gpgga);
    }
}


// ===========================================================
//  CONVERSIÓN NMEA ddmm.mmmm → grados decimales
// ===========================================================
static double nmea_to_decimal(double nmea_value, char hemisphere)
{
    int degrees = (int)(nmea_value / 100);
    double minutes = nmea_value - (degrees * 100);
    double decimal = degrees + (minutes / 60.0);
    if (hemisphere == 'S' || hemisphere == 'W')
        decimal *= -1.0;
    return decimal;
}

// ===========================================================
//  VERIFICACIÓN DE CHECKSUM
// ===========================================================
bool nmea_verify_checksum(const char *sentence)
{
    if (sentence == NULL || *sentence != '$') return false;
    const char *asterisk = strchr(sentence, '*');
    if (!asterisk) return false;

    uint8_t checksum = 0;
    for (const char *p = sentence + 1; p < asterisk; p++)
        checksum ^= (uint8_t)*p;

    if (strlen(asterisk) < 3) return false;
    uint8_t received = (uint8_t)strtol(asterisk + 1, NULL, 16);
    return (checksum == received);
}

// ===========================================================
//  PARSER GPRMC (posición, velocidad, rumbo)
// ===========================================================
bool gps_parse_gprmc(const char *nmea, gps_data_t *gps)
{
    if (strncmp(nmea, "$GPRMC", 6) != 0) return false;

    char buf[128];
    strncpy(buf, nmea, sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';

    char *tok = strtok(buf, ",");
    int field = 0;

    while (tok != NULL) {
        switch (field) {
            case 1: strncpy(gps->time, tok, sizeof(gps->time)); break;
            case 2: gps->valid = (tok[0] == 'A'); break;
            case 3: gps->latitude = atof(tok); break;
            case 4: gps->latitude = nmea_to_decimal(gps->latitude, tok[0]); break;
            case 5: gps->longitude = atof(tok); break;
            case 6: gps->longitude = nmea_to_decimal(gps->longitude, tok[0]); break;
            case 7: gps->speed_kmh = atof(tok) * 1.852; break;
            case 8: gps->course_deg = atof(tok); break;
            case 9: strncpy(gps->date, tok, sizeof(gps->date)); break;
        }
        tok = strtok(NULL, ",");
        field++;
    }
    return true;
}

// ===========================================================
//  PARSER GPGGA (precisión, altitud, satélites)
// ===========================================================
bool gps_parse_gpgga(const char *nmea, gps_quality_t *q)
{
    if (strncmp(nmea, "$GPGGA", 6) != 0) return false;

    char buf[128];
    strncpy(buf, nmea, sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';

    char *tok = strtok(buf, ",");
    int field = 0;

    while (tok != NULL) {
        switch (field) {
            case 7: q->satellites = atoi(tok); break;
            case 8: q->hdop = atof(tok); break;
            case 9: q->altitude = atof(tok); break;
        }
        tok = strtok(NULL, ",");
        field++;
    }
    return true;
}

// ===========================================================
//  FILTRO KALMAN UNIDIMENSIONAL
// ===========================================================
void kalman_init(kalman_t *kf, double Q, double R)
{
    kf->estimate = 0.0;
    kf->P = 1.0;
    kf->Q = Q;
    kf->R = R;
}

double kalman_update(kalman_t *kf, double measurement)
{
    kf->P += kf->Q;
    double K = kf->P / (kf->P + kf->R);
    kf->estimate += K * (measurement - kf->estimate);
    kf->P *= (1.0 - K);
    return kf->estimate;
}

// ===========================================================
//  FUNCIONES DE LECTURA Y ESCRITURA DE DATOS GPS   
// ===========================================================

// getters
double gps_get_speed_kmh(void){return gps.speed_kmh;}
double gps_get_umbral_movimiento(void){return umbral_movimiento_kmh;}
double gps_get_latitude(void){return gps.latitude;}
double gps_get_longitude(void){return gps.longitude;}
double gps_get_altitude(void){return quality.altitude;}
int gps_get_satellites(void){return quality.satellites;}
double gps_get_hdop(void){return quality.hdop;}
bool gps_is_valid(void){return gps.valid;}


// setters
void gps_set_umbral_movimiento(double valor){
    umbral_movimiento_kmh = valor;
    ESP_LOGI(TAG,"🟢 Nuevo umbral de movimiento establecido a %.2f km/h", umbral_movimiento_kmh);
}

// ===========================================================
//  TAREA PRINCIPAL DE LECTURA Y PROCESAMIENTO GPS
// ===========================================================
void task_gps_read_and_parse(void *pvParameters)
{
    uint8_t data[GPS_BUFFER_SIZE];

    //filtro Kalman -> por calibrar -- 
    //se confiara en la salida del l80R para prubas en campo abierto con buena covertura al cielo
    //kalman_t kalman_vel;
    //kalman_init(&kalman_vel, 0.05, 1.0);

    while (1) {
        int len = uart_read_bytes(GPS_UART_NUM, data, GPS_BUFFER_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0';

            // Buscar tramas GPGGA y GPRMC
            char *gpgga = strstr((char *)data, "$GPGGA");
            char *gprmc = strstr((char *)data, "$GPRMC");

            if (gpgga && nmea_verify_checksum(gpgga))
                gps_parse_gpgga(gpgga, &quality);

            if (gprmc && nmea_verify_checksum(gprmc)) {
                if (gps_parse_gprmc(gprmc, &gps) && gps.valid) {

                      // Ajustar R del Kalman según precisión HDOP -- > filtro Kalman DISABLE
                    //double R_dyn = fmax(0.5, quality.hdop * 0.5);
                    //kalman_vel.R = R_dyn;

                      // Aplicar filtro Kalman                      --> filtro Kalman DISABLE
                    //gps.speed_kmh = kalman_update(&kalman_vel, gps.speed_kmh);

                    if (gps.speed_kmh < umbral_movimiento_kmh) gps.speed_kmh = 0.0;

                    //display_set_number((int16_t)(gps.speed_kmh*100));//(int)gps.speed_kmh
                    
                    ESP_LOGI(TAG, "✅ Valido | HDOP: %.2f | Sat: %d", quality.hdop, quality.satellites);
                    ESP_LOGI(TAG, "Hora: %s | Vel: %.2f km/h | Lat: %.6f | Lon: %.6f | Alt: %.1f m",
                             gps.time, gps.speed_kmh, gps.latitude, gps.longitude, quality.altitude);
                } else {
                    ESP_LOGW(TAG, "⚠️ GPRMC sin fix válido");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
