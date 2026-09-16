#include "lsm_reg.h"
#include "stdint.h"
#ifndef __LSM_H
#define __LSM_H

void LSM_WriteReg(uint8_t RegAddress, uint8_t Data);
uint8_t LSM_ReadReg(uint8_t RegAddress);
void LSM_Init(void);
void LSM_ConfigureMotionStartInt(void);
void LSM_ClearMotionInt(void);
uint8_t LSM_WaitReady(uint32_t timeout_ms);
uint8_t LSM_GetID(void);
uint8_t LSM_ReadMlcResult(void);
#endif
