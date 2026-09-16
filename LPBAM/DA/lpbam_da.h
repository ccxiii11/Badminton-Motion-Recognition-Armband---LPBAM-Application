/* USER CODE BEGIN Header */
/**
  **********************************************************************************************************************
  * @file    lpbam_da.h
  * @author  MCD Application Team
  * @brief   Header for LPBAM DA application
  **********************************************************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  **********************************************************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/
#ifndef LPBAM_DA_H
#define LPBAM_DA_H

/* Includes ----------------------------------------------------------------------------------------------------------*/
#include "main.h"
#include "stm32_lpbam.h"

/* Exported functions ------------------------------------------------------------------------------------------------*/
/* DA application initialization */
void MX_DA_Init(void);

/* DA application device scenario initialization */
void MX_DA_device_Init(void);

/* DA application device scenario de-initialization */
void MX_DA_device_DeInit(void);

/* DA application device scenario build */
void MX_DA_device_Build(void);

/* DA application device scenario link */
void MX_DA_device_Link(DMA_HandleTypeDef *hdma);

/* DA application device scenario unlink */
void MX_DA_device_UnLink(DMA_HandleTypeDef *hdma);

/* DA application device scenario start */
void MX_DA_device_Start(DMA_HandleTypeDef *hdma);

/* DA application device scenario stop */
void MX_DA_device_Stop(DMA_HandleTypeDef *hdma);

#endif /* LPBAM_DA_H */
