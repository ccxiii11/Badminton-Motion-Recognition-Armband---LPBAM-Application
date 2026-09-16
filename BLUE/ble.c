#include "ble.h"
#include <string.h>
#include "usart.h"
#include "stm32u5xx_hal.h"
/*定义蓝牙参数 */
#define M301H_RX_BUF_SIZE    1024U
#define M301H_CMD_TIMEOUT_MS 3000U

static uint8_t m301h_buffer[M301H_RX_BUF_SIZE];
static uint16_t m301h_buffer_size;
static uint8_t s_ble_connected;

static void m301h_ParseUrc(const uint8_t *buf, uint16_t len);
static int m301h_SendCmd(const uint8_t *cmd, uint16_t size);
static void m301h_ProcessRx(uint32_t timeout_ms);

static void m301h_ParseUrc(const uint8_t *buf, uint16_t len)
{
    char tmp[M301H_RX_BUF_SIZE + 1U];
    uint16_t copy_len = len;

    if (copy_len >= sizeof(tmp))
    {
        copy_len = (uint16_t)(sizeof(tmp) - 1U);
    }
    memcpy(tmp, buf, copy_len);
    tmp[copy_len] = '\0';

    if (strstr(tmp, "+BLESTA:1") != NULL)
    {
        s_ble_connected = 1U;
    }
    else if (strstr(tmp, "+BLESTA:0") != NULL)
    {
        s_ble_connected = 0U;
    }
}
/*接收蓝牙反馈*/
static void m301h_ProcessRx(uint32_t timeout_ms)
{
    HAL_UARTEx_ReceiveToIdle(&huart1, m301h_buffer, M301H_RX_BUF_SIZE,
                              &m301h_buffer_size, timeout_ms);
    if (m301h_buffer_size > 0U)
    {
        m301h_ParseUrc(m301h_buffer, m301h_buffer_size);
    }
}

static int m301h_SendCmd(const uint8_t *cmd, uint16_t size)
{
    uint32_t t0 = HAL_GetTick();

    memset(m301h_buffer, 0, M301H_RX_BUF_SIZE);
    if (HAL_UART_Transmit(&huart1, (uint8_t *)cmd, size, 1000) != HAL_OK)
    {
        return -1;
    }
    do
    {
        m301h_ProcessRx(200);
        if (strstr((char *)m301h_buffer, "OK") != NULL)
        {
            return 0;
        }
        if (strstr((char *)m301h_buffer, "+ERROR:") != NULL)
        {
            return -1;
        }
    } while ((HAL_GetTick() - t0) < M301H_CMD_TIMEOUT_MS);

    return -1;
}
/*初始化蓝牙设置蓝牙参数*/
void BLE_Init(void)
{
    s_ble_connected = 0U;
    MX_USART1_UART_Init();
    HAL_Delay(100);
    m301h_SendCmd((const uint8_t *)"AT\r\n", 4);
    m301h_SendCmd((const uint8_t *)"AT+NAME=STM32U5_BLE\r\n", 21);
    m301h_SendCmd((const uint8_t *)"AT+MAC=72:62:77:33:08:55\r\n", 25);
    m301h_SendCmd((const uint8_t *)"AT+ADVDATA=0201060303F0FF03190000\r\n", 35);
    m301h_SendCmd((const uint8_t *)"AT+UUIDS=FE00\r\n", 15);
    m301h_SendCmd((const uint8_t *)"AT+UUIDW=FE01\r\n", 15);
    m301h_SendCmd((const uint8_t *)"AT+UUIDN=FE02\r\n", 15);
    m301h_SendCmd((const uint8_t *)"AT+RESET\r\n", 10);
    HAL_Delay(500);
    m301h_ProcessRx(200);
}

uint8_t BLE_IsConnected(void)
{
    return s_ble_connected;
}

int BLE_WaitConnected(uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();

    if (s_ble_connected != 0U)
    {
        return 0;
    }

    while (1)
    {
        m301h_ProcessRx(500);
        if (s_ble_connected != 0U)
        {
            return 0;
        }
        if ((timeout_ms != 0U) && ((HAL_GetTick() - t0) >= timeout_ms))
        {
            return -1;
        }
    }
}
void BLE_Poll(void)
{
    m301h_ProcessRx(50);
}

int BLE_SendData(const uint8_t *data, uint16_t size)
{
    if ((s_ble_connected == 0U) || (data == NULL) || (size == 0U))
    {
        return -1;
    }

    if (HAL_UART_Transmit(&huart1, (uint8_t *)data, size, 1000) != HAL_OK)
    {
        return -1;
    }

    return 0;
}

int BLE_SendTraj(const int16_t *xyz, uint16_t point_count)
{
    uint8_t pkt[4U + (3U * 50U * 2U * 3U)];
    uint16_t payload_len;
    uint16_t i;
    if ((s_ble_connected == 0U) || (xyz == NULL) || (point_count == 0U) || (point_count > 50U))
    {
        return -1;
    }
   /*打包数据" T J 1 数量 "*/
    pkt[0] = (uint8_t)'T';
    pkt[1] = (uint8_t)'J';
    pkt[2] = 1U;
    pkt[3] = (uint8_t)point_count;
    payload_len = 4U;

    for (i = 0U; i < point_count; i++)
    {
        pkt[payload_len++] = (uint8_t)(xyz[i * 6U] & 0xFF);
        pkt[payload_len++] = (uint8_t)((uint16_t)xyz[i * 6U] >> 8);
        pkt[payload_len++] = (uint8_t)(xyz[i * 6U + 1U] & 0xFF);
        pkt[payload_len++] = (uint8_t)((uint16_t)xyz[i * 6U + 1U] >> 8);
        pkt[payload_len++] = (uint8_t)(xyz[i * 6U + 2U] & 0xFF);
        pkt[payload_len++] = (uint8_t)((uint16_t)xyz[i * 6U + 2U] >> 8);
        pkt[payload_len++] = (uint8_t)(xyz[i *6U+2U] & 0xFF);
        pkt[payload_len++] = (uint8_t)((uint16_t)xyz[i * 6U] >> 8);
        pkt[payload_len++] = (uint8_t)(xyz[i * 6U + 1U] & 0xFF);
        pkt[payload_len++] = (uint8_t)((uint16_t)xyz[i * 6U + 1U] >> 8);
        pkt[payload_len++] = (uint8_t)(xyz[i * 6U + 2U] & 0xFF);
        pkt[payload_len++] = (uint8_t)((uint16_t)xyz[i * 6U + 2U] >> 8);
    }

    return BLE_SendData(pkt, payload_len);
}
