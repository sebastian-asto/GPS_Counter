#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include "drivers/buzzer_driver.h"

static const char* TAG = "BUZZER_DRIVER";

void init_buzzer_gpio(void){
    gpio_config_t buzzer_gpio_config = {
        .pin_bit_mask = (1ULL<<BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&buzzer_gpio_config);
    gpio_set_level(BUZZER_GPIO,1); // Desactivar el buzzer
    ESP_LOGI(TAG, "GPIO for Buzzer initialized.");

}

void buzzer_on(void){
    gpio_set_level(BUZZER_GPIO,0); // Activar el buzzer
    ESP_LOGI(TAG, "Buzzer ON");
}

void buzzer_off(void){
    gpio_set_level(BUZZER_GPIO,1); // Desactivar el buzzer
    ESP_LOGI(TAG, "Buzzer OFF");
}