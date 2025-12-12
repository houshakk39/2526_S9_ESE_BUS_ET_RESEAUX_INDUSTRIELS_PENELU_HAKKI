/*
 * valve_control.h
 *
 *  Created on: Dec 12, 2025
 *      Author: penel
 */

#ifndef VALVE_CONTROL_H_
#define VALVE_CONTROL_H_

#include "main.h"
#include <stdint.h>

/**
 * @brief Initialise le contrôleur de vanne (position initiale, etc.).
 */
void ValveControl_Init(void);

/**
 * @brief Calcule l’angle cible (0..90°) à partir de T et K, puis pilote le moteur.
 *
 * @param temp_centi  Température en 0.01°C (ex: 2500 = 25.00°C)
 * @param k_centi     K en 0.01°C (ex: 100 = 1.00°C) => bande [25-K, 25+K]
 */
void ValveControl_Update(int32_t temp_centi, int32_t k_centi);

#endif /* VALVE_CONTROL_H_ */
