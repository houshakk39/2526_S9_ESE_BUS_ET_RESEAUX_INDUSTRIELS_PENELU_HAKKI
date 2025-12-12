/*
 * sensors_app.h
 *
 *  Created on: Dec 12, 2025
 *      Author: penel
 */

#ifndef SENSORS_APP_H_
#define SENSORS_APP_H_

#include "main.h"
#include <stdint.h>
#include "bmp280.h"
#include "mpu9250.h"

/* Etat capteurs disponible pour le protocole */
typedef struct
{
    volatile int32_t  temp_centi;   /* Température en 0.01°C */
    volatile uint32_t press_pa;     /* Pression en Pa */
    volatile int32_t  angle_milli;  /* Angle en 0.001° (placeholder) */
} sensors_state_t;

/**
 * @brief Initialise BMP280 + MPU9250 (I2C déjà initialisé par CubeMX).
 *
 * @param hi2c  Handle I2C (ex: &hi2c1)
 * @return HAL_OK si OK
 */
HAL_StatusTypeDef SensorsApp_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Met à jour les valeurs globales (T, P, angle).
 *        À appeler périodiquement dans la boucle.
 */
void SensorsApp_Update(void);

/**
 * @brief Accès à l’état courant des capteurs (pointeur stable).
 */
const sensors_state_t* SensorsApp_GetState(void);

#endif /* SENSORS_APP_H_ */

