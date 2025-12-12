/*
 * bmp280.c
 *
 *  Created on: Nov 26, 2025
 *      Author: penel
 */

#include "bmp280.h"

/* --------------------------------------------------------------------------
 * Fonctions internes (static) : accès I2C de bas niveau
 * -------------------------------------------------------------------------- */

/**
 * @brief  Lecture de plusieurs octets consécutifs dans le BMP280.
 */
static HAL_StatusTypeDef bmp280_read_regs(BMP280_HandleTypedef *dev,
                                          uint8_t reg,
                                          uint8_t *pData,
                                          uint16_t size)
{
    return HAL_I2C_Mem_Read(dev->hi2c,
                            dev->i2c_addr,
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            pData,
                            size,
                            HAL_MAX_DELAY);
}

/**
 * @brief  Écriture d'un octet dans un registre du BMP280.
 */
static HAL_StatusTypeDef bmp280_write_reg(BMP280_HandleTypedef *dev,
                                          uint8_t reg,
                                          uint8_t value)
{
    return HAL_I2C_Mem_Write(dev->hi2c,
                             dev->i2c_addr,
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &value,
                             1,
                             HAL_MAX_DELAY);
}

/* --------------------------------------------------------------------------
 * Fonctions publiques : initialisation et configuration
 * -------------------------------------------------------------------------- */

HAL_StatusTypeDef BMP280_ReadCalibration(BMP280_HandleTypedef *dev)
{
    HAL_StatusTypeDef ret;
    uint8_t buf[BMP280_CALIB_LENGTH];

    /* Lecture en bloc de tous les registres d'étalonnage (0x88 -> 0xA1) */
    ret = bmp280_read_regs(dev, BMP280_REG_CALIB_START, buf, BMP280_CALIB_LENGTH);
    if (ret != HAL_OK)
    {
        return ret;
    }

    /* Reconstruction des coefficients à partir des octets (LSB puis MSB) */
    dev->calib.dig_T1 = (uint16_t)(buf[1]  << 8 | buf[0]);
    dev->calib.dig_T2 = (int16_t) (buf[3]  << 8 | buf[2]);
    dev->calib.dig_T3 = (int16_t) (buf[5]  << 8 | buf[4]);

    dev->calib.dig_P1 = (uint16_t)(buf[7]  << 8 | buf[6]);
    dev->calib.dig_P2 = (int16_t) (buf[9]  << 8 | buf[8]);
    dev->calib.dig_P3 = (int16_t) (buf[11] << 8 | buf[10]);
    dev->calib.dig_P4 = (int16_t) (buf[13] << 8 | buf[12]);
    dev->calib.dig_P5 = (int16_t) (buf[15] << 8 | buf[14]);
    dev->calib.dig_P6 = (int16_t) (buf[17] << 8 | buf[16]);
    dev->calib.dig_P7 = (int16_t) (buf[19] << 8 | buf[18]);
    dev->calib.dig_P8 = (int16_t) (buf[21] << 8 | buf[20]);
    dev->calib.dig_P9 = (int16_t) (buf[23] << 8 | buf[22]);

    /* Les octets 0xA0..0xA1 sont réservés et ignorés ici */

    return HAL_OK;
}

HAL_StatusTypeDef BMP280_ReadID(BMP280_HandleTypedef *dev, uint8_t *id)
{
    /* Lecture d'un seul octet : registre ID (0xD0) */
    return bmp280_read_regs(dev, BMP280_REG_ID, id, 1);
}

HAL_StatusTypeDef BMP280_ConfigDefault(BMP280_HandleTypedef *dev)
{
    HAL_StatusTypeDef ret;
    uint8_t value   = BMP280_CTRL_MEAS_DEFAULT;
    uint8_t readback = 0;

    /* Écriture de la configuration par défaut dans CTRL_MEAS */
    ret = bmp280_write_reg(dev, BMP280_REG_CTRL_MEAS, value);
    if (ret != HAL_OK)
    {
        return ret;
    }

    /* Lecture immédiate pour vérifier que la valeur a bien été prise en compte */
    ret = bmp280_read_regs(dev, BMP280_REG_CTRL_MEAS, &readback, 1);
    if (ret != HAL_OK)
    {
        return ret;
    }

    /* Ici, on ne fait qu'un contrôle simple :
     * l'utilisateur peut vérifier l'égalité value == readback
     * s'il souhaite ajouter des messages de debug dans l'application.
     */
    return HAL_OK;
}

HAL_StatusTypeDef BMP280_ReadRaw(BMP280_HandleTypedef *dev,
                                 uint32_t *raw_temp,
                                 uint32_t *raw_press)
{
    HAL_StatusTypeDef ret;
    uint8_t data[6];

    /* Lecture des 6 octets : press_msb..press_xlsb, temp_msb..temp_xlsb */
    ret = bmp280_read_regs(dev, BMP280_REG_PRESS_TEMP, data, 6);
    if (ret != HAL_OK)
    {
        *raw_temp  = 0;
        *raw_press = 0;
        return ret;
    }

    /* Reconstruction des valeurs brutes 20 bits (non signées) */
    *raw_press  = ((uint32_t)data[0] << 12) |
                  ((uint32_t)data[1] << 4)  |
                  ((uint32_t)data[2] >> 4);

    *raw_temp   = ((uint32_t)data[3] << 12) |
                  ((uint32_t)data[4] << 4)  |
                  ((uint32_t)data[5] >> 4);

    return HAL_OK;
}

HAL_StatusTypeDef BMP280_Init(BMP280_HandleTypedef *dev,
                              I2C_HandleTypeDef *hi2c,
                              uint8_t i2c_addr)
{
    HAL_StatusTypeDef ret;
    uint8_t id = 0;

    /* Stockage des paramètres dans le handle */
    dev->hi2c     = hi2c;
    dev->i2c_addr = i2c_addr;
    dev->t_fine   = 0;

    /* Vérification du chip ID */
    ret = BMP280_ReadID(dev, &id);
    if (ret != HAL_OK)
    {
        return ret;
    }

    /* L'utilisateur peut vérifier id == BMP280_CHIP_ID dans l'application
     * (ex: afficher un message sur l'UART).
     */

    /* Lecture des coefficients d'étalonnage */
    ret = BMP280_ReadCalibration(dev);
    if (ret != HAL_OK)
    {
        return ret;
    }

    /* Configuration de mesure par défaut */
    ret = BMP280_ConfigDefault(dev);
    if (ret != HAL_OK)
    {
        return ret;
    }

    return HAL_OK;
}

/* --------------------------------------------------------------------------
 * Fonctions publiques : compensation température / pression (entier 32 bits)
 * -------------------------------------------------------------------------- */

/**
 * @brief  Raccourcis internes pour simplifier la lecture du code
 *         (équivalent aux #define dig_T1 ... que tu utilisais).
 */
static inline uint16_t bmp_dig_T1(BMP280_HandleTypedef *dev) { return dev->calib.dig_T1; }
static inline int16_t  bmp_dig_T2(BMP280_HandleTypedef *dev) { return dev->calib.dig_T2; }
static inline int16_t  bmp_dig_T3(BMP280_HandleTypedef *dev) { return dev->calib.dig_T3; }

static inline uint16_t bmp_dig_P1(BMP280_HandleTypedef *dev) { return dev->calib.dig_P1; }
static inline int16_t  bmp_dig_P2(BMP280_HandleTypedef *dev) { return dev->calib.dig_P2; }
static inline int16_t  bmp_dig_P3(BMP280_HandleTypedef *dev) { return dev->calib.dig_P3; }
static inline int16_t  bmp_dig_P4(BMP280_HandleTypedef *dev) { return dev->calib.dig_P4; }
static inline int16_t  bmp_dig_P5(BMP280_HandleTypedef *dev) { return dev->calib.dig_P5; }
static inline int16_t  bmp_dig_P6(BMP280_HandleTypedef *dev) { return dev->calib.dig_P6; }
static inline int16_t  bmp_dig_P7(BMP280_HandleTypedef *dev) { return dev->calib.dig_P7; }
static inline int16_t  bmp_dig_P8(BMP280_HandleTypedef *dev) { return dev->calib.dig_P8; }
static inline int16_t  bmp_dig_P9(BMP280_HandleTypedef *dev) { return dev->calib.dig_P9; }

/* Retourne la température en 0.01 °C (5123 -> 51.23 °C) */
BMP280_S32_t BMP280_Compensate_T_int32(BMP280_HandleTypedef *dev,
                                       BMP280_S32_t adc_T)
{
    BMP280_S32_t var1, var2, T;

    var1 = ((((adc_T >> 3) - ((BMP280_S32_t)bmp_dig_T1(dev) << 1)) *
             ((BMP280_S32_t)bmp_dig_T2(dev))) >> 11);

    var2 = (((((adc_T >> 4) - ((BMP280_S32_t)bmp_dig_T1(dev))) *
              ((adc_T >> 4) - ((BMP280_S32_t)bmp_dig_T1(dev)))) >> 12) *
              ((BMP280_S32_t)bmp_dig_T3(dev)) >> 14);

    dev->t_fine = var1 + var2;
    T = (dev->t_fine * 5 + 128) >> 8;

    return T;
}

/* Retourne la pression en Pa (entier non signé 32 bits) */
BMP280_U32_t BMP280_Compensate_P_int32(BMP280_HandleTypedef *dev,
                                       BMP280_S32_t adc_P)
{
    BMP280_S32_t var1, var2;
    BMP280_U32_t p;

    var1 = (dev->t_fine >> 1) - (BMP280_S32_t)64000;

    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * (BMP280_S32_t)bmp_dig_P6(dev);
    var2 = var2 + ((var1 * (BMP280_S32_t)bmp_dig_P5(dev)) << 1);
    var2 = (var2 >> 2) + ((BMP280_S32_t)bmp_dig_P4(dev) << 16);

    var1 = (((((BMP280_S32_t)bmp_dig_P3(dev) *
               (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) +
             (((BMP280_S32_t)bmp_dig_P2(dev) * var1) >> 1)) >> 18);

    var1 = ((((BMP280_S32_t)32768 + var1) * (BMP280_S32_t)bmp_dig_P1(dev)) >> 15);

    if (var1 == 0)
    {
        /* Évite la division par zéro */
        return 0;
    }

    p = (((BMP280_U32_t)(((BMP280_S32_t)1048576) - adc_P) - (var2 >> 12)) * 3125u);

    if (p < 0x80000000u)
    {
        p = (p << 1) / (BMP280_U32_t)var1;
    }
    else
    {
        p = (p / (BMP280_U32_t)var1) * 2u;
    }

    var1 = ((BMP280_S32_t)bmp_dig_P9(dev) *
           (BMP280_S32_t)(((p >> 3) * (p >> 3)) >> 13)) >> 12;

    var2 = (((BMP280_S32_t)(p >> 2)) * (BMP280_S32_t)bmp_dig_P8(dev)) >> 13;

    p = (BMP280_U32_t)((BMP280_S32_t)p + ((var1 + var2 + bmp_dig_P7(dev)) >> 4));

    return p;
}

