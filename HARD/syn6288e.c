/**
  ******************************************************************************
  * @file    syn6288e.c
  * @brief   SYN6288E driver (aligned with vendor SYN_FrameInfo protocol)
  ******************************************************************************
  */
#include "syn6288e.h"
#include "usart.h"
#include <string.h>

#define SYN6288E_UART_TIMEOUT_MS      200U
#define SYN6288E_BOOT_DELAY_MS        500U
#define SYN6288E_STATUS_TIMEOUT_MS    500U
#define SYN6288E_IDLE_TIMEOUT_MS      5000U
#define SYN6288E_SPEECH_MIN_MS          300U

static void syn6288e_clear_uart_errors(UART_HandleTypeDef *huart)
{
  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF);
  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_NEF);
  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_FEF);
  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_PEF);
}

static void syn6288e_flush_rx(UART_HandleTypeDef *huart)
{
  uint8_t dummy;

  syn6288e_clear_uart_errors(huart);
  while (HAL_UART_Receive(huart, &dummy, 1U, 1U) == HAL_OK)
  {
  }
  syn6288e_clear_uart_errors(huart);
}

static uint8_t syn6288e_read_status(UART_HandleTypeDef *huart,
                                    uint8_t *status,
                                    uint32_t timeout_ms)
{
  if (status == NULL)
  {
    return 0U;
  }

  syn6288e_clear_uart_errors(huart);
  return (HAL_UART_Receive(huart, status, 1U, timeout_ms) == HAL_OK) ? 1U : 0U;
}

static uint8_t syn6288e_calc_ecc(const uint8_t *data, uint16_t len)
{
  uint8_t ecc = 0U;
  uint16_t i;

  for (i = 0U; i < len; i++)
  {
    ecc ^= data[i];
  }
  return ecc;
}

/*
 * Vendor frame (SYN_FrameInfo):
 *   FD + lenH + lenL + 0x01 + (0x01|music<<4) + text + XOR
 *   len field = text_len + 3  (cmd + param + text + checksum)
 */
static HAL_StatusTypeDef syn6288e_send_speech_frame(SYN6288E_Handle_t *dev,
                                                    uint8_t music,
                                                    const uint8_t *text,
                                                    uint16_t text_len)
{
  uint8_t frame[SYN6288E_TX_BUF_SIZE];
  uint16_t frame_len;
  uint8_t ecc;
  HAL_StatusTypeDef st;

  if ((dev == NULL) || (dev->huart == NULL) || (text == NULL) || (text_len == 0U))
  {
    return HAL_ERROR;
  }
  if ((uint16_t)(text_len + 6U) > sizeof(frame))
  {
    return HAL_ERROR;
  }

  USART3_PauseRxIT();
  syn6288e_flush_rx(dev->huart);

  frame[0] = SYN6288E_FRAME_HEAD;
  frame[1] = 0x00U;
  frame[2] = (uint8_t)(text_len + 3U);
  frame[3] = SYN6288E_CMD_TTS;
  frame[4] = (uint8_t)(0x01U | ((uint16_t)music << 4));
  memcpy(&frame[5], text, text_len);
  ecc = syn6288e_calc_ecc(frame, (uint16_t)(5U + text_len));
  frame[5U + text_len] = ecc;
  frame_len = (uint16_t)(6U + text_len);

  st = HAL_UART_Transmit(dev->huart, frame, frame_len, SYN6288E_UART_TIMEOUT_MS);
  USART3_ResumeRxIT();
  return st;
}

static HAL_StatusTypeDef syn6288e_send_cmd_frame(SYN6288E_Handle_t *dev, uint8_t cmd)
{
  uint8_t frame[5];
  HAL_StatusTypeDef st;

  if ((dev == NULL) || (dev->huart == NULL))
  {
    return HAL_ERROR;
  }

  USART3_PauseRxIT();
  syn6288e_flush_rx(dev->huart);

  frame[0] = SYN6288E_FRAME_HEAD;
  frame[1] = 0x00U;
  frame[2] = 0x02U;
  frame[3] = cmd;
  frame[4] = syn6288e_calc_ecc(frame, 4U);

  st = HAL_UART_Transmit(dev->huart, frame, sizeof(frame), SYN6288E_UART_TIMEOUT_MS);
  USART3_ResumeRxIT();
  return st;
}

static void syn6288e_track_status(SYN6288E_Handle_t *dev, uint8_t status)
{
  dev->last_status = status;
  if (status == SYN6288E_ACK_BUSY)
  {
    dev->speech_seen_busy = true;
  }
}

static bool syn6288e_is_playback_done(const SYN6288E_Handle_t *dev, uint8_t status)
{
  if (status != SYN6288E_ACK_IDLE)
  {
    return false;
  }

  /* begin_speech 后 last_status 已是 ACK/BUSY；此处 IDLE 且过了最短保护时间即播完 */
  return ((HAL_GetTick() - dev->speech_start_tick) >= SYN6288E_SPEECH_MIN_MS);
}

static void syn6288e_begin_speech(SYN6288E_Handle_t *dev, uint8_t ack_status)
{
  dev->speech_active = true;
  dev->speech_seen_busy = (ack_status == SYN6288E_ACK_BUSY);
  dev->speech_start_tick = HAL_GetTick();
}

static void syn6288e_end_speech(SYN6288E_Handle_t *dev)
{
  dev->speech_active = false;
  dev->speech_seen_busy = false;
}

static bool syn6288e_wait_speech_done(SYN6288E_Handle_t *dev)
{
  uint8_t status;
  uint32_t start = HAL_GetTick();
  uint32_t busy_start = 0U;

  if ((dev == NULL) || (dev->huart == NULL))
  {
    return false;
  }

  while ((HAL_GetTick() - start) < SYN6288E_IDLE_TIMEOUT_MS)
  {
    SYN6288E_Process(dev);
    if (syn6288e_is_playback_done(dev, dev->last_status))
    {
      return true;
    }

    if (!syn6288e_read_status(dev->huart, &status, 100U))
    {
      continue;
    }

    syn6288e_track_status(dev, status);
    if (syn6288e_is_playback_done(dev, status))
    {
      return true;
    }

    if (status == SYN6288E_ACK_BUSY)
    {
      if (busy_start == 0U)
      {
        busy_start = HAL_GetTick();
      }
      else if ((HAL_GetTick() - busy_start) > 3000U)
      {
        return false;
      }
    }
    else
    {
      busy_start = 0U;
    }
  }

  return false;
}

HAL_StatusTypeDef SYN6288E_Init(SYN6288E_Handle_t *dev, UART_HandleTypeDef *huart)
{
  uint8_t status;

  if ((dev == NULL) || (huart == NULL))
  {
    return HAL_ERROR;
  }

  memset(dev, 0, sizeof(*dev));
  dev->huart = huart;
  dev->wait_idle = true;
  dev->frame_gap_ms = 10U;
  dev->last_status = SYN6288E_ACK_IDLE;
  dev->speech_active = false;

  HAL_Delay(SYN6288E_BOOT_DELAY_MS);
  syn6288e_flush_rx(huart);

  if (syn6288e_read_status(huart, &status, 100U))
  {
    dev->last_status = status;
  }

  return HAL_OK;
}

void SYN6288E_RxByte(SYN6288E_Handle_t *dev, uint8_t byte)
{
  uint16_t next;

  if (dev == NULL)
  {
    return;
  }

  next = (uint16_t)((dev->rx_head + 1U) % SYN6288E_RX_BUF_SIZE);
  if (next != dev->rx_tail)
  {
    dev->rx_buf[dev->rx_head] = byte;
    dev->rx_head = next;
    dev->last_status = byte;
  }
}

bool SYN6288E_IsIdle(SYN6288E_Handle_t *dev)
{
  if (dev == NULL)
  {
    return false;
  }
  return (dev->last_status == SYN6288E_ACK_IDLE);
}

bool SYN6288E_IsSpeechDone(SYN6288E_Handle_t *dev)
{
  uint8_t status;

  if ((dev == NULL) || !dev->speech_active)
  {
    return false;
  }

  /* 刚下发的前几百毫秒不做结束判断，避免残留状态误判 */
  if ((HAL_GetTick() - dev->speech_start_tick) < SYN6288E_SPEECH_MIN_MS)
  {
    SYN6288E_Process(dev);
    return false;
  }

  SYN6288E_Process(dev);
  if (dev->last_status == SYN6288E_ACK_BUSY)
  {
    dev->speech_seen_busy = true;
  }
  if (syn6288e_is_playback_done(dev, dev->last_status))
  {
    syn6288e_end_speech(dev);
    return true;
  }

  /* 主动查状态：播放中为 BUSY，播完为 IDLE */
  if (SYN6288E_QueryStatus(dev) == HAL_OK)
  {
    status = dev->last_status;
    if (status == SYN6288E_ACK_BUSY)
    {
      dev->speech_seen_busy = true;
    }
    if (syn6288e_is_playback_done(dev, status))
    {
      syn6288e_end_speech(dev);
      return true;
    }
  }

  return false;
}

bool SYN6288E_WaitIdle(SYN6288E_Handle_t *dev, uint32_t timeout_ms)
{
  uint32_t start = HAL_GetTick();
  uint8_t status;

  if (dev == NULL)
  {
    return false;
  }

  while ((HAL_GetTick() - start) < timeout_ms)
  {
    SYN6288E_Process(dev);
    if (SYN6288E_IsIdle(dev))
    {
      return true;
    }

    if (syn6288e_read_status(dev->huart, &status, 50U))
    {
      dev->last_status = status;
      if (status == SYN6288E_ACK_IDLE)
      {
        return true;
      }
    }

    HAL_Delay(2U);
  }

  return false;
}

void SYN6288E_Process(SYN6288E_Handle_t *dev)
{
  if (dev == NULL)
  {
    return;
  }

  while (dev->rx_tail != dev->rx_head)
  {
    syn6288e_track_status(dev, dev->rx_buf[dev->rx_tail]);
    dev->rx_tail = (uint16_t)((dev->rx_tail + 1U) % SYN6288E_RX_BUF_SIZE);
  }
}

HAL_StatusTypeDef SYN6288E_PlayText(SYN6288E_Handle_t *dev,
                                    const uint8_t *text,
                                    uint16_t len,
                                    SYN6288E_Encoding_t enc)
{
  uint8_t status;
  HAL_StatusTypeDef st;

  (void)enc;

  if ((dev == NULL) || (text == NULL) || (len == 0U) || (len > SYN6288E_MAX_TEXT_LEN))
  {
    return HAL_ERROR;
  }

  if (dev->wait_idle)
  {
    (void)SYN6288E_WaitIdle(dev, SYN6288E_IDLE_TIMEOUT_MS);
    HAL_Delay(dev->frame_gap_ms);
  }

  st = syn6288e_send_speech_frame(dev, 0U, text, len);
  if (st != HAL_OK)
  {
    return st;
  }

  if (!syn6288e_read_status(dev->huart, &status, SYN6288E_STATUS_TIMEOUT_MS))
  {
    return HAL_ERROR;
  }

  syn6288e_track_status(dev, status);
  if (status == SYN6288E_ACK_FAIL)
  {
    return HAL_ERROR;
  }

  if (dev->wait_idle)
  {
    syn6288e_begin_speech(dev, status);
    (void)syn6288e_wait_speech_done(dev);
    syn6288e_end_speech(dev);
  }
  else
  {
    syn6288e_begin_speech(dev, status);
  }

  return HAL_OK;
}

HAL_StatusTypeDef SYN6288E_PlayTextGBK(SYN6288E_Handle_t *dev, const char *gbk_text)
{
  if ((dev == NULL) || (gbk_text == NULL))
  {
    return HAL_ERROR;
  }

  return SYN6288E_PlayText(dev, (const uint8_t *)gbk_text, (uint16_t)strlen(gbk_text),
                           SYN6288E_ENC_GBK);
}

HAL_StatusTypeDef SYN6288E_Stop(SYN6288E_Handle_t *dev)
{
  if (dev == NULL)
  {
    return HAL_ERROR;
  }

  return syn6288e_send_cmd_frame(dev, SYN6288E_CMD_STOP);
}

HAL_StatusTypeDef SYN6288E_Pause(SYN6288E_Handle_t *dev)
{
  if (dev == NULL)
  {
    return HAL_ERROR;
  }

  return syn6288e_send_cmd_frame(dev, SYN6288E_CMD_PAUSE);
}

HAL_StatusTypeDef SYN6288E_Resume(SYN6288E_Handle_t *dev)
{
  if (dev == NULL)
  {
    return HAL_ERROR;
  }

  return syn6288e_send_cmd_frame(dev, SYN6288E_CMD_RESUME);
}

HAL_StatusTypeDef SYN6288E_QueryStatus(SYN6288E_Handle_t *dev)
{
  uint8_t status;
  HAL_StatusTypeDef st;

  if (dev == NULL)
  {
    return HAL_ERROR;
  }

  st = syn6288e_send_cmd_frame(dev, SYN6288E_CMD_STATUS);
  if (st != HAL_OK)
  {
    return st;
  }

  if (!syn6288e_read_status(dev->huart, &status, SYN6288E_STATUS_TIMEOUT_MS))
  {
    return HAL_ERROR;
  }

  dev->last_status = status;
  return HAL_OK;
}

HAL_StatusTypeDef SYN6288E_SetVolume(SYN6288E_Handle_t *dev, uint8_t level)
{
  char tag[8];
  uint8_t i = 0U;
  uint8_t n;

  if ((dev == NULL) || (level > 16U))
  {
    return HAL_ERROR;
  }

  tag[i++] = '[';
  tag[i++] = 'v';
  if (level >= 10U)
  {
    tag[i++] = (char)('0' + (level / 10U));
    n = (uint8_t)(level % 10U);
  }
  else
  {
    n = level;
  }
  tag[i++] = (char)('0' + n);
  tag[i++] = ']';
  tag[i] = '\0';

  return SYN6288E_PlayTextGBK(dev, tag);
}

void SYN6288E_OnUartReady(SYN6288E_Handle_t *dev)
{
  if ((dev == NULL) || (dev->huart == NULL))
  {
    return;
  }

  syn6288e_flush_rx(dev->huart);
}
