#pragma once
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define BUZZER_GPIO GPIO_NUM_15 
// control del buzzer
void init_buzzer_gpio(void);
void buzzer_on(void);
void buzzer_off(void);
