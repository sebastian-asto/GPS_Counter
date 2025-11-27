#ifndef MONITOR_VELOCIDAD_H
#define MONITOR_VELOCIDAD_H

#include <stdint.h>
#include <stdbool.h>

// GETTERS
double monitor_velocidad_get_umbral(void);
uint16_t monitor_velocidad_get_contador_eventos(void);
bool monitor_velocidad_ultimo_fix_valido(void);

// SETTERS
void monitor_velocidad_set_umbral(double nuevo_umbral);
void monitor_velocidad_reset_contador(void);

// TAREA PRINCIPAL
void task_monitor_velocidad(void *pvParameters);

#endif
