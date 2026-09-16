/* USER CODE BEGIN Header */
/**
  **********************************************************************************************************************
  * @file    lpbam_da_device_config.c
  * @author  MCD Application Team
  * @brief   Provide LPBAM DA application device configuration services.
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
/* Includes ----------------------------------------------------------------------------------------------------------*/
#include "lpbam_da.h"

/* Private typedef ---------------------------------------------------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define --------------------------------------------------------------------------------------------------*/
#define CONFIG_Q_IDX (0U)
#define DMA_TIMEOUT_DURATION (0x1000U)

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -----------------------------------------------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* External variables ------------------------------------------------------------------------------------------------*/
/* IP handler declaration */
extern I2C_HandleTypeDef hi2c3;
extern LPTIM_HandleTypeDef hlptim1;

/* LPBAM queues declaration */
extern DMA_QListTypeDef config_Q;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private function prototypes ---------------------------------------------------------------------------------------*/

/* LPBAM I2C3 management APIs */
static void MX_I2C3_Init(void);
static void MX_I2C3_MspInit(I2C_HandleTypeDef *hi2c3);
static void MX_I2C3_MspDeInit(I2C_HandleTypeDef *hi2c3);
static void MX_I2C3_DeInit(void);

/* LPBAM LPTIM1 management APIs */
static void MX_LPTIM1_Init(void);
static void MX_LPTIM1_MspInit(LPTIM_HandleTypeDef *hlptim1);
static void MX_LPTIM1_MspDeInit(LPTIM_HandleTypeDef *hlptim1);
static void MX_LPTIM1_DeInit(void);

/* LPBAM autonomous mode management APIs */
static void MX_AutonomousMode_Init(void);
static void MX_AutonomousMode_DeInit(void);

/* LPBAM queue linking/unlinking APIs */
static void MX_config_Q_Link(DMA_HandleTypeDef *hdma);
static void MX_config_Q_UnLink(DMA_HandleTypeDef *hdma);

/* LPBAM DMA user callback APIs */
static void MX_config_Q_DMA_Error_Callback(DMA_HandleTypeDef *hdma);
static void MX_config_Q_DMA_TC_Callback(DMA_HandleTypeDef *hdma);
/* LPBAM DMA NVIC API */
static void MX_DMA_NVIC_Config(DMA_HandleTypeDef *hdma, uint32_t PreemptPriority, uint32_t SubPriority);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Exported functions ----------------------------------------------------------------------------------------------*/
/**
  * @brief  DA application device scenario initialization
  * @param  None
  * @retval None
  */
void MX_DA_device_Init(void)
{
  /* LPBAM I2C3 initialization */
  MX_I2C3_Init();

  /* LPBAM LPTIM1 initialization */
  MX_LPTIM1_Init();

  /* Autonomous Mode initialization */
  MX_AutonomousMode_Init();

  /* USER CODE BEGIN DA_device_Init */

  /* USER CODE END DA_device_Init */
}

/**
  * @brief DA application device scenario de-initialization
  * @param None
  * @retval None
  */
void MX_DA_device_DeInit(void)
{
  /* LPBAM I2C3 De-initialization */
  MX_I2C3_DeInit();

  /* LPBAM LPTIM1 De-initialization */
  MX_LPTIM1_DeInit();

  /* Autonomous mode de-initialization */
  MX_AutonomousMode_DeInit();

  /* USER CODE BEGIN DA_device_DeInit */

  /* USER CODE END DA_device_DeInit */
}

/**
  * @brief DA application device scenario link
  * @param None
  * @retval None
  */
void MX_DA_device_Link(DMA_HandleTypeDef *hdma)
{
  /* USER CODE BEGIN DA_device_Link 0 */

  /* USER CODE END DA_device_Link 0 */

  /* Link config queue to DMA channel */
  MX_config_Q_Link(&hdma[CONFIG_Q_IDX]);

  /* USER CODE BEGIN LINK CONFIG_Q_IDX */

  /* USER CODE END LINK CONFIG_Q_IDX */

  /* USER CODE BEGIN DA_device_Link 1 */

  /* USER CODE END DA_device_Link 1 */
}

/**
  * @brief DA application device scenario unlink
  * @param hdma :Pointer to a DMA_HandleTypeDef structure that contains the configuration information for the specified
  *              DMA Channel
  * @retval None
  */
void MX_DA_device_UnLink(DMA_HandleTypeDef *hdma)
{
  /* USER CODE BEGIN DA_device_UnLink 0 */

  /* USER CODE END DA_device_UnLink 0 */

  /* LPBAM unLink config queue to DMA channel */
  MX_config_Q_UnLink(&hdma[CONFIG_Q_IDX]);

  /* USER CODE BEGIN UNLINK CONFIG_Q_IDX */

  /* USER CODE END UNLINK CONFIG_Q_IDX */

  /* USER CODE BEGIN DA_device_UnLink 1 */

  /* USER CODE END DA_device_UnLink 1 */
}

/**
  * @brief DA application device scenario start
  * @retval None
  */
void MX_DA_device_Start(DMA_HandleTypeDef *hdma)
{
  /* LPBAM start DMA channel in linked-list mode */
  if (HAL_DMAEx_List_Start(&hdma[CONFIG_Q_IDX]) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN DA_device_Start */

  /* USER CODE END DA_device_Start */
}

/**
  * @brief DA application device scenario stop
  * @retval None
  */
void MX_DA_device_Stop(DMA_HandleTypeDef *hdma)
{
  /* LPBAM stop DMA channel in linked-list mode */
  if ((hdma[CONFIG_Q_IDX].State == HAL_DMA_STATE_BUSY) && (hdma[CONFIG_Q_IDX].LinkedListQueue->FirstCircularNode != 0U))
  {
    if (HAL_DMA_Abort(&hdma[CONFIG_Q_IDX]) != HAL_OK)
    {
      Error_Handler();
    }
  }

  /* Check if DMA channel interrupt is enabled */
  if ((hdma[CONFIG_Q_IDX].State == HAL_DMA_STATE_BUSY) && (__HAL_DMA_GET_IT_SOURCE(&hdma[CONFIG_Q_IDX], DMA_IT_TC) == 0U))
  {
    if (HAL_DMA_PollForTransfer(&hdma[CONFIG_Q_IDX], HAL_DMA_FULL_TRANSFER, DMA_TIMEOUT_DURATION) != HAL_OK)
    {
      Error_Handler();
    }
  }

  /* USER CODE BEGIN DA_device_Stop */

  /* USER CODE END DA_device_Stop */
}

/**
  * @brief DA application device autonomous mode init
  * @param None
  * @retval None
  */
static void MX_AutonomousMode_Init(void)
{
  /* Enable LPDMA1 Sleep Clock */
  __HAL_RCC_LPDMA1_CLK_SLEEP_ENABLE();
  /* Enable LPDMA1 Autonomous Mode */
  __HAL_RCC_LPDMA1_CLKAM_ENABLE();

  /* Enable SRAM4 Sleep Clock */
  __HAL_RCC_SRAM4_CLK_SLEEP_ENABLE();
  /* Enable SRAM4 Autonomous Mode */
  __HAL_RCC_SRAM4_CLKAM_ENABLE();

  /* Enable I2C3 Sleep Clock */
  __HAL_RCC_I2C3_CLK_SLEEP_ENABLE();
  /* Enable I2C3 Autonomous Mode */
  __HAL_RCC_I2C3_CLKAM_ENABLE();

  /* Enable LPTIM1 Sleep Clock */
  __HAL_RCC_LPTIM1_CLK_SLEEP_ENABLE();
  /* Enable LPTIM1 Autonomous Mode */
  __HAL_RCC_LPTIM1_CLKAM_ENABLE();

  /* USER CODE BEGIN AutonomousMode_Init */
  //启动GPIOB
  __HAL_RCC_GPIOB_CLK_SLEEP_ENABLE();
  __HAL_RCC_SYSCFG_CLK_SLEEP_ENABLE();
  /* USER CODE END AutonomousMode_Init */
}

/**
  * @brief  DA application device autonomous mode deinit
  * @param  None
  * @retval None
  */
static void MX_AutonomousMode_DeInit(void)
{
  /* Disable LPDMA1 Sleep Clock */
  __HAL_RCC_LPDMA1_CLK_SLEEP_DISABLE();
  /* Disable LPDMA1 Autonomous Mode */
  __HAL_RCC_LPDMA1_CLKAM_DISABLE();

  /* Disable SRAM4 Sleep Clock */
  __HAL_RCC_SRAM4_CLK_SLEEP_DISABLE();
  /* Disable SRAM4 Autonomous Mode */
  __HAL_RCC_SRAM4_CLKAM_DISABLE();

  /* Disable I2C3 Sleep Clock */
  __HAL_RCC_I2C3_CLK_SLEEP_DISABLE();
  /* Disable I2C3 Autonomous Mode */
  __HAL_RCC_I2C3_CLKAM_DISABLE();

  /* Disable LPTIM1 Sleep Clock */
  __HAL_RCC_LPTIM1_CLK_SLEEP_DISABLE();
  /* Disable LPTIM1 Autonomous Mode */
  __HAL_RCC_LPTIM1_CLKAM_DISABLE();

  /* USER CODE BEGIN AutonomousMode_DeInit */
  
  /* USER CODE END AutonomousMode_DeInit */
}

/**
  * @brief I2C3 initialization.
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{
  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */

  /* Set I2C3 instance */
  hi2c3.Instance = I2C3;

  /* Register I2C msp callbacks */
  if (HAL_I2C_RegisterCallback(&hi2c3, HAL_I2C_MSPINIT_CB_ID, MX_I2C3_MspInit) != HAL_OK)
  {
    Error_Handler();
  }
  /* Register I2C msp callbacks */
  if (HAL_I2C_RegisterCallback(&hi2c3, HAL_I2C_MSPDEINIT_CB_ID, MX_I2C3_MspDeInit) != HAL_OK)
  {
    Error_Handler();
  }
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x00100D14;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configuration in LPBAM context
  */
  if (HAL_I2CEx_EnableWakeUp(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  if (ADV_LPBAM_I2C_EnableDMARequests(I2C3) != LPBAM_OK)
  {
    Error_Handler();
  }
  /** Register ISR
  */
  if (ADV_LPBAM_I2C_RegisterISR(&hi2c3) != LPBAM_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief I2C3 de-initialization.
  * @param None
  * @retval None
  */
static void MX_I2C3_DeInit(void)
{
  /* USER CODE BEGIN I2C3_DeInit 0 */

  /* USER CODE END I2C3_DeInit 0 */

  /* Set I2C3 instance */
  hi2c3.Instance = I2C3;

  /* UnRegister I2C msp callbacks */
  if (HAL_I2C_UnRegisterCallback(&hi2c3, HAL_I2C_MSPINIT_CB_ID) != HAL_OK)
  {
    Error_Handler();
  }
  /* Init I2C3 peripheral */
  if (HAL_I2C_DeInit(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  /* UnRegister I2C msp callbacks */
  if (HAL_I2C_UnRegisterCallback(&hi2c3, HAL_I2C_MSPDEINIT_CB_ID) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN I2C3_DeInit 1 */

  /* USER CODE END I2C3_DeInit 1 */
}

/**
  * @brief I2C3 MSP initialization.
  * @retval None
  */
static void MX_I2C3_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(i2cHandle->Instance==I2C3)
  {
  /* USER CODE BEGIN I2C3_MspInit 0 */

  /* USER CODE END I2C3_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C3;
    PeriphClkInit.I2c3ClockSelection = RCC_I2C3CLKSOURCE_MSIK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**I2C3 GPIO Configuration
    PC0     ------> I2C3_SCL
    PC1     ------> I2C3_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* I2C3 clock enable */
    __HAL_RCC_I2C3_CLK_ENABLE();

    /* I2C3 interrupt Init */
    HAL_NVIC_SetPriority(I2C3_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C3_EV_IRQn);
    HAL_NVIC_SetPriority(I2C3_ER_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C3_ER_IRQn);
  /* USER CODE BEGIN I2C3_MspInit 1 */

  /* USER CODE END I2C3_MspInit 1 */
  }
}

/**
  * @brief I2C3 MSP de-initialization.
  * @retval None
  */
static void MX_I2C3_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if (i2cHandle->Instance==I2C3)
  {
  /* USER CODE BEGIN I2C3_MspDeInit 0 */

  /* USER CODE END I2C3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C3_CLK_DISABLE();

    /**I2C3 GPIO Configuration
    PC0     ------> I2C3_SCL
    PC1     ------> I2C3_SDA
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0);

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1);

    /* I2C3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(I2C3_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C3_ER_IRQn);
  /* USER CODE BEGIN I2C3_MspDeInit 1 */

  /* USER CODE END I2C3_MspDeInit 1 */
  }
}

/**
  * @brief LPTIM1 initialization.
  * @param None
  * @retval None
  */
static void MX_LPTIM1_Init(void)
{
  /* USER CODE BEGIN LPTIM1_Init 0 */

  /* USER CODE END LPTIM1_Init 0 */

  LPTIM_OC_ConfigTypeDef sConfig1 = {0};

  /* USER CODE BEGIN LPTIM1_Init 1 */

  /* USER CODE END LPTIM1_Init 1 */

  /* Set LPTIM1 instance */
  hlptim1.Instance = LPTIM1;

  /* Register LPTIM msp callbacks */
  if (HAL_LPTIM_RegisterCallback(&hlptim1, HAL_LPTIM_MSPINIT_CB_ID, MX_LPTIM1_MspInit) != HAL_OK)
  {
    Error_Handler();
  }
  /* Register LPTIM msp callbacks */
  if (HAL_LPTIM_RegisterCallback(&hlptim1, HAL_LPTIM_MSPDEINIT_CB_ID, MX_LPTIM1_MspDeInit) != HAL_OK)
  {
    Error_Handler();
  }
  hlptim1.Instance = LPTIM1;
  hlptim1.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
  hlptim1.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV1;
  hlptim1.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
  hlptim1.Init.Period = 319;
  hlptim1.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
  hlptim1.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
  hlptim1.Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
  hlptim1.Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;
  hlptim1.Init.RepetitionCounter = 0;
  if (HAL_LPTIM_Init(&hlptim1) != HAL_OK)
  {
    Error_Handler();
  }
  sConfig1.Pulse = 159;
  sConfig1.OCPolarity = LPTIM_OCPOLARITY_HIGH;
  if (HAL_LPTIM_OC_ConfigChannel(&hlptim1, &sConfig1, LPTIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPTIM1_Init 2 */

  /* USER CODE END LPTIM1_Init 2 */

}

/**
  * @brief LPTIM1 de-initialization.
  * @param None
  * @retval None
  */
static void MX_LPTIM1_DeInit(void)
{
  /* USER CODE BEGIN LPTIM1_DeInit 0 */

  /* USER CODE END LPTIM1_DeInit 0 */

  /* Set LPTIM1 instance */
  hlptim1.Instance = LPTIM1;

  /* UnRegister LPTIM msp callbacks */
  if (HAL_LPTIM_UnRegisterCallback(&hlptim1, HAL_LPTIM_MSPINIT_CB_ID) != HAL_OK)
  {
    Error_Handler();
  }
  /* Init LPTIM1 peripheral */
  if (HAL_LPTIM_DeInit(&hlptim1) != HAL_OK)
  {
    Error_Handler();
  }
  /* UnRegister LPTIM msp callbacks */
  if (HAL_LPTIM_UnRegisterCallback(&hlptim1, HAL_LPTIM_MSPDEINIT_CB_ID) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN LPTIM1_DeInit 1 */

  /* USER CODE END LPTIM1_DeInit 1 */
}

/**
  * @brief LPTIM1 MSP initialization.
  * @retval None
  */
static void MX_LPTIM1_MspInit(LPTIM_HandleTypeDef* lptimHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(lptimHandle->Instance==LPTIM1)
  {
  /* USER CODE BEGIN LPTIM1_MspInit 0 */

  /* USER CODE END LPTIM1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
    PeriphClkInit.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* LPTIM1 clock enable */
    __HAL_RCC_LPTIM1_CLK_ENABLE();
  /* USER CODE BEGIN LPTIM1_MspInit 1 */

  /* USER CODE END LPTIM1_MspInit 1 */
  }
}

/**
  * @brief LPTIM1 MSP de-initialization.
  * @retval None
  */
static void MX_LPTIM1_MspDeInit(LPTIM_HandleTypeDef* lptimHandle)
{

  if (lptimHandle->Instance==LPTIM1)
  {
  /* USER CODE BEGIN LPTIM1_MspDeInit 0 */

  /* USER CODE END LPTIM1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_LPTIM1_CLK_DISABLE();
  /* USER CODE BEGIN LPTIM1_MspDeInit 1 */

  /* USER CODE END LPTIM1_MspDeInit 1 */
  }
}

/**
  * @brief  config queue link
  * @retval None
  */
static void MX_config_Q_Link(DMA_HandleTypeDef *hdma)
{
  /* Enable LPDMA1 clock */
  __HAL_RCC_LPDMA1_CLK_ENABLE();

  hdma->InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
  hdma->InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
  hdma->InitLinkedList.TransferEventMode = DMA_TCEM_LAST_LL_ITEM_TRANSFER;
  hdma->InitLinkedList.LinkedListMode = DMA_LINKEDLIST_NORMAL;
  if (HAL_DMAEx_List_Init(hdma) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMAEx_List_LinkQ(hdma, &config_Q) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_DMA_ENABLE_IT(hdma, DMA_IT_DTE|DMA_IT_ULE
                              |DMA_IT_USE|DMA_IT_TC);

  /* Register DMA channel error callbacks */
  if (HAL_DMA_RegisterCallback(hdma, HAL_DMA_XFER_ERROR_CB_ID, MX_config_Q_DMA_Error_Callback) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA_RegisterCallback(hdma, HAL_DMA_XFER_CPLT_CB_ID, MX_config_Q_DMA_TC_Callback) != HAL_OK)
  {
    Error_Handler();
  }
  MX_DMA_NVIC_Config(hdma, 0, 0);
}

/**
  * @brief  config queue unlink
  * @retval None
  */
static void MX_config_Q_UnLink(DMA_HandleTypeDef *hdma)
{
  /* UnLink config queue to DMA channel */
  if (HAL_DMAEx_List_UnLinkQ(hdma) != HAL_OK)
  {
    Error_Handler();
  }

  /* UnRegister DMA channel error callbacks */
  if (HAL_DMA_UnRegisterCallback(hdma, HAL_DMA_XFER_ERROR_CB_ID) != HAL_OK)
  {
    Error_Handler();
  }

  /* Register DMA channel transfer complete callbacks */
  if (HAL_DMA_UnRegisterCallback(hdma, HAL_DMA_XFER_CPLT_CB_ID) != HAL_OK)
  {
    Error_Handler();
  }

  /* DMA linked list de-init */
  if (HAL_DMAEx_List_DeInit(hdma) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  config queue dma error callback
  * @retval None
  */
static void MX_config_Q_DMA_Error_Callback(DMA_HandleTypeDef *hdma)
{
  /* USER CODE BEGIN config_DMA_Error_Callback */

  /* USER CODE END config_DMA_Error_Callback */
}

/**
  * @brief  config queue dma transfer complete callbacks
  * @retval None
  */
static void MX_config_Q_DMA_TC_Callback(DMA_HandleTypeDef *hdma)
{
  /* USER CODE BEGIN config_DMA_TC_Callback */

  /* USER CODE END config_DMA_TC_Callback */
}

/* USER CODE BEGIN DA_device_Config */

/* USER CODE END DA_device_Config */

/**
  * @brief DMA channel NVIC configuration
  * @retval None
  */
static void MX_DMA_NVIC_Config(DMA_HandleTypeDef *hdma, uint32_t PreemptPriority, uint32_t SubPriority)
{
  IRQn_Type irq = LPDMA1_Channel0_IRQn;

  /* Check DMA channel instance */
  switch ((uint32_t)hdma->Instance)
  {
    case (uint32_t)LPDMA1_Channel0: /* DMA channel_0 */
    {
      irq = LPDMA1_Channel0_IRQn;
      break;
    }

    case (uint32_t)LPDMA1_Channel1: /* DMA channel_1 */
    {
      irq = LPDMA1_Channel1_IRQn;
      break;
    }

    case (uint32_t)LPDMA1_Channel2: /* DMA channel_2 */
    {
      irq = LPDMA1_Channel2_IRQn;
      break;
    }

    case (uint32_t)LPDMA1_Channel3: /* DMA channel_3 */
    {
      irq = LPDMA1_Channel3_IRQn;
      break;
    }
  }

  /* Enable NVIC for DMA channel */
  HAL_NVIC_SetPriority(irq, PreemptPriority, SubPriority);
  HAL_NVIC_EnableIRQ(irq);
}
