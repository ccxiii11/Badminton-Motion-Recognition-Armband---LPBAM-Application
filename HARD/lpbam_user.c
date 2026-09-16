#include "lpbam_user.h"
#include "lpdma.h"
#include "lsm_reg.h"
#include "stm32_lpbam_lptim.h"
#include <string.h>
           /*定义I2C从机地址*/
#define LSM_I2C_ADDR  0xD4U

         /*引入定义好的I2C数据缓冲区*/
extern volatile uint8_t receive_data[IMU_FRAME_COUNT * IMU_FRAME_BYTES];
extern uint8_t REG[1];

extern LPTIM_HandleTypeDef hlptim1;//LPTIM句柄

       /*数据的存储结构必须放到sram4中*/
__attribute__((section(".lpbam_section"), aligned(4)))
static LPBAM_I2C_MasterTxDataDesc_t s_tx_desc[IMU_FRAME_COUNT];

__attribute__((section(".lpbam_section"), aligned(4)))
static LPBAM_I2C_MasterRxDataDesc_t s_rx_desc[IMU_FRAME_COUNT];

__attribute__((section(".lpbam_section"), aligned(4)))
DMA_QListTypeDef config_Q;
   /*定义LPBAM的链表节点个数*/
typedef struct
{
  DMA_NodeTypeDef pNodes[3U];
  uint32_t pReg[3U];
} lpbam_lptim_start_desc_t;
     /*放入sram4当中*/
__attribute__((section(".lpbam_section"), aligned(4)))
static lpbam_lptim_start_desc_t s_lptim_start;
           /*采样完成标志放 SRAM4，STOP2 期间 LPDMA 中断可安全写入*/
__attribute__((section(".lpbam_section"), aligned(4)))
static volatile uint8_t s_sample_done;
__attribute__((section(".lpbam_section"), aligned(4)))
static volatile uint8_t s_sample_error;
            /*根据设置的采样率配置ARR*/
static void LPBAM_User_SetLptimPeriod(void)
{
  __HAL_LPTIM_DISABLE(&hlptim1);
  __HAL_LPTIM_ENABLE(&hlptim1);
  __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_ARROK);
  __HAL_LPTIM_AUTORELOAD_SET(&hlptim1, LPBAM_LPTIM_PERIOD);
  {
    uint32_t tickstart = HAL_GetTick();
    while (__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_ARROK) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > 100U)
      {
        break;
      }
    }
  }
  __HAL_LPTIM_COMPARE_SET(&hlptim1, LPTIM_CHANNEL_1, LPBAM_LPTIM_PERIOD / 2U);
  __HAL_LPTIM_DISABLE(&hlptim1);
}
            /*配置触发模式有PWM输出10HZ频率的上升沿触发*/ 
static void LPBAM_User_PrepareLptimPwm(void)
{
  __HAL_LPTIM_DISABLE(&hlptim1);
  hlptim1.Instance->CFGR &= ~LPTIM_CFGR_WAVE;
  __HAL_LPTIM_CAPTURE_COMPARE_ENABLE(&hlptim1, LPTIM_CHANNEL_1);
}

/* PB4 上升沿 → EXTI4 → LPDMA 写 LPTIM CR 启动计数 */
static void LPBAM_User_BuildLptimStartHead(LPBAM_COMMON_TrigAdvConf_t *trig_cfg)
{
  LPBAM_LPTIM_ConfNode_t config_node = {0};
  DMA_NodeConfTypeDef dma_node_conf = {0};
  LPBAM_DMAListInfo_t dma_list = {0};
  uint32_t node_idx = 0U;
  uint32_t reg_idx = 0U;

  dma_list.QueueType = LPBAM_LINEAR_ADDRESSING_Q;
  dma_list.pInstance = LPDMA1;

  config_node.pInstance = LPTIM1;
  config_node.NodeDesc.NodeInfo.NodeType = LPBAM_LINEAR_ADDRESSING_Q;

  config_node.NodeDesc.NodeInfo.NodeID = (uint32_t)LPBAM_LPTIM_CONFIG_ID;
  config_node.NodeDesc.pSrcVarReg = &s_lptim_start.pReg[reg_idx];
  config_node.Config.State = ENABLE;
  config_node.Config.UpdateStartMode = DISABLE;
  if (LPBAM_LPTIM_FillNodeConfig(&config_node, &dma_node_conf) != LPBAM_OK)
  {
    Error_Handler();
  }
  if (HAL_DMAEx_List_BuildNode(&dma_node_conf, &s_lptim_start.pNodes[node_idx]) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMAEx_List_InsertNode_Tail(&config_Q, &s_lptim_start.pNodes[node_idx]) != HAL_OK)
  {
    Error_Handler();
  }
  node_idx++;
  reg_idx++;

  config_node.NodeDesc.NodeInfo.NodeID = (uint32_t)LPBAM_LPTIM_WAKEUP_IT_ID;
  config_node.NodeDesc.pSrcVarReg = &s_lptim_start.pReg[reg_idx];
  config_node.Config.WakeupIT = LPBAM_LPTIM_IT_NONE;
  if (LPBAM_LPTIM_FillNodeConfig(&config_node, &dma_node_conf) != LPBAM_OK)
  {
    Error_Handler();
  }
  if (HAL_DMAEx_List_BuildNode(&dma_node_conf, &s_lptim_start.pNodes[node_idx]) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMAEx_List_InsertNode_Tail(&config_Q, &s_lptim_start.pNodes[node_idx]) != HAL_OK)
  {
    Error_Handler();
  }
  node_idx++;
  reg_idx++;

  config_node.NodeDesc.NodeInfo.NodeID = (uint32_t)LPBAM_LPTIM_CONFIG_ID;
  config_node.NodeDesc.pSrcVarReg = &s_lptim_start.pReg[reg_idx];
  config_node.Config.State = ENABLE;
  config_node.Config.UpdateStartMode = ENABLE;
  config_node.Config.StartMode = LPBAM_LPTIM_START_CONTINUOUS;
  if (LPBAM_LPTIM_FillNodeConfig(&config_node, &dma_node_conf) != LPBAM_OK)
  {
    Error_Handler();
  }
  if (HAL_DMAEx_List_BuildNode(&dma_node_conf, &s_lptim_start.pNodes[node_idx]) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMAEx_List_InsertNode_Tail(&config_Q, &s_lptim_start.pNodes[node_idx]) != HAL_OK)
  {
    Error_Handler();
  }

  trig_cfg->TriggerConfig.TriggerMode = LPBAM_DMA_TRIGM_BLOCK_TRANSFER;
  trig_cfg->TriggerConfig.TriggerPolarity = LPBAM_DMA_TRIG_POLARITY_RISING;
  trig_cfg->TriggerConfig.TriggerSelection = LPBAM_LPDMA1_TRIGGER_EXTI_LINE4;
  if (ADV_LPBAM_Q_SetTriggerConfig(trig_cfg, LPBAM_LPTIM_START_FULLQ_CONFIG_NODE,
                                   &s_lptim_start) != LPBAM_OK)
  {
    Error_Handler();
  }
}
      /* DMA的完成回调函数*/
static void LPBAM_User_DmaTcCallback(DMA_HandleTypeDef *hdma)
{
  if ((hdma->Instance->CLLR == 0U) && (hdma->Instance->CBR1 == 0U))
  {
    s_sample_done = 1U;
  }
}
/* DMA的错误回调函数*/
static void LPBAM_User_DmaErrorCallback(DMA_HandleTypeDef *hdma)
{
  (void)hdma;
  s_sample_error = 1U;
  s_sample_done = 1U;
}
/* NORMAL 链表跑完后 TC 中断被关，HAL 不会把 State 置 READY，需手动复位才能再次 Start */
static void LPBAM_User_ResetDmaChannel(void)
{
  DMA_HandleTypeDef *const hdma = &handle_LPDMA1_Channel0;
  uint32_t tickstart;
  if (hdma->State == HAL_DMA_STATE_READY)
  {
    return;
  }

  __HAL_DMA_DISABLE(hdma);
  tickstart = HAL_GetTick();
  while ((hdma->Instance->CCR & DMA_CCR_EN) != 0U)
  {
    if ((HAL_GetTick() - tickstart) > 100U)
    {
      break;
    }
  }

  __HAL_DMA_CLEAR_FLAG(hdma,
                       DMA_FLAG_TC | DMA_FLAG_HT | DMA_FLAG_DTE | DMA_FLAG_ULE | DMA_FLAG_USE);
  hdma->ErrorCode = HAL_DMA_ERROR_NONE;
  hdma->State = HAL_DMA_STATE_READY;

  if (hdma->LinkedListQueue != NULL)
  {
    hdma->LinkedListQueue->State = HAL_DMA_QUEUE_STATE_READY;
    hdma->LinkedListQueue->ErrorCode = HAL_DMA_QUEUE_ERROR_NONE;
    hdma->Instance->CBR1 = 0U;
  }

  __HAL_UNLOCK(hdma);
}
/*计算内存中能存的最大数据帧数*/
uint32_t LPBAM_User_MaxBatchFrames(void)
{
  const uint32_t overhead = 128U;
  const uint32_t per_frame = IMU_FRAME_BYTES + (2U * 76U);
  uint32_t max_frames = (16384U - overhead) / per_frame;

  if (max_frames > IMU_FRAME_COUNT)
  {
    max_frames = IMU_FRAME_COUNT;
  }
  return max_frames;
}
      /*读取IMU数据*/
static void decode_imu_frame(const uint8_t *raw, imu_traj_frame_t *out)
{
  out->gx = (int16_t)((uint8_t)raw[0] | ((uint8_t)raw[1] << 8));
  out->gy = (int16_t)((uint8_t)raw[2] | ((uint8_t)raw[3] << 8));
  out->gz = (int16_t)((uint8_t)raw[4] | ((uint8_t)raw[5] << 8));
  out->ax = (int16_t)((uint8_t)raw[6] | ((uint8_t)raw[7] << 8));
  out->ay = (int16_t)((uint8_t)raw[8] | ((uint8_t)raw[9] << 8));
  out->az = (int16_t)((uint8_t)raw[10] | ((uint8_t)raw[11] << 8));
}

void LPBAM_User_LoadTraj(imu_traj_frame_t *dst, uint32_t frame_count)
{
  /*错误检测，判断是否超过最大量程 */
  if ((dst == NULL) || (frame_count == 0U) || (frame_count > IMU_FRAME_COUNT))
  {
    return;
  }
  
  for (uint32_t frame = 0U; frame < frame_count; frame++)
  {
    const uint8_t *raw = (const uint8_t *)&receive_data[frame * IMU_FRAME_BYTES];
    decode_imu_frame(raw, &dst[frame]);
  }
}
/*建立链表*/
void LPBAM_User_BuildQueue(void)
{
  LPBAM_DMAListInfo_t dma_list = {0};
  LPBAM_I2C_DataAdvConf_t tx_data = {0};
  LPBAM_COMMON_DataAdvConf_t data_cfg = {0};
  LPBAM_I2C_DataAdvConf_t rx_data = {0};
  LPBAM_COMMON_TrigAdvConf_t trig_cfg = {0};
  uint32_t data_size;
  uint32_t tmp_data_size;
  const uint32_t frame_count = LPBAM_User_MaxBatchFrames();

  (void)memset(&config_Q, 0, sizeof(config_Q));

  dma_list.QueueType = LPBAM_LINEAR_ADDRESSING_Q;
  dma_list.pInstance = LPDMA1;

  LPBAM_User_BuildLptimStartHead(&trig_cfg);

  for (uint32_t frame = 0U; frame < frame_count; frame++)
  {
    tx_data.AutoModeConf.TriggerState = LPBAM_I2C_AUTO_MODE_DISABLE;
    tx_data.AddressingMode = LPBAM_I2C_ADDRESSINGMODE_7BIT;
    tx_data.SequenceNumber = 1;
    tx_data.pData = (uint8_t *)&REG[0];
    tx_data.DevAddress = LSM_I2C_ADDR;
    tx_data.Size = 1;
    data_size = tx_data.Size;
    tmp_data_size = tx_data.Size;

    while (data_size != 0U)
    {
      if (ADV_LPBAM_I2C_MasterTx_SetDataQ(I2C3, &dma_list, &tx_data,
                                          &s_tx_desc[frame], &config_Q) != LPBAM_OK)
      {
        Error_Handler();
      }

      if (data_size > LPBAM_I2C_MAX_DATA_SIZE)
      {
        data_size -= LPBAM_I2C_MAX_DATA_SIZE;
      }
      else
      {
        data_size = 0U;
      }
      tx_data.Size = data_size;
    }
    tx_data.Size = tmp_data_size;

    data_cfg.TransferConfig.Transfer.SrcInc = LPBAM_DMA_SINC_INCREMENTED;
    data_cfg.TransferConfig.Transfer.DestInc = LPBAM_DMA_DINC_FIXED;
    data_cfg.TransferConfig.Transfer.SrcDataWidth = LPBAM_DMA_SRC_DATAWIDTH_BYTE;
    data_cfg.TransferConfig.Transfer.DestDataWidth = LPBAM_DMA_DEST_DATAWIDTH_BYTE;
    data_cfg.UpdateSrcInc = DISABLE;
    data_cfg.UpdateDestInc = DISABLE;
    data_cfg.UpdateSrcDataWidth = DISABLE;
    data_cfg.UpdateDestDataWidth = DISABLE;
    data_cfg.UpdateTransferEventMode = DISABLE;
    if (ADV_LPBAM_Q_SetDataConfig(&data_cfg, LPBAM_I2C_MASTERTX_DATAQ_DATA_NODE,
                                  &s_tx_desc[frame]) != LPBAM_OK)
    {
      Error_Handler();
    }

    /* 每帧由 LPTIM1 CH1 触发（LPTIM 已由 EXTI4 队列头节点启动） */
    trig_cfg.TriggerConfig.TriggerMode = LPBAM_DMA_TRIGM_BLOCK_TRANSFER;
    trig_cfg.TriggerConfig.TriggerPolarity = LPBAM_DMA_TRIG_POLARITY_RISING;
    trig_cfg.TriggerConfig.TriggerSelection = LPBAM_LPDMA1_TRIGGER_LPTIM1_CH1;
      if (ADV_LPBAM_Q_SetTriggerConfig(&trig_cfg, LPBAM_I2C_MASTERTX_DATAQ_CONFIG_NODE,
                                       &s_tx_desc[frame]) != LPBAM_OK)
      {
        Error_Handler();
      }

    rx_data.AutoModeConf.TriggerState = LPBAM_I2C_AUTO_MODE_DISABLE;
    rx_data.AddressingMode = LPBAM_I2C_ADDRESSINGMODE_7BIT;
    rx_data.SequenceNumber = 1;
    rx_data.pData = (uint8_t *)&receive_data[frame * IMU_FRAME_BYTES];
    rx_data.DevAddress = LSM_I2C_ADDR;
    rx_data.Size = IMU_FRAME_BYTES;
    data_size = rx_data.Size;
    tmp_data_size = rx_data.Size;

    while (data_size != 0U)
    {
      if (ADV_LPBAM_I2C_MasterRx_SetDataQ(I2C3, &dma_list, &rx_data,
                                          &s_rx_desc[frame], &config_Q) != LPBAM_OK)
      {
        Error_Handler();
      }

      if (data_size > LPBAM_I2C_MAX_DATA_SIZE)
      {
        data_size -= LPBAM_I2C_MAX_DATA_SIZE;
      }
      else
      {
        data_size = 0U;
      }
      rx_data.Size = data_size;
    }
    rx_data.Size = tmp_data_size;

    data_cfg.TransferConfig.Transfer.SrcInc = LPBAM_DMA_SINC_FIXED;
    data_cfg.TransferConfig.Transfer.DestInc = LPBAM_DMA_DINC_INCREMENTED;
    if (ADV_LPBAM_Q_SetDataConfig(&data_cfg, LPBAM_I2C_MASTERRX_DATAQ_DATA_NODE,
                                  &s_rx_desc[frame]) != LPBAM_OK)
    {
      Error_Handler();
    }
  }
}

void LPBAM_User_BeginSampling(void)
{
  s_sample_done = 0U;
  s_sample_error = 0U;
  LPBAM_User_ResetDmaChannel();
  if (HAL_DMA_RegisterCallback(&handle_LPDMA1_Channel0, HAL_DMA_XFER_CPLT_CB_ID,
                               LPBAM_User_DmaTcCallback) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA_RegisterCallback(&handle_LPDMA1_Channel0, HAL_DMA_XFER_ERROR_CB_ID,
                               LPBAM_User_DmaErrorCallback) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_DMA_ENABLE_IT(&handle_LPDMA1_Channel0,
                      DMA_IT_TC | DMA_IT_DTE | DMA_IT_ULE | DMA_IT_USE);
  HAL_NVIC_SetPriority(LPDMA1_Channel0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(LPDMA1_Channel0_IRQn);
  HAL_NVIC_DisableIRQ(I2C3_EV_IRQn);
  HAL_NVIC_DisableIRQ(I2C3_ER_IRQn);
  
  LPBAM_User_SetLptimPeriod();
  LPBAM_User_PrepareLptimPwm();

  if (HAL_DMAEx_List_Start(&handle_LPDMA1_Channel0) != HAL_OK)
  {
    Error_Handler();
  }
}
   /*停止定时器收集*/
void LPBAM_User_StopSampleTimer(void)
{
  (void)HAL_LPTIM_PWM_Stop(&hlptim1, LPTIM_CHANNEL_1);
  __HAL_LPTIM_DISABLE_IT(&hlptim1, LPTIM_IT_ARRM);
  HAL_NVIC_DisableIRQ(LPTIM1_IRQn);
  LPBAM_User_ResetDmaChannel();
}
   /*收集完成判断*/
uint8_t LPBAM_User_IsSampleDone(void)
{
  return s_sample_done;
}
     /*收集发生错误判断*/
uint8_t LPBAM_User_HasSampleError(void)
{
  return s_sample_error;
}
