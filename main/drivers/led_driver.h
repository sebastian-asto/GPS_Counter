#pragma once
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>

#define LED_GPIO GPIO_NUM_22

void init_gpio_config_led_state(void);

// Función para alternar el estado del LED
void toggle_led_state(void);
void led_state_on(void);
void led_state_off(void);


