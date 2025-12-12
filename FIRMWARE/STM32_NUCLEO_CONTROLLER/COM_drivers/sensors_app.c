/*
 * sensors_app.c
 *
 *  Created on: Dec 12, 2025
 *      Author: penel
 */

#include "sensors_app.h"
#include <stdio.h>

/* Handles capteurs */
static BMP280_HandleTypedef s_bmp;
static mpu9250_raw_data_t   s_imu;

/* Etat global */
static sensors_state_t s_state =
{
    .temp_centi  = 0,
    .press_pa    = 0,
    .angle_milli = 0
};

HAL_StatusTypeDef SensorsApp_Init(I2C_HandleTypeDef *hi2c)
{
    printf("\r\n=== Init capteurs ===\r\n");

    if (BMP280_Init(&s_bmp, hi2c, BMP280_I2C_ADDR_DEFAULT) != HAL_OK)
    {
        printf("Erreur init BMP280\r\n");
        return HAL_ERROR;
    }

    if (mpu9250_init() != HAL_OK)
    {
        printf("Erreur init MPU9250\r\n");
        /* On ne bloque pas forcément : à toi de décider */
        return HAL_ERROR;
    }

    return HAL_OK;
}

void SensorsApp_Update(void)
{
    uint32_t raw_temp, raw_press;
    BMP280_S32_t T;
    BMP280_U32_t P;

    if (BMP280_ReadRaw(&s_bmp, &raw_temp, &raw_press) == HAL_OK)
    {
        T = BMP280_Compensate_T_int32(&s_bmp, (BMP280_S32_t)raw_temp);
        P = BMP280_Compensate_P_int32(&s_bmp, (BMP280_S32_t)raw_press);

        s_state.temp_centi = (int32_t)T;
        s_state.press_pa   = (uint32_t)P;
    }

    if (mpu9250_read_raw(&s_imu) == HAL_OK)
    {
        /* TODO: calcul angle (plus tard) */
        s_state.angle_milli = 0;
    }
}

const sensors_state_t* SensorsApp_GetState(void)
{
    return &s_state;
}

