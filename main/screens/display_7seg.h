
#pragma once
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 

// bit para el decodificador 7 segmentos 
#define BIT_0_GPIO GPIO_NUM_2
#define BIT_1_GPIO GPIO_NUM_4
#define BIT_2_GPIO GPIO_NUM_13
#define BIT_3_GPIO GPIO_NUM_33

//pines gpio de los digitos (de los 3 displays)
#define DIGITO_1_GPIO GPIO_NUM_18
#define DIGITO_2_GPIO GPIO_NUM_19
#define DIGITO_3_GPIO GPIO_NUM_21

//pin gpio para el punto decimal (no usado)
#define DP_GPIO GPIO_NUM_14

void init_7seg_display_gpio(void);
void display_set_number(uint16_t number);

//task para manejar el display 7 seg
void task_display_7seg(void *arg);
//void display_number(void);