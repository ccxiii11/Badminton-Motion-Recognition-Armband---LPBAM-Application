#ifndef __BLE_H
#define __BLE_H

#include <stdint.h>

void BLE_Init(void);
int BLE_WaitConnected(uint32_t timeout_ms);
uint8_t BLE_IsConnected(void);
void BLE_Poll(void);
int BLE_SendData(const uint8_t *data, uint16_t size);
int BLE_SendTraj(const int16_t *xyz, uint16_t point_count);

#endif
