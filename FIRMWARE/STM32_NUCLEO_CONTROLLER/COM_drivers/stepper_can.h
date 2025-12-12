/*
 * stepper_can.h
 *
 *  Created on: Dec 12, 2025
 *      Author: penel
 */

#ifndef STEPPER_CAN_H_
#define STEPPER_CAN_H_

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* =========================
 *  Identifiants CAN (base)
 *  SW1/SW2 sur la carte moteur:
 *   00 -> 0x60/0x61/0x62
 *   10 -> 0x70/0x71/0x72
 *   01 -> 0x80/0x81/0x82
 *   11 -> 0x90/0x91/0x92
 * ========================= */
typedef enum
{
    STEPPER_CAN_BASE_0x60 = 0x60,
    STEPPER_CAN_BASE_0x70 = 0x70,
    STEPPER_CAN_BASE_0x80 = 0x80,
    STEPPER_CAN_BASE_0x90 = 0x90
} stepper_can_base_id_t;

/* Direction en mode manuel (D0) */
typedef enum
{
    STEPPER_DIR_CCW = 0x00, /* Anti-horaire */
    STEPPER_DIR_CW  = 0x01  /* Horaire */
} stepper_can_dir_t;

/* Signe en mode angle (D1) */
typedef enum
{
    STEPPER_SIGN_POS = 0x00,
    STEPPER_SIGN_NEG = 0x01
} stepper_can_sign_t;

/**
 * @brief Initialise et démarre le périphérique CAN (HAL_CAN_Start).
 *        Optionnel : active une notification TX mailbox vide.
 *
 * @param hcan  Handle CAN (ex: &hcan1)
 * @return HAL_OK si OK
 */
HAL_StatusTypeDef StepperCAN_Init(CAN_HandleTypeDef *hcan);

/**
 * @brief Définit la base d'ID à utiliser (0x60 / 0x70 / 0x80 / 0x90).
 */
void StepperCAN_SetBaseId(stepper_can_base_id_t base_id);

/**
 * @brief Mode manuel (ID = base + 0):
 *        D0 = direction (0x00 CCW, 0x01 CW)
 *        D1 = steps (0x01..0xFF) (1 unité = 1°)
 *        D2 = speed (0x01..0xFF) (0x01=1ms, 0xFF=255ms)
 *
 * @param dir    Direction
 * @param steps  1..255
 * @param speed  1..255
 */
HAL_StatusTypeDef StepperCAN_ManualMove(stepper_can_dir_t dir,
                                        uint8_t steps,
                                        uint8_t speed);

/**
 * @brief Mode angle (ID = base + 1):
 *        D0 = angle (1..255) mais carte clamp à 180°
 *        D1 = signe (0 positif, 1 négatif)
 *
 * @param angle_deg  angle en degrés (0..180 recommandé)
 * @param sign       signe
 */
HAL_StatusTypeDef StepperCAN_SetAngle(uint8_t angle_deg,
                                      stepper_can_sign_t sign);

/**
 * @brief Reset de la position interne (ID = base + 2).
 */
HAL_StatusTypeDef StepperCAN_SetZero(void);

#endif /* STEPPER_CAN_H_ */
