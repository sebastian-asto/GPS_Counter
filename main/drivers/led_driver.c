#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include "drivers/led_driver.h"

static const char* TAG = "LED_DRIVER";
static uint8_t state_led = 0;

void init_gpio_config_led_state(void){
    gpio_config_t config_pin_state = {
        .pin_bit_mask = (1ULL << LED_GPIO), // Máscara del pin
        .mode = GPIO_MODE_OUTPUT,            // Modo salida
        .pull_up_en = GPIO_PULLUP_DISABLE,   // Sin pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Sin pull-down
        .intr_type = GPIO_INTR_DISABLE       // Sin interrupción
    };

    gpio_config(&config_pin_state);
    gpio_set_level(LED_GPIO,0); //led Apagado al iniciar
    ESP_LOGI(TAG, "Led GPIO initialized.");
}

void toggle_led_state(void){
    if(state_led == 0){
        gpio_set_level(LED_GPIO,1);
        ESP_LOGI(TAG,"LED_STATE_ENCENDIDO");
        state_led = 1;
    }else{
        gpio_set_level(LED_GPIO,0);
        ESP_LOGI(TAG,"LED_STATE_APAGADO");
        state_led = 0;
    }
}

void led_state_on(void){
    gpio_set_level(LED_GPIO,1);
    ESP_LOGI(TAG,"LED_STATE_ENCENDIDO");
    state_led = 1;
}

void led_state_off(void){
    gpio_set_level(LED_GPIO,0);
    ESP_LOGI(TAG,"LED_STATE_APAGADO");
    state_led = 0;
}

