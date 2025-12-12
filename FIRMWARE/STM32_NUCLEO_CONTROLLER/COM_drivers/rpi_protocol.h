/*
 * rpi_protocol.h
 *
 *  Created on: Dec 12, 2025
 *      Author: penel
 */

#ifndef RPI_PROTOCOL_H_
#define RPI_PROTOCOL_H_

#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include "sensors_app.h"

/**
 * @brief Initialise le protocole UART vers Raspberry Pi.
 *        Lance la réception IT sur UART1.
 *
 * @param huart_rpi  UART vers Raspberry (ex: &huart1)
 * @param state      Pointeur vers l'état capteurs (SensorsApp_GetState())
 */
void RpiProto_Init(UART_HandleTypeDef *huart_rpi, const sensors_state_t *state);

/**
 * @brief À appeler depuis HAL_UART_RxCpltCallback() quand un byte est reçu sur UART1.
 */
void RpiProto_OnRxByte(uint8_t ch);

/**
 * @brief À appeler dans la boucle principale :
 *        traite une commande complète si disponible.
 */
void RpiProto_Task(void);

int32_t RpiProto_GetK_centi(void);


#endif /* RPI_PROTOCOL_H_ */

