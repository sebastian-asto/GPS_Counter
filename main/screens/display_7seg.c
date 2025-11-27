#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "screens/display_7seg.h"

static const char* TAG = "DISPLAY_7SEG";

// Variable del valor a mostrar
static uint16_t number_to_display = 0;

// ----------------------
// Inicialización GPIO
// ----------------------
void init_7seg_display_gpio(void)
{
    gpio_config_t bits_config = {
        .pin_bit_mask = (1ULL<<BIT_0_GPIO) | (1ULL<<BIT_1_GPIO) | (1ULL<<BIT_2_GPIO) | (1ULL<<BIT_3_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config_t digits_config = {
        .pin_bit_mask = (1ULL<<DIGITO_1_GPIO) | (1ULL<<DIGITO_2_GPIO) | (1ULL<<DIGITO_3_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&bits_config);
    gpio_config(&digits_config);

    gpio_set_level(DIGITO_1_GPIO, 0);
    gpio_set_level(DIGITO_2_GPIO, 0);
    gpio_set_level(DIGITO_3_GPIO, 0);

    ESP_LOGI(TAG, "Display GPIO initialized.");
}

// ----------------------
// Funcion para actualizar el número a mostrar
// ----------------------
void display_set_number(uint16_t number)
{
    if (number > 999) number = 999;
    if (number < 0) number = 0;
    number_to_display = number;
}

// ----------------------
// Mostrar un dígito específico
// ----------------------
static void show_digit(uint8_t digit, uint8_t value)
{
    // bits BCD
    gpio_set_level(BIT_0_GPIO, (value >> 0) & 0x01);
    gpio_set_level(BIT_1_GPIO, (value >> 1) & 0x01);
    gpio_set_level(BIT_2_GPIO, (value >> 2) & 0x01);
    gpio_set_level(BIT_3_GPIO, (value >> 3) & 0x01);

    // activar solo un dígito
    gpio_set_level(DIGITO_1_GPIO, (digit == 1));
    gpio_set_level(DIGITO_2_GPIO, (digit == 2));
    gpio_set_level(DIGITO_3_GPIO, (digit == 3));
}

// ----------------------
// Tarea ejecutandose en el CPU 1
// ----------------------

void task_display_7seg(void *arg)
{
    ESP_LOGI(TAG, "Displays task started.");
    init_7seg_display_gpio();
    uint8_t d1, d2, d3;
    uint16_t n;
    while (true){
        
        n = number_to_display;

        d1 = (n / 100) % 10;
        d2 = (n / 10) % 10;
        d3 = n % 10;

        // Ciclo de multiplexado
        show_digit(3, d1);
        vTaskDelay(pdMS_TO_TICKS(6)); 

        show_digit(2, d2);
        vTaskDelay(pdMS_TO_TICKS(6));

        show_digit(1, d3);
        vTaskDelay(pdMS_TO_TICKS(6));
    }
}
