#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/uart.h"

#include "drivers/led_driver.h"
#include "drivers/buzzer_driver.h"
#include "modules/gps_l80r.h"
#include "screens/display_7seg.h"
#include "modules/monitor_velocidad.h"

static const char* TAG = "MAIN";
void init_nvs(void);
void config_gps(void);

void app_main(void){

    init_nvs();

    init_uart_gps_l80r();
    init_7seg_display_gpio();
    init_buzzer_gpio();
    init_gpio_config_led_state();
    
    vTaskDelay(pdMS_TO_TICKS(1000)); // Esperar a que todo se inicialice
    led_state_off();
    config_gps(); // Configurar GPS
    //gps_restore_default(); // Restaurar configuración default del GPS


    // creando tarea en el CPU 1
    xTaskCreatePinnedToCore(task_display_7seg,"task_display_7seg",4096,NULL,3,NULL,1);
    // tareas en CPU 0
    xTaskCreate(task_gps_read_and_parse,"task_gps_read_and_parse",4096,NULL,5,NULL);
    xTaskCreate(task_monitor_velocidad,"task_monitor_velocidad",4096,NULL,4,NULL);

    //vTaskDelay(pdMS_TO_TICKS(100));

    //test buzzer
    //buzzer_on();
    //vTaskDelay(pdMS_TO_TICKS(2000));
    //buzzer_off();

    //test
    //monitor_velocidad_set_umbral(0.9); // km/h
    //gps_set_umbral_movimiento(0.1);

    //

    ESP_LOGI(TAG,"Umbral de velocidad: %f Km/h",monitor_velocidad_get_umbral());
    ESP_LOGI(TAG,"Umbral de movimiento: %f Km/h",gps_get_umbral_movimiento());

    while(true){
        //ESP_LOGI(TAG,"While principal");
        //mostrar_data_NMEA(); // ver data cruda
        vTaskDelay(pdMS_TO_TICKS(10000)); // simular trabajo en el CPU 0
        //monitor_velocidad_reset_contador();
    }
}

// ===========================================================
//Inicializar NVS
// ===========================================================
void init_nvs(void){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS inicializado correctamente");
}

// ===========================================================
//  CONFIGURAR GPS
// ===========================================================
void config_gps(void){
    gps_set_update_rate_hz(5);        // 5Hz
    vTaskDelay(pdMS_TO_TICKS(200));

    gps_enable_rmc_gga_only();        // Solo RMC + GGA
    vTaskDelay(pdMS_TO_TICKS(200));

    uart_flush(GPS_UART_NUM);         //Limpiar todo el buffer
    vTaskDelay(pdMS_TO_TICKS(200));  //Esperar a que PMTK se aplique
}
