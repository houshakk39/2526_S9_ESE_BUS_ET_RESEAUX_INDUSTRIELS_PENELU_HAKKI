/*
 * stepper_can.c
 *
 *  Created on: Dec 12, 2025
 *      Author: penel
 */

#include "stepper_can.h"

/* Handle CAN global (fourni par CubeMX) */
extern CAN_HandleTypeDef hcan1;

/* Base ID par défaut (SW1=0, SW2=0) */
static uint16_t s_base_id = (uint16_t)STEPPER_CAN_BASE_0x60;

/* ---------- Fonction interne : envoi trame CAN standard ---------- */
static HAL_StatusTypeDef StepperCAN_SendStd(uint16_t std_id,
                                            const uint8_t *data,
                                            uint8_t dlc)
{
    CAN_TxHeaderTypeDef txh;
    uint32_t tx_mailbox;

    txh.StdId = std_id;
    txh.ExtId = 0;
    txh.IDE   = CAN_ID_STD;
    txh.RTR   = CAN_RTR_DATA;
    txh.DLC   = dlc;
    txh.TransmitGlobalTime = DISABLE;

    return HAL_CAN_AddTxMessage(&hcan1, &txh, (uint8_t*)data, &tx_mailbox);
}

/* ------------------- API ------------------- */

HAL_StatusTypeDef StepperCAN_Init(CAN_HandleTypeDef *hcan)
{
    if (hcan == NULL)
        return HAL_ERROR;

    /* Démarrage CAN */
    if (HAL_CAN_Start(hcan) != HAL_OK)
        return HAL_ERROR;

    /* Optionnel : notification TX (pas obligatoire si on ne reçoit pas) */
    (void)HAL_CAN_ActivateNotification(hcan, CAN_IT_TX_MAILBOX_EMPTY);

    return HAL_OK;
}

void StepperCAN_SetBaseId(stepper_can_base_id_t base_id)
{
    s_base_id = (uint16_t)base_id;
}

HAL_StatusTypeDef StepperCAN_ManualMove(stepper_can_dir_t dir,
                                        uint8_t steps,
                                        uint8_t speed)
{
    /* Sécurité : les valeurs 0 ne sont pas prévues (doc: 0x01..0xFF) */
    if (steps == 0 || speed == 0)
        return HAL_ERROR;

    uint8_t d[3];
    d[0] = (uint8_t)dir;
    d[1] = steps;
    d[2] = speed;

    /* ID = base + 0 */
    return StepperCAN_SendStd((uint16_t)(s_base_id + 0u), d, 3u);
}

HAL_StatusTypeDef StepperCAN_SetAngle(uint8_t angle_deg,
                                      stepper_can_sign_t sign)
{
    /* Doc: angle entre 0° et 180° ( >180 clamp à 180° sur la carte) */
    if (angle_deg > 180)
        angle_deg = 180;

    uint8_t d[2];
    d[0] = angle_deg;
    d[1] = (uint8_t)sign;

    /* ID = base + 1 */
    return StepperCAN_SendStd((uint16_t)(s_base_id + 1u), d, 2u);
}

HAL_StatusTypeDef StepperCAN_SetZero(void)
{
    /* ID = base + 2
     * Doc ne spécifie pas de données -> DLC=0
     */
    return StepperCAN_SendStd((uint16_t)(s_base_id + 2u), NULL, 0u);
}
