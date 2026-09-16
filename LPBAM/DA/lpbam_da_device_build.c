/* USER CODE BEGIN Header */
/**
  **********************************************************************************************************************
  * @file   lpbam_da_device_build.c
  * @author MCD Application Team
  * @brief  Provides LPBAM DA application device scenario build services
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

/* Private variables -------------------------------------------------------------------------------------------------*/
/* LPBAM variables declaration */
/* USER CODE BEGIN DA_device_Descs 0 */
extern uint8_t receive_data[12];
extern uint8_t REG[1];
/* USER CODE END DA_device_Descs 0 */

/* USER CODE BEGIN config_Q_Master_Transmit_Data_7_Desc[1] */

/* USER CODE END config_Q_Master_Transmit_Data_7_Desc[1] */
static LPBAM_I2C_MasterTxDataDesc_t config_Q_Master_Transmit_Data_7_Desc[1];

/* USER CODE BEGIN config_Q_Master_Receive_Data_1_Desc[1] */

/* USER CODE END config_Q_Master_Receive_Data_1_Desc[1] */
static LPBAM_I2C_MasterRxDataDesc_t config_Q_Master_Receive_Data_1_Desc[1];

/* USER CODE BEGIN DA_device_Descs 1 */

/* USER CODE END DA_device_Descs 1 */

/* Exported variables ------------------------------------------------------------------------------------------------*/
/* LPBAM queues declaration */
DMA_QListTypeDef config_Q;

/* External variables ------------------------------------------------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private function prototypes ---------------------------------------------------------------------------------------*/
static void MX_config_Q_Build(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Exported functions ------------------------------------------------------------------------------------------------*/
/**
  * @brief DA application device scenario build
  * @param None
  * @retval None
  */
void MX_DA_device_Build(void)
{
  /* USER CODE BEGIN DA_device_Build 0 */

  /* USER CODE END DA_device_Build 0 */

  /* LPBAM build config queue */
  MX_config_Q_Build();

  /* USER CODE BEGIN DA_device_Build 1 */

  /* USER CODE END DA_device_Build 1 */
}

/* Private functions -------------------------------------------------------------------------------------------------*/

/**
  * @brief  DA application device scenario config queue build
  * @param  None
  * @retval None
  */
static void MX_config_Q_Build(void)
{
  /* LPBAM build variable */
  LPBAM_DMAListInfo_t pDMAListInfo_I2C = {0};
  LPBAM_I2C_DataAdvConf_t pTxData_I2C = {0};
  LPBAM_COMMON_DataAdvConf_t pDataConfig_I2C = {0};
  LPBAM_I2C_DataAdvConf_t pRxData_I2C = {0};
  LPBAM_COMMON_TrigAdvConf_t pTrigConfig_I2C = {0};
  uint32_t data_size = 0;
  uint32_t tmp_data_size = 0;
  uint32_t transfer_idx = 0;

  /**
    * config queue Master_Transmit_Data_7 build
    */
   pDMAListInfo_I2C.QueueType= LPBAM_LINEAR_ADDRESSING_Q;
   pDMAListInfo_I2C.pInstance= LPDMA1;
  pTxData_I2C.AutoModeConf.TriggerState = LPBAM_I2C_AUTO_MODE_DISABLE;
   pTxData_I2C.AddressingMode = LPBAM_I2C_ADDRESSINGMODE_7BIT;
   pTxData_I2C.SequenceNumber = 1;
   pTxData_I2C.pData = (uint8_t*)&REG[0];
   pTxData_I2C.DevAddress = 0xD4;
   pTxData_I2C.Size = 1;
  /* Set transfer parameters */
  data_size = pTxData_I2C.Size;
  tmp_data_size = pTxData_I2C.Size;
  transfer_idx = 0;

  /* Repeat inserting I2C master Tx data queue until completing all data */
  while (data_size != 0U)
  {
    if (ADV_LPBAM_I2C_MasterTx_SetDataQ(I2C3, &pDMAListInfo_I2C, &pTxData_I2C, &config_Q_Master_Transmit_Data_7_Desc[transfer_idx], &config_Q) != LPBAM_OK)
    {
      Error_Handler();
    }

    transfer_idx++;

    if (data_size > LPBAM_I2C_MAX_DATA_SIZE)
    {
      data_size -= LPBAM_I2C_MAX_DATA_SIZE;
    }
    else
    {
      data_size = 0U;
    }

    pTxData_I2C.Size = data_size;
  }
  pTxData_I2C.Size = tmp_data_size;
  pDataConfig_I2C.TransferConfig.Transfer.SrcInc = LPBAM_DMA_SINC_INCREMENTED;
  pDataConfig_I2C.TransferConfig.Transfer.DestInc = LPBAM_DMA_DINC_FIXED;
  pDataConfig_I2C.TransferConfig.Transfer.SrcDataWidth = LPBAM_DMA_SRC_DATAWIDTH_BYTE;
  pDataConfig_I2C.TransferConfig.Transfer.DestDataWidth = LPBAM_DMA_DEST_DATAWIDTH_BYTE;
  pDataConfig_I2C.TransferConfig.Transfer.TransferEventMode = LPBAM_DMA_TCEM_EACH_LL_ITEM_TRANSFER;
   pDataConfig_I2C.UpdateSrcInc = DISABLE;
   pDataConfig_I2C.UpdateDestInc = DISABLE;
   pDataConfig_I2C.UpdateSrcDataWidth = DISABLE;
   pDataConfig_I2C.UpdateDestDataWidth = DISABLE;
   pDataConfig_I2C.UpdateTransferEventMode = ENABLE;
  if (ADV_LPBAM_Q_SetDataConfig (&pDataConfig_I2C, LPBAM_I2C_MASTERTX_DATAQ_DATA_NODE, &config_Q_Master_Transmit_Data_7_Desc) != LPBAM_OK)
  {
    Error_Handler();
  }

  /**
    * config queue Master_Receive_Data_1 build
    */
  pRxData_I2C.AutoModeConf.TriggerState = LPBAM_I2C_AUTO_MODE_DISABLE;
   pRxData_I2C.AddressingMode = LPBAM_I2C_ADDRESSINGMODE_7BIT;
   pRxData_I2C.SequenceNumber = 1;
   pRxData_I2C.pData = (uint8_t*)&receive_data[0];
   pRxData_I2C.DevAddress = 0xD4;
   pRxData_I2C.Size = 12;
  /* Set transfer parameters */
  data_size = pRxData_I2C.Size;
  tmp_data_size = pRxData_I2C.Size;
  transfer_idx = 0;

  /* Repeat inserting I2C master Tx data queue until completing all data */
  while (data_size != 0U)
  {
    if (ADV_LPBAM_I2C_MasterRx_SetDataQ(I2C3, &pDMAListInfo_I2C, &pRxData_I2C, &config_Q_Master_Receive_Data_1_Desc[transfer_idx], &config_Q) != LPBAM_OK)
    {
      Error_Handler();
    }

    transfer_idx++;

    if (data_size > LPBAM_I2C_MAX_DATA_SIZE)
    {
      data_size -= LPBAM_I2C_MAX_DATA_SIZE;
    }
    else
    {
      data_size = 0U;
    }

    pRxData_I2C.Size = data_size;
  }
  pRxData_I2C.Size = tmp_data_size;
  pTrigConfig_I2C.TriggerConfig.TriggerMode = LPBAM_DMA_TRIGM_BLOCK_TRANSFER;
  pTrigConfig_I2C.TriggerConfig.TriggerPolarity = LPBAM_DMA_TRIG_POLARITY_RISING;
  pTrigConfig_I2C.TriggerConfig.TriggerSelection = LPBAM_LPDMA1_TRIGGER_LPDMA1_CH0_TCF;
  if (ADV_LPBAM_Q_SetTriggerConfig (&pTrigConfig_I2C, LPBAM_I2C_MASTERRX_DATAQ_CONFIG_NODE, &config_Q_Master_Receive_Data_1_Desc) != LPBAM_OK)
  {
    Error_Handler();
  }
  pDataConfig_I2C.TransferConfig.Transfer.SrcInc = LPBAM_DMA_SINC_FIXED;
  pDataConfig_I2C.TransferConfig.Transfer.DestInc = LPBAM_DMA_DINC_INCREMENTED;
  if (ADV_LPBAM_Q_SetDataConfig (&pDataConfig_I2C, LPBAM_I2C_MASTERRX_DATAQ_DATA_NODE, &config_Q_Master_Receive_Data_1_Desc) != LPBAM_OK)
  {
    Error_Handler();
  }

  /**
    * Set circular mode
    */
  if (ADV_LPBAM_Q_SetCircularMode(&config_Q_Master_Transmit_Data_7_Desc, LPBAM_I2C_MASTERTX_DATAQ_CONFIG_NODE, &config_Q) != LPBAM_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN DA_device_Build */

/* USER CODE END DA_device_Build */
