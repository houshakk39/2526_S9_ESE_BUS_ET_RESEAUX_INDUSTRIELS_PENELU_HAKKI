/*
 * mpu9250.h
 *
 *  Created on: Nov 27, 2025
 *      Author: penel
 */
#ifndef MPU9250_H_
#define MPU9250_H_

#include "main.h"
#include <stdint.h>

/**
 * @brief Adresse I2C (7 bits) = 0x68 -> adresse HAL (8 bits) = 0x68 << 1
 */
#define MPU9250_I2C_ADDR          (0x68u << 1)

/* Registres principaux (voir Register Map MPU-9250) */
#define MPU9250_REG_SMPLRT_DIV    0x19u
#define MPU9250_REG_CONFIG        0x1Au
#define MPU9250_REG_GYRO_CONFIG   0x1Bu
#define MPU9250_REG_ACCEL_CONFIG  0x1Cu
#define MPU9250_REG_ACCEL_CONFIG2 0x1Du

#define MPU9250_REG_INT_STATUS    0x3Au

#define MPU9250_REG_ACCEL_XOUT_H  0x3Bu
#define MPU9250_REG_TEMP_OUT_H    0x41u
#define MPU9250_REG_GYRO_XOUT_H   0x43u

#define MPU9250_REG_PWR_MGMT_1    0x6Bu
#define MPU9250_REG_PWR_MGMT_2    0x6Cu

#define MPU9250_REG_WHO_AM_I      0x75u
#define MPU9250_WHO_AM_I_VALUE    0x71u   /* Valeur typique pour MPU-9250 */

/* Full scale utilisé dans ce driver :
 * - Gyro :  ±250 dps -> 131 LSB/(°/s)
 * - Accel : ±2 g     -> 16384 LSB/g
 * Les conversions se feront en entier fixe (milli-g, milli-dps).
 */
#define MPU9250_GYRO_SENS_250DPS_LSB   131      /* LSB/(°/s)  */
#define MPU9250_ACCEL_SENS_2G_LSB      16384    /* LSB/g      */

/**
 * @brief Structure pour les données brutes (accéléromètre + gyroscope)
 *        Format : entier signé 16 bits, complément à deux.
 */
typedef struct
{
    int16_t ax;
    int16_t ay;
    int16_t az;

    int16_t gx;
    int16_t gy;
    int16_t gz;
} mpu9250_raw_data_t;

/**
 * @brief Lecture du registre WHO_AM_I et vérification de l'identité.
 *
 * @param[out] who_am_i  Pointeur sur la variable qui recevra la valeur brute.
 * @return HAL_OK si la lecture s'est bien déroulée.
 */
HAL_StatusTypeDef mpu9250_read_who_am_i(uint8_t *who_am_i);

/**
 * @brief Initialisation basique du MPU9250 (accéléro + gyro).
 *
 * - Sort du mode sleep
 * - Sélectionne une source d’horloge correcte
 * - Configure l’échelle du gyro (±250 dps)
 * - Configure l’échelle de l’accéléro (±2 g)
 * - Configuration simple des filtres et du taux d’échantillonnage
 *
 * @return HAL_OK si tout s'est bien passé.
 */
HAL_StatusTypeDef mpu9250_init(void);

/**
 * @brief Lecture des 6 axes (accéléro + gyro) en une seule transaction I2C.
 *
 * @param[out] data  Structure recevant les données brutes.
 * @return HAL_OK si la lecture s'est bien déroulée.
 */
HAL_StatusTypeDef mpu9250_read_raw(mpu9250_raw_data_t *data);

/**
 * @brief Conversion des données brutes d'accélération en milli-g (mg).
 *
 *        Exemple : ax_mg = 1000 -> 1 g
 *
 * @param[in]  raw   Données brutes (LSB)
 * @param[out] ax_mg Accélération axe X en milli-g
 * @param[out] ay_mg Accélération axe Y en milli-g
 * @param[out] az_mg Accélération axe Z en milli-g
 */
void mpu9250_convert_accel_mg(const mpu9250_raw_data_t *raw,
                              int32_t *ax_mg, int32_t *ay_mg, int32_t *az_mg);

/**
 * @brief Conversion des données brutes de gyro en milli-deg/s (mdps).
 *
 *        Exemple : gx_mdps = 1000 -> 1 °/s
 *
 * @param[in]  raw      Données brutes (LSB)
 * @param[out] gx_mdps  Vitesse angulaire X en milli-deg/s
 * @param[out] gy_mdps  Vitesse angulaire Y en milli-deg/s
 * @param[out] gz_mdps  Vitesse angulaire Z en milli-deg/s
 */
void mpu9250_convert_gyro_mdps(const mpu9250_raw_data_t *raw,
                               int32_t *gx_mdps, int32_t *gy_mdps, int32_t *gz_mdps);

#endif /* MPU9250_H_ */
