/*
 * valve_control.c
 *
 *  Created on: Dec 12, 2025
 *      Author: penel
 */

#include "valve_control.h"
#include "stepper_can.h"

/* Reference temperature: 25.00 °C (in centi-degrees) */
#define T_REF_CENTI      2500

/* Mechanical saturation of the valve */
#define ANGLE_LIMIT_DEG  90   /* range: [-90 ; +90] */

static int32_t s_last_angle = 999; /* valeur impossible pour forcer 1er envoi */
static int32_t s_last_cmd = 9999;

void ValveControl_Init(void)
{
    s_last_angle = 999; /* force 1ère commande */
}

void ValveControl_Update(int32_t temp_centi, int32_t k_centi)
{
    /* Temperature error relative to reference (in 0.01 °C) */
    int32_t err_centi = temp_centi - T_REF_CENTI;

    /* Proportional control law (integer arithmetic only):
     *
     * angle_deg = K * (T - Tref)
     *
     * temp_centi = (T - Tref) * 100
     * k_centi    = K * 100
     *
     * => angle_deg = (k_centi * err_centi) / 10000
     */
    int32_t prod = k_centi * err_centi;
    int32_t angle_deg = prod / 10000;

    /* Saturation to protect the actuator */
    if (angle_deg >  ANGLE_LIMIT_DEG) angle_deg =  ANGLE_LIMIT_DEG;
    if (angle_deg < -ANGLE_LIMIT_DEG) angle_deg = -ANGLE_LIMIT_DEG;

    /* Do not resend the same command repeatedly on the CAN bus */
    if (angle_deg == s_last_cmd)
        return;

    s_last_cmd = angle_deg;

    /* Send command to the stepper motor */
    if (angle_deg < 0)
    {
        /* Negative angle */
        StepperCAN_SetAngle((uint8_t)(-angle_deg), STEPPER_SIGN_NEG);
    }
    else
    {
        /* Positive or zero angle */
        StepperCAN_SetAngle((uint8_t)(angle_deg), STEPPER_SIGN_POS);
    }
}
