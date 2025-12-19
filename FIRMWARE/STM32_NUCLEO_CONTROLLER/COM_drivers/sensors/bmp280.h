/*
 * bmp280.h
 *
 *  Created on: Nov 26, 2025
 *      Author: penel
 */

#ifndef BMP280_H
#define BMP280_H

#include "main.h"   // Contient normalement stm32f4xx_hal.h et les types HAL
#include <stdint.h>

/* --------------------------------------------------------------------------
 * Définitions de types entiers spécifiques BMP280
 * -------------------------------------------------------------------------- */

/* Entier signé 32 bits, utilisé par les formules de compensation */
typedef int32_t  BMP280_S32_t;
/* Entier non signé 32 bits, utilisé par les formules de compensation */
typedef uint32_t BMP280_U32_t;

/* --------------------------------------------------------------------------
 * Définitions des registres et de l'adresse I2C
 * -------------------------------------------------------------------------- */

/* Adresse I2C par défaut du BMP280 (SDO connecté à VDDIO -> 0x77) */
#define BMP280_I2C_ADDR_DEFAULT   (0x77 << 1)

/* Registres principaux */
#define BMP280_REG_ID             0xD0
#define BMP280_REG_CTRL_MEAS      0xF4
#define BMP280_REG_CALIB_START    0x88
#define BMP280_CALIB_LENGTH       26    /* 0x88 -> 0xA1 inclus */
#define BMP280_REG_PRESS_TEMP     0xF7  /* début des registres press/temp */

/* Valeur attendue dans le registre ID pour un BMP280 */
#define BMP280_CHIP_ID            0x58

/* Valeur de configuration "par défaut" :
 *  - mode normal
 *  - oversampling pression x16
 *  - oversampling température x2
 * (voir datasheet : registre CTRL_MEAS)
 */
#define BMP280_CTRL_MEAS_DEFAULT  0x57

/* --------------------------------------------------------------------------
 * Structure des coefficients d'étalonnage (datasheet BMP280)
 * -------------------------------------------------------------------------- */

/**
 * @brief  Coefficients d'étalonnage internes du BMP280.
 *         Ces valeurs sont lues une fois au démarrage dans la NVM du capteur.
 */
typedef struct
{
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} BMP280_CalibData_t;

/* --------------------------------------------------------------------------
 * Handle principal du BMP280
 * -------------------------------------------------------------------------- */

/**
 * @brief  Handle de haut niveau pour piloter un BMP280 via I2C.
 *
 *  - hi2c     : pointeur sur le handle I2C HAL utilisé (ex: &hi2c1)
 *  - i2c_addr : adresse I2C (7 bits décalés à gauche, ex: 0x77<<1)
 *  - calib    : coefficients d'étalonnage (remplis une fois à l'init)
 *  - t_fine   : variable interne utilisée par la compensation (datasheet)
 */
typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint8_t            i2c_addr;
    BMP280_CalibData_t calib;
    BMP280_S32_t       t_fine;
} BMP280_HandleTypedef;

/* --------------------------------------------------------------------------
 * Prototypes des fonctions publiques
 * -------------------------------------------------------------------------- */

/**
 * @brief  Initialise le handle BMP280 (liaison I2C + adresse) et lit l'étalonnage.
 *
 * @param  dev      Pointeur sur le handle BMP280 à initialiser.
 * @param  hi2c     Pointeur sur le handle I2C HAL (ex: &hi2c1).
 * @param  i2c_addr Adresse I2C du capteur (ex: BMP280_I2C_ADDR_DEFAULT).
 *
 * @retval HAL_OK   En cas de succès.
 * @retval autre    Code d'erreur HAL en cas de problème I2C.
 */
HAL_StatusTypeDef BMP280_Init(BMP280_HandleTypedef *dev,
                              I2C_HandleTypeDef *hi2c,
                              uint8_t i2c_addr);

/**
 * @brief  Lit le registre ID du BMP280.
 *
 * @param  dev Pointeur sur le handle BMP280.
 * @param  id  Pointeur où stocker l'ID lu.
 *
 * @retval HAL_OK   En cas de succès.
 * @retval autre    Code d'erreur HAL en cas de problème I2C.
 */
HAL_StatusTypeDef BMP280_ReadID(BMP280_HandleTypedef *dev, uint8_t *id);

/**
 * @brief  Configure le capteur avec la configuration "par défaut".
 *
 *         Configuration par défaut : BMP280_CTRL_MEAS_DEFAULT
 *         (mode normal, oversampling pression x16, température x2).
 *
 * @param  dev Pointeur sur le handle BMP280.
 *
 * @retval HAL_OK   En cas de succès.
 * @retval autre    Code d'erreur HAL en cas de problème I2C.
 */
HAL_StatusTypeDef BMP280_ConfigDefault(BMP280_HandleTypedef *dev);

/**
 * @brief  Lit les coefficients d'étalonnage dans la NVM interne du capteur.
 *
 * @param  dev Pointeur sur le handle BMP280.
 *
 * @retval HAL_OK   En cas de succès.
 * @retval autre    Code d'erreur HAL en cas de problème I2C.
 */
HAL_StatusTypeDef BMP280_ReadCalibration(BMP280_HandleTypedef *dev);

/**
 * @brief  Lit les valeurs brutes (non compensées) de température et pression.
 *
 * @param  dev       Pointeur sur le handle BMP280.
 * @param  raw_temp  Pointeur où stocker la température brute 20 bits.
 * @param  raw_press Pointeur où stocker la pression brute 20 bits.
 *
 * @retval HAL_OK   En cas de succès.
 * @retval autre    Code d'erreur HAL en cas de problème I2C.
 */
HAL_StatusTypeDef BMP280_ReadRaw(BMP280_HandleTypedef *dev,
                                 uint32_t *raw_temp,
                                 uint32_t *raw_press);

/**
 * @brief  Compense la température à partir de la valeur brute (format entier).
 *
 *         Utilise les coefficients d'étalonnage stockés dans dev->calib.
 *         Met à jour dev->t_fine, nécessaire pour la compensation de pression.
 *
 * @param  dev   Pointeur sur le handle BMP280.
 * @param  adc_T Valeur brute de température (20 bits, dans un entier 32 bits).
 *
 * @return Température en 0.01 °C (ex : 5123 -> 51.23 °C).
 */
BMP280_S32_t BMP280_Compensate_T_int32(BMP280_HandleTypedef *dev,
                                       BMP280_S32_t adc_T);

/**
 * @brief  Compense la pression à partir de la valeur brute (format entier).
 *
 *         Utilise dev->t_fine mis à jour par BMP280_Compensate_T_int32().
 *
 * @param  dev   Pointeur sur le handle BMP280.
 * @param  adc_P Valeur brute de pression (20 bits, dans un entier 32 bits).
 *
 * @return Pression en Pascal (Pa), au format entier 32 bits non signé.
 */
BMP280_U32_t BMP280_Compensate_P_int32(BMP280_HandleTypedef *dev,
                                       BMP280_S32_t adc_P);

#endif /* BMP280_H */

