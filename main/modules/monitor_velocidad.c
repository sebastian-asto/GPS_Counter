#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <stdbool.h>
#include <stdint.h>

#include "nvs_flash.h"
#include "nvs.h"

#include "monitor_velocidad.h"
#include "gps_l80r.h"
#include "screens/display_7seg.h"
#include "drivers/buzzer_driver.h"

static const char *TAG = "MONITOR_VEL";

// ===========================================================
//  PARÁMETROS CONFIGURABLES
// ===========================================================
static double umbral_velocidad = 30.0;   // km/h

// ===========================================================
//  VARIABLES INTERNAS
// ===========================================================
static uint16_t contador_eventos = 0;
static bool estaba_sobre_umbral = false;
static bool ultimo_fix_valido = false;

// ===========================================================
//  NVS: GUARDAR CONTADOR COMO uint32_t
// ===========================================================
static void guardar_contador_eventos(uint16_t valor)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("almacen", NVS_READWRITE, &handle);

    if (err == ESP_OK) {
        uint32_t tmp = (uint32_t)valor;
        nvs_set_u32(handle, "eventos", tmp);
        nvs_commit(handle);
        nvs_close(handle);

        ESP_LOGI(TAG, "Contador guardado en NVS: %u", valor);
    } else {
        ESP_LOGE(TAG, "Error al abrir NVS para guardar contador");
    }
}

// ===========================================================
//  NVS: LEER CONTADOR GUARDADO
// ===========================================================
static void leer_contador_guardado_en_nvs(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("almacen", NVS_READWRITE, &handle);

    if (err == ESP_OK) {
        uint32_t tmp = 0;

        if (nvs_get_u32(handle, "eventos", &tmp) == ESP_OK) {
            contador_eventos = (uint16_t)tmp;
            ESP_LOGI(TAG, "Contador cargado desde NVS: %u", contador_eventos);
        } else {
            contador_eventos = 0;
            ESP_LOGW(TAG, "No existe contador previo, comenzando en 0");
        }

        nvs_close(handle);
    } else {
        ESP_LOGE(TAG, "Error al abrir NVS para leer contador");
    }
}

// ===========================================================
//  GETTERS Y SETTERS
// ===========================================================
double monitor_velocidad_get_umbral(void) {
    return umbral_velocidad;
}

uint16_t monitor_velocidad_get_contador_eventos(void) {
    return contador_eventos;
}

bool monitor_velocidad_ultimo_fix_valido(void) {
    return ultimo_fix_valido;
}

void monitor_velocidad_set_umbral(double nuevo_umbral) {
    umbral_velocidad = nuevo_umbral;
    ESP_LOGI(TAG,"🟢 Nuevo umbral de velocidad establecido a %.2f km/h", umbral_velocidad);
}

void monitor_velocidad_reset_contador(void)
{
    contador_eventos = 0;
    guardar_contador_eventos(0);

    ESP_LOGW(TAG, "🟢 Contador de eventos reiniciado a 0");
    display_set_number(0);

    buzzer_on();
    vTaskDelay(pdMS_TO_TICKS(100));
    buzzer_off();
}

// ===========================================================
//  PATRÓN DE ALERTA: BI-BI… (pausa) … BI-BI…
// ===========================================================
static void alerta_bi_bi(void)
{
    static uint32_t ts_ultimo_bip = 0;
    static uint8_t contador_bips = 0;

    uint32_t ahora = xTaskGetTickCount();

    // bip cada 180 ms
    if (contador_bips < 2) {
        if (ahora - ts_ultimo_bip > pdMS_TO_TICKS(180)) {

            // beep corto
            buzzer_on();
            vTaskDelay(pdMS_TO_TICKS(70));
            buzzer_off();

            contador_bips++;
            ts_ultimo_bip = xTaskGetTickCount();
        }
    }
    else {
        // pausa larga después del BI-BI
        if (ahora - ts_ultimo_bip > pdMS_TO_TICKS(600)) {
            contador_bips = 0;   // reiniciar bi-bi
            ts_ultimo_bip = xTaskGetTickCount();
        }
    }
}

// ===========================================================
//  FUNCIÓN PARA DIAGNOSTICAR ESTADO DEL GPS
// ===========================================================
static bool gps_diagnostico_ok(double vel, bool fix_ok)
{
    uint32_t tramas = gps_get_contador_tramas();
    uint32_t rmc = gps_get_contador_rmc();

    // 1) GPS no conectado
    if (tramas == 0) {
        ESP_LOGW("MONITOR_VEL", "🛑 GPS NO CONECTADO (no llegan tramas)");
        vTaskDelay(pdMS_TO_TICKS(300));
        return false;
    }

    // 2) No llegan tramas RMC
    if (rmc == 0) {
        ESP_LOGW("MONITOR_VEL", "⚠️ GPS CONECTADO → PERO NO LLEGAN TRAMAS RMC");
        vTaskDelay(pdMS_TO_TICKS(300));
        return false;
    }

    // 3) Hay RMC pero sin FIX válido
    if (!fix_ok) {
        ESP_LOGW("MONITOR_VEL", "⚠️ RMC SIN FIX VÁLIDO → Vel %.2f ignorada", vel);
        vTaskDelay(pdMS_TO_TICKS(200));
        return false;
    }

    return true; // Todo está OK
}


// ===========================================================
//  Task principal
// ===========================================================
void task_monitor_velocidad(void *pvParameters)
{
    leer_contador_guardado_en_nvs();
    display_set_number(contador_eventos);

    while (1)
    {
        double vel = gps_get_speed_kmh();
        bool fix_ok = gps_is_valid();
        ultimo_fix_valido = fix_ok;

        // DIAGNÓSTICO CENTRALIZADO
        if (!gps_diagnostico_ok(vel, fix_ok))
            continue;

        // =============================
        // ESTADO NORMAL (GPS con FIX)
        // =============================
        if (!estaba_sobre_umbral && vel > umbral_velocidad)
        {
            contador_eventos++;
            estaba_sobre_umbral = true;

            ESP_LOGI(TAG, "🚀 Evento #%u (vel=%.2f)", contador_eventos, vel);

            guardar_contador_eventos(contador_eventos);
            display_set_number(contador_eventos);

            buzzer_on();
            vTaskDelay(pdMS_TO_TICKS(120));
            buzzer_off();
        }

        if (estaba_sobre_umbral && vel > umbral_velocidad)
        {
            alerta_bi_bi();
        }
        else
        {
            estaba_sobre_umbral = false;
        }

        vTaskDelay(pdMS_TO_TICKS(40));
    }
}


