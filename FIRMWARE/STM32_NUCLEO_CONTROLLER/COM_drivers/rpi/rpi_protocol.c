/*
 * rpi_protocol.c
 *
 *  Created on: Dec 12, 2025
 *      Author: penel
 */

#include "rpi_protocol.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* UART et état capteurs */
static UART_HandleTypeDef *s_huart = NULL;
static const sensors_state_t *s_state = NULL;

/* Coefficient K en 1/100 */
static volatile int32_t s_K_centi = 500; /* 1.00 */

/* Buffer commande */
#define CMD_BUF_LEN 32
static char g_cmd_buf[CMD_BUF_LEN];
static volatile uint8_t g_cmd_idx   = 0;
static volatile uint8_t g_cmd_ready = 0;

/* Réception IT (1 byte) */
static uint8_t s_rx_byte = 0;

static void Proto_SendString(const char *s)
{
    if (s_huart == NULL || s == NULL)
        return;

    HAL_UART_Transmit(s_huart, (uint8_t*)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

static void Proto_HandleCommand(const char *cmd)
{
    char tx[48];

    if (cmd == NULL || s_state == NULL)
        return;

    /* GET_T */
    if (strncmp(cmd, "GET_T", 5) == 0)
    {
        int32_t t = s_state->temp_centi;
        char sign = '+';
        if (t < 0) { sign = '-'; t = -t; }

        int32_t t_int  = t / 100;
        int32_t t_frac = t % 100;

        snprintf(tx, sizeof(tx), "T=%c%02ld.%02ld_C\r\n",
                 sign, (long)t_int, (long)t_frac);
        Proto_SendString(tx);
    }
    /* GET_P */
    else if (strncmp(cmd, "GET_P", 5) == 0)
    {
        uint32_t p = s_state->press_pa;
        snprintf(tx, sizeof(tx), "P=%luPa\r\n", (unsigned long)p);
        Proto_SendString(tx);
    }
    /* SET_K=1234 */
    else if (strncmp(cmd, "SET_K=", 6) == 0)
    {
        const char *value_str = cmd + 6;
        s_K_centi = (int32_t)atoi(value_str);

        snprintf(tx, sizeof(tx), "SET_K=OK\r\n");
        Proto_SendString(tx);
    }
    /* GET_K */
    else if (strncmp(cmd, "GET_K", 5) == 0)
    {
        int32_t k = s_K_centi;
        int32_t k_int  = k / 100;
        int32_t k_frac = k % 100;
        if (k_frac < 0) k_frac = -k_frac;

        snprintf(tx, sizeof(tx), "K=%ld.%02ld000\r\n",
                 (long)k_int, (long)k_frac);
        Proto_SendString(tx);
    }
    /* GET_A */
    else if (strncmp(cmd, "GET_A", 5) == 0)
    {
        int32_t a = s_state->angle_milli; /* 0.001° */
        int32_t a_int  = a / 1000;
        int32_t a_frac = a % 1000;
        if (a_frac < 0) a_frac = -a_frac;

        snprintf(tx, sizeof(tx), "A=%ld.%03ld0\r\n",
                 (long)a_int, (long)a_frac);
        Proto_SendString(tx);
    }
    else
    {
        snprintf(tx, sizeof(tx), "ERR=CMD\r\n");
        Proto_SendString(tx);
    }
}

void RpiProto_Init(UART_HandleTypeDef *huart_rpi, const sensors_state_t *state)
{
    s_huart = huart_rpi;
    s_state = state;

    g_cmd_idx   = 0;
    g_cmd_ready = 0;
    s_rx_byte   = 0;

    /* Lance RX IT sur UART1 */
    HAL_UART_Receive_IT(s_huart, &s_rx_byte, 1);

    printf("=== Protocole UART1 pret (Raspberry Pi) ===\r\n");
}

void RpiProto_OnRxByte(uint8_t ch)
{
    /* Construit la ligne */
    if (ch == '\r' || ch == '\n')
    {
        if (g_cmd_idx > 0)
        {
            g_cmd_buf[g_cmd_idx] = '\0';
            g_cmd_ready = 1;
        }
        g_cmd_idx = 0;
    }
    else
    {
        if (g_cmd_idx < (CMD_BUF_LEN - 1))
        {
            g_cmd_buf[g_cmd_idx++] = (char)ch;
        }
        else
        {
            g_cmd_idx = 0; /* overflow -> reset */
        }
    }
}

void RpiProto_Task(void)
{
    if (g_cmd_ready)
    {
        g_cmd_ready = 0;
        Proto_HandleCommand(g_cmd_buf);
    }
}

int32_t RpiProto_GetK_centi(void)
{
    return s_K_centi;
}
