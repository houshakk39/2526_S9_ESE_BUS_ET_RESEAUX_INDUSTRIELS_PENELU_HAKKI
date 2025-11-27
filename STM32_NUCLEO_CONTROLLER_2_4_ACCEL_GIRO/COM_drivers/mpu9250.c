/*
 * mpu9250.c
 *
 *  Created on: Nov 27, 2025
 *      Author: penel
 */
#include "mpu9250.h"
#include <stdio.h>

/* On suppose que hi2c1 est défini dans main.c (ou i2c.c) */
extern I2C_HandleTypeDef hi2c1;

/* ======================================================================= */
/* Fonctions internes (statiques)                                         */
/* ======================================================================= */

/**
 * @brief Écrit un octet dans un registre du MPU9250.
 */
static HAL_StatusTypeDef mpu9250_write_reg(uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(&hi2c1,
                             MPU9250_I2C_ADDR,
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &value,
                             1,
                             HAL_MAX_DELAY);
}

/**
 * @brief Lit un octet dans un registre du MPU9250.
 */
static HAL_StatusTypeDef mpu9250_read_reg(uint8_t reg, uint8_t *value)
{
    return HAL_I2C_Mem_Read(&hi2c1,
                            MPU9250_I2C_ADDR,
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            value,
                            1,
                            HAL_MAX_DELAY);
}

/**
 * @brief Lit plusieurs octets consécutifs à partir d’une adresse de registre.
 */
static HAL_StatusTypeDef mpu9250_read_multi(uint8_t reg, uint8_t *p_data, uint16_t size)
{
    return HAL_I2C_Mem_Read(&hi2c1,
                            MPU9250_I2C_ADDR,
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            p_data,
                            size,
                            HAL_MAX_DELAY);
}

/* ======================================================================= */
/* Fonctions publiques                                                    */
/* ======================================================================= */

HAL_StatusTypeDef mpu9250_read_who_am_i(uint8_t *who_am_i)
{
    HAL_StatusTypeDef ret;

    ret = mpu9250_read_reg(MPU9250_REG_WHO_AM_I, who_am_i);
    if (ret != HAL_OK)
    {
        printf("MPU9250: Erreur I2C lecture WHO_AM_I (ret = %d)\r\n", ret);
        return ret;
    }

    printf("MPU9250 WHO_AM_I = 0x%02X\r\n", *who_am_i);

    if (*who_am_i == MPU9250_WHO_AM_I_VALUE)
    {
        printf("MPU9250 identifie correctement (0x%02X attendu)\r\n",
               MPU9250_WHO_AM_I_VALUE);
    }
    else
    {
        printf("MPU9250 WHO_AM_I inattendu (0x%02X attendu)\r\n",
               MPU9250_WHO_AM_I_VALUE);
    }

    return HAL_OK;
}

HAL_StatusTypeDef mpu9250_init(void)
{
    HAL_StatusTypeDef ret;
    uint8_t who_am_i = 0;
    uint8_t value;

    /* Lecture et vérification du WHO_AM_I */
    ret = mpu9250_read_who_am_i(&who_am_i);
    if (ret != HAL_OK)
    {
        return ret;
    }

    /* Sortie du mode sleep, sélection de la source d’horloge
     * PWR_MGMT_1 (0x6B)
     * bit 6 = SLEEP
     * bits 2:0 = CLKSEL (001 = PLL gyro X)
     */
    value = 0x01;  /* SLEEP = 0, CLKSEL = 1 */
    ret = mpu9250_write_reg(MPU9250_REG_PWR_MGMT_1, value);
    if (ret != HAL_OK)
    {
        printf("MPU9250: Erreur I2C ecriture PWR_MGMT_1\r\n");
        return ret;
    }

    /* Désactivation des standby sur tous les axes (gyro + accel)
     * PWR_MGMT_2 (0x6C) = 0x00 -> tous les axes actifs
     */
    value = 0x00;
    ret = mpu9250_write_reg(MPU9250_REG_PWR_MGMT_2, value);
    if (ret != HAL_OK)
    {
        printf("MPU9250: Erreur I2C ecriture PWR_MGMT_2\r\n");
        return ret;
    }

    /* Configuration de la fréquence d’échantillonnage :
     * SMPLRT_DIV (0x19)
     * Fréquence = Gyro_Output_Rate / (1 + SMPLRT_DIV)
     * Gyro_Output_Rate par défaut = 8 kHz -> ici on prend par exemple ~1 kHz
     * -> SMPLRT_DIV = 7
     */
    value = 7;
    ret = mpu9250_write_reg(MPU9250_REG_SMPLRT_DIV, value);
    if (ret != HAL_OK)
    {
        printf("MPU9250: Erreur I2C ecriture SMPLRT_DIV\r\n");
        return ret;
    }

    /* Configuration du filtre DLPF (CONFIG, 0x1A)
     * On choisit par exemple DLPF_CFG = 3 (cf. Register Map pour les détails)
     * -> bande passante gyro ~ 44 Hz
     */
    value = 0x03;
    ret = mpu9250_write_reg(MPU9250_REG_CONFIG, value);
    if (ret != HAL_OK)
    {
        printf("MPU9250: Erreur I2C ecriture CONFIG\r\n");
        return ret;
    }

    /* Configuration du gyro :
     * GYRO_CONFIG (0x1B)
     * [4:3] = FS_SEL
     *   00 -> ±250 dps
     *   01 -> ±500 dps
     *   10 -> ±1000 dps
     *   11 -> ±2000 dps
     * Ici on choisit ±250 dps -> FS_SEL = 0
     */
    value = 0x00;
    ret = mpu9250_write_reg(MPU9250_REG_GYRO_CONFIG, value);
    if (ret != HAL_OK)
    {
        printf("MPU9250: Erreur I2C ecriture GYRO_CONFIG\r\n");
        return ret;
    }

    /* Configuration de l’accéléro :
     * ACCEL_CONFIG (0x1C)
     * [4:3] = AFS_SEL
     *   00 -> ±2g
     *   01 -> ±4g
     *   10 -> ±8g
     *   11 -> ±16g
     * Ici on choisit ±2g -> AFS_SEL = 0
     */
    value = 0x00;
    ret = mpu9250_write_reg(MPU9250_REG_ACCEL_CONFIG, value);
    if (ret != HAL_OK)
    {
        printf("MPU9250: Erreur I2C ecriture ACCEL_CONFIG\r\n");
        return ret;
    }

    /* Configuration du filtre d’accélération (ACCEL_CONFIG2, 0x1D)
     * Par exemple A_DLPFCFG = 3 (~44 Hz)
     */
    value = 0x03;
    ret = mpu9250_write_reg(MPU9250_REG_ACCEL_CONFIG2, value);
    if (ret != HAL_OK)
    {
        printf("MPU9250: Erreur I2C ecriture ACCEL_CONFIG2\r\n");
        return ret;
    }

    printf("MPU9250: initialisation terminee\r\n");

    /* Petit délai pour laisser les filtres / capteurs se stabiliser */
    HAL_Delay(100);

    return HAL_OK;
}

HAL_StatusTypeDef mpu9250_read_raw(mpu9250_raw_data_t *data)
{
    HAL_StatusTypeDef ret;
    uint8_t buf[14];

    if (data == NULL)
    {
        return HAL_ERROR;
    }

    /* Lecture des registres :
     * ACCEL_XOUT_H (0x3B) -> ACCEL_XOUT_L, ACCEL_YOUT_H, ..., GYRO_ZOUT_L
     * Total : 14 octets (6 pour accel, 2 pour température, 6 pour gyro)
     */
    ret = mpu9250_read_multi(MPU9250_REG_ACCEL_XOUT_H, buf, 14);
    if (ret != HAL_OK)
    {
        printf("MPU9250: Erreur I2C lecture donnees brutes (ret = %d)\r\n", ret);
        return ret;
    }

    /* Conversion des octets en int16_t (big-endian : MSB puis LSB) */
    data->ax = (int16_t)((buf[0] << 8) | buf[1]);
    data->ay = (int16_t)((buf[2] << 8) | buf[3]);
    data->az = (int16_t)((buf[4] << 8) | buf[5]);

    /* buf[6] et buf[7] -> température interne, ignorée ici */

    data->gx = (int16_t)((buf[8]  << 8) | buf[9]);
    data->gy = (int16_t)((buf[10] << 8) | buf[11]);
    data->gz = (int16_t)((buf[12] << 8) | buf[13]);

    return HAL_OK;
}

/* ======================================================================= */
/* Conversions en entier fixe (sans float)                                 */
/* ======================================================================= */

void mpu9250_convert_accel_mg(const mpu9250_raw_data_t *raw,
                              int32_t *ax_mg, int32_t *ay_mg, int32_t *az_mg)
{
    if (!raw || !ax_mg || !ay_mg || !az_mg)
        return;

    /* Échelle : ±2g -> 16384 LSB/g
     * On veut le résultat en milli-g (mg) :
     *   1 g = 1000 mg
     *   accel[g]  = raw / 16384
     *   accel[mg] = accel[g] * 1000 = raw * 1000 / 16384
     */
    *ax_mg = ((int32_t)raw->ax * 1000) / MPU9250_ACCEL_SENS_2G_LSB;
    *ay_mg = ((int32_t)raw->ay * 1000) / MPU9250_ACCEL_SENS_2G_LSB;
    *az_mg = ((int32_t)raw->az * 1000) / MPU9250_ACCEL_SENS_2G_LSB;
}

void mpu9250_convert_gyro_mdps(const mpu9250_raw_data_t *raw,
                               int32_t *gx_mdps, int32_t *gy_mdps, int32_t *gz_mdps)
{
    if (!raw || !gx_mdps || !gy_mdps || !gz_mdps)
        return;

    /* Échelle : ±250 dps -> 131 LSB/(°/s)
     * On veut le résultat en milli-deg/s (mdps) :
     *   1 °/s = 1000 mdps
     *   gyro[dps]   = raw / 131
     *   gyro[mdps]  = gyro[dps] * 1000 = raw * 1000 / 131
     */
    *gx_mdps = ((int32_t)raw->gx * 1000) / MPU9250_GYRO_SENS_250DPS_LSB;
    *gy_mdps = ((int32_t)raw->gy * 1000) / MPU9250_GYRO_SENS_250DPS_LSB;
    *gz_mdps = ((int32_t)raw->gz * 1000) / MPU9250_GYRO_SENS_250DPS_LSB;
}
