#ifndef LPBAM_USER_H
#define LPBAM_USER_H

#include "main.h"
#include "stm32_lpbam.h"

#define LPBAM_LPTIM_CLK_HZ    32768U
#define LPBAM_LPTIM_PERIOD    ((uint32_t)((LPBAM_LPTIM_CLK_HZ * IMU_SAMPLE_DT_MS) / 1000U) - 1U)

extern DMA_QListTypeDef config_Q;

void LPBAM_User_BuildQueue(void);
void LPBAM_User_LoadTraj(imu_traj_frame_t *dst, uint32_t frame_count);
uint32_t LPBAM_User_MaxBatchFrames(void);

void LPBAM_User_BeginSampling(void);
void LPBAM_User_StopSampleTimer(void);
uint8_t LPBAM_User_IsSampleDone(void);
uint8_t LPBAM_User_HasSampleError(void);

#endif
