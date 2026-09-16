/**
  ******************************************************************************
  * @file    score_voice.c
  * @brief   �������ģ��ʵ��
  ******************************************************************************
  */
#include "score_voice.h"
#include <stdio.h>
#include <string.h>

/* GBK �̶�����Ƭ�� */
static const uint8_t k_gbk_this_score[] = {0xB1, 0xBE, 0xB4, 0xCE, 0xB5, 0xC3, 0xB7, 0xD6}; /* ���ε÷� */
static const uint8_t k_gbk_point[]      = {0xB7, 0xD6};                                     /* �� */
static const uint8_t k_gbk_total[]      = {0xB5, 0xB1, 0xC7, 0xB0, 0xD7, 0xDC, 0xB7, 0xD6}; /* ��ǰ�ܷ� */
static const uint8_t k_gbk_item[]       = {0xB5, 0xDA};                                     /* �� */
static const uint8_t k_gbk_item_unit[]  = {0xCF, 0xEE};                                     /* �� */
static const uint8_t k_gbk_add[]        = {0xBC, 0xD3, 0xB7, 0xD6};                         /* �ӷ� */
static const uint8_t k_gbk_sub[]        = {0xBF, 0xDB, 0xB7, 0xD6};                         /* �۷� */
static const uint8_t k_gbk_start[]      = {0xB1, 0xC8, 0xC8, 0xFC, 0xBF, 0xAA, 0xCA, 0xBC}; /* ������ʼ */
static const uint8_t k_gbk_end[]        = {0xB1, 0xC8, 0xC8, 0xFC, 0xBD, 0xE1, 0xCA, 0xF8}; /* �������� */
static const uint8_t k_gbk_ready[]      = {
    0xBB, 0xB6, 0xD3, 0xAD, 0xCA, 0xB9, 0xD3, 0xC3, 0xD6, 0xC7, 0xC4, 0xDC, 0xB1, 0xDB, 0xB4, 0xF8
}; /* 欢迎使用智能臂带 */

static const char k_voice_prefix[] = "[v16][m0][t5]";

/** 播放 GBK 二进制文本，自动加语速前缀 */
static HAL_StatusTypeDef score_voice_play_buf(ScoreVoice_Handle_t *ctx,
                                              const uint8_t *buf,
                                              uint16_t len)
{
  char frame[160];
  uint16_t prefix_len = (uint16_t)strlen(k_voice_prefix);

  if ((ctx == NULL) || (ctx->tts == NULL) || (buf == NULL) || (len == 0U)) {
    return HAL_ERROR;
  }
  if ((uint16_t)(prefix_len + len) >= (uint16_t)sizeof(frame)) {
    return HAL_ERROR;
  }

  memcpy(frame, k_voice_prefix, prefix_len);
  memcpy(&frame[prefix_len], buf, len);
  return SYN6288E_PlayText(ctx->tts, (const uint8_t *)frame,
                           (uint16_t)(prefix_len + len), SYN6288E_ENC_GBK);
}

/** ���ַ�����׷�� [n2]���� ��ǣ���оƬ����ֵ�������� */
static uint16_t score_voice_append_num(char *dst, uint16_t pos, uint16_t cap, int32_t value)
{
  char num_ascii[16];
  uint16_t n;

  if (value < 0) {
    if ((pos + 4U) >= cap) {
      return pos;
    }
    dst[pos++] = '-';
    value = -value;
  }

  (void)snprintf(num_ascii, sizeof(num_ascii), "[n2]%ld", (long)value);
  n = (uint16_t)strlen(num_ascii);
  if ((pos + n) >= cap) {
    return pos;
  }
  memcpy(&dst[pos], num_ascii, n);
  return (uint16_t)(pos + n);
}

static HAL_StatusTypeDef score_voice_speak(ScoreVoice_Handle_t *ctx, const char *gbk_buf)
{
  if ((ctx == NULL) || (ctx->tts == NULL) || (gbk_buf == NULL)) {
    return HAL_ERROR;
  }

  if (strncmp(gbk_buf, k_voice_prefix, strlen(k_voice_prefix)) == 0) {
    return SYN6288E_PlayTextGBK(ctx->tts, gbk_buf);
  }

  {
    char buf[160];
    int n = snprintf(buf, sizeof(buf), "%s%s", k_voice_prefix, gbk_buf);
    if ((n <= 0) || ((uint16_t)n >= (uint16_t)sizeof(buf))) {
      return HAL_ERROR;
    }
    return SYN6288E_PlayTextGBK(ctx->tts, buf);
  }
}

/** ����"�ӷ�XX��"��"�۷�XX��" */
static HAL_StatusTypeDef score_voice_announce_delta(ScoreVoice_Handle_t *ctx, int16_t delta)
{
  char buf[96];
  uint16_t pos = 0U;
  const uint8_t *prefix;
  uint16_t prefix_len;
  int16_t magnitude = delta;

  if (delta >= 0) {
    prefix = k_gbk_add;
    prefix_len = sizeof(k_gbk_add);
  } else {
    prefix = k_gbk_sub;
    prefix_len = sizeof(k_gbk_sub);
    magnitude = (int16_t)(-delta);
  }

  memcpy(buf, prefix, prefix_len);
  pos = prefix_len;
  pos = score_voice_append_num(buf, pos, (uint16_t)sizeof(buf), magnitude);
  buf[pos++] = (char)k_gbk_point[0];
  buf[pos++] = (char)k_gbk_point[1];
  buf[pos] = '\0';

  return score_voice_speak(ctx, buf);
}

/** ����"��ǰ�ܷ�XX��" */
static HAL_StatusTypeDef score_voice_announce_total(ScoreVoice_Handle_t *ctx)
{
  char buf[64];
  uint16_t pos = 0U;

  memcpy(buf, k_gbk_total, sizeof(k_gbk_total));
  pos = (uint16_t)sizeof(k_gbk_total);
  pos = score_voice_append_num(buf, pos, (uint16_t)sizeof(buf), ctx->total_score);
  buf[pos++] = (char)k_gbk_point[0];
  buf[pos++] = (char)k_gbk_point[1];
  buf[pos] = '\0';

  return score_voice_speak(ctx, buf);
}

/** 初始化模块：设置音量并播报"欢迎使用智能臂带" */
HAL_StatusTypeDef ScoreVoice_Init(ScoreVoice_Handle_t *ctx, SYN6288E_Handle_t *tts)
{
  if ((ctx == NULL) || (tts == NULL)) {
    return HAL_ERROR;
  }

  memset(ctx, 0, sizeof(*ctx));
  ctx->tts = tts;
  (void)SYN6288E_SetVolume(tts, 16U);
  return score_voice_play_buf(ctx, k_gbk_ready, (uint16_t)sizeof(k_gbk_ready));
}

/** ���� total_score �͸� item_scores */
void ScoreVoice_Reset(ScoreVoice_Handle_t *ctx)
{
  if (ctx == NULL) {
    return;
  }

  ctx->total_score = 0;
  memset(ctx->item_scores, 0, sizeof(ctx->item_scores));
}

int32_t ScoreVoice_GetTotal(const ScoreVoice_Handle_t *ctx)
{
  if (ctx == NULL) {
    return 0;
  }
  return ctx->total_score;
}

/** �ۼӷ������Ȳ����Ӽ����ٲ����ܷ� */
HAL_StatusTypeDef ScoreVoice_AddPoints(ScoreVoice_Handle_t *ctx, int16_t points)
{
  if (ctx == NULL) {
    return HAL_ERROR;
  }

  ctx->total_score += points;
  (void)score_voice_announce_delta(ctx, points);
  return score_voice_announce_total(ctx);
}

HAL_StatusTypeDef ScoreVoice_SetTotal(ScoreVoice_Handle_t *ctx, int32_t total)
{
  if (ctx == NULL) {
    return HAL_ERROR;
  }

  ctx->total_score = total;
  return score_voice_announce_total(ctx);
}

HAL_StatusTypeDef ScoreVoice_ReportTotal(ScoreVoice_Handle_t *ctx)
{
  return score_voice_announce_total(ctx);
}

HAL_StatusTypeDef ScoreVoice_AnnounceStart(ScoreVoice_Handle_t *ctx)
{
  if (ctx == NULL) {
    return HAL_ERROR;
  }
  ScoreVoice_Reset(ctx);
  return score_voice_play_buf(ctx, k_gbk_start, (uint16_t)sizeof(k_gbk_start));
}

HAL_StatusTypeDef ScoreVoice_AnnounceEnd(ScoreVoice_Handle_t *ctx)
{
  HAL_StatusTypeDef st;

  if (ctx == NULL) {
    return HAL_ERROR;
  }

  st = score_voice_play_buf(ctx, k_gbk_end, (uint16_t)sizeof(k_gbk_end));
  if (st != HAL_OK) {
    return st;
  }
  return score_voice_announce_total(ctx);
}

/**
 * @brief  �����������¼�
 *         ���·���/�ܷ֣�����"��X��ε÷�XX��"����ǰ�ܷ�
 */
HAL_StatusTypeDef ScoreVoice_ApplyEvent(ScoreVoice_Handle_t *ctx, const ScoreVoice_Event_t *event)
{
  char buf[96];
  uint16_t pos;
  HAL_StatusTypeDef st;

  if ((ctx == NULL) || (event == NULL)) {
    return HAL_ERROR;
  }

  if ((event->item_id > 0U) && (event->item_id <= SCORE_VOICE_MAX_ITEMS)) {
    ctx->item_scores[event->item_id - 1U] += event->points;
  }
  ctx->total_score += event->points;

  if (event->announce_item && (event->item_id > 0U)) {
    pos = 0U;
    memcpy(buf, k_gbk_item, sizeof(k_gbk_item));
    pos = (uint16_t)sizeof(k_gbk_item);
    pos = score_voice_append_num(buf, pos, (uint16_t)sizeof(buf), event->item_id);
    buf[pos++] = (char)k_gbk_item_unit[0];
    buf[pos++] = (char)k_gbk_item_unit[1];

    memcpy(&buf[pos], k_gbk_this_score, sizeof(k_gbk_this_score));
    pos = (uint16_t)(pos + sizeof(k_gbk_this_score));
    pos = score_voice_append_num(buf, pos, (uint16_t)sizeof(buf), event->points);
    buf[pos++] = (char)k_gbk_point[0];
    buf[pos++] = (char)k_gbk_point[1];
    buf[pos] = '\0';

    st = score_voice_speak(ctx, buf);
    if (st != HAL_OK) {
      return st;
    }
  } else {
    st = score_voice_announce_delta(ctx, event->points);
    if (st != HAL_OK) {
      return st;
    }
  }

  return score_voice_announce_total(ctx);
}

/** �����������ͷַ�����Ӧ�������� */
HAL_StatusTypeDef ScoreVoice_HandleInput(ScoreVoice_Handle_t *ctx, const ScoreVoice_Input_t *input)
{
  ScoreVoice_Event_t event;

  if ((ctx == NULL) || (input == NULL)) {
    return HAL_ERROR;
  }

  switch (input->type) {
  case SCORE_VOICE_INPUT_START:
    return ScoreVoice_AnnounceStart(ctx);

  case SCORE_VOICE_INPUT_END:
    return ScoreVoice_AnnounceEnd(ctx);

  case SCORE_VOICE_INPUT_RESET:
    ScoreVoice_Reset(ctx);
    return HAL_OK;

  case SCORE_VOICE_INPUT_REPORT:
    return ScoreVoice_ReportTotal(ctx);

  case SCORE_VOICE_INPUT_ADD:
    return ScoreVoice_AddPoints(ctx, (int16_t)input->value);

  case SCORE_VOICE_INPUT_SUB:
    return ScoreVoice_AddPoints(ctx, (int16_t)(-input->value));

  case SCORE_VOICE_INPUT_SET:
    return ScoreVoice_SetTotal(ctx, input->value);

  case SCORE_VOICE_INPUT_ITEM:
    event.item_id = input->item_id;
    event.points = (int16_t)input->value;
    event.announce_item = true;
    return ScoreVoice_ApplyEvent(ctx, &event);

  default:
    return HAL_ERROR;
  }
}

void ScoreVoice_Process(ScoreVoice_Handle_t *ctx)
{
  if ((ctx == NULL) || (ctx->tts == NULL)) {
    return;
  }
  SYN6288E_Process(ctx->tts);
}

HAL_StatusTypeDef ScoreVoice_AnnounceInference(ScoreVoice_Handle_t *ctx,
                                               uint16_t label_id,
                                               uint8_t confidence,
                                               int16_t points)
{
  char buf[128];
  uint16_t pos = 0U;
  static const uint8_t k_gbk_detect[] = {0xBC, 0xEC, 0xB2, 0xE2, 0xB5, 0xBD}; /* 检测到 */
  static const uint8_t k_gbk_smash[] = {0xBF, 0xDB, 0xC9, 0xB1};               /* 扣杀 */
  static const uint8_t k_gbk_lift_shot[] = {
      0xB7, 0xB4, 0xCA, 0xD6, 0xCC, 0xF4, 0xC7, 0xF2
  }; /* 反手挑球 */
  static const uint8_t k_gbk_lift[] = {0xCC, 0xF4, 0xC7, 0xF2};                 /* 挑球 */
  static const uint8_t k_gbk_drive[] = {0xC6, 0xBD, 0xB3, 0xE9, 0xC7, 0xF2};   /* 平抽球 */
  static const uint8_t k_gbk_clear[] = {
      0xB8, 0xDF, 0xD4, 0xB6, 0xC7, 0xF2
  }; /* 高远球 */
  const uint8_t *label_gbk = NULL;
  uint16_t label_len = 0U;

  if (ctx == NULL) {
    return HAL_ERROR;
  }

  (void)confidence;

  /* NanoEdgeAI: smash, lift_shot, lift_001, idle2, drive, clear */
  switch (label_id) {
  case 0U:
    label_gbk = k_gbk_smash;
    label_len = sizeof(k_gbk_smash);
    break;
  case 1U:
    label_gbk = k_gbk_lift_shot;
    label_len = sizeof(k_gbk_lift_shot);
    break;
  case 2U:
    label_gbk = k_gbk_lift;
    label_len = sizeof(k_gbk_lift);
    break;
  case 3U:
    /* idle2：空闲，不播报 */
    return HAL_OK;
  case 4U:
    label_gbk = k_gbk_drive;
    label_len = sizeof(k_gbk_drive);
    break;
  case 5U:
    label_gbk = k_gbk_clear;
    label_len = sizeof(k_gbk_clear);
    break;
  default:
    break;
  }

  memcpy(buf, k_gbk_detect, sizeof(k_gbk_detect));
  pos = (uint16_t)sizeof(k_gbk_detect);

  if (label_gbk != NULL) {
    memcpy(&buf[pos], label_gbk, label_len);
    pos = (uint16_t)(pos + label_len);
  } else {
    pos = score_voice_append_num(buf, pos, (uint16_t)sizeof(buf), (int32_t)label_id);
  }

  buf[pos++] = (char)0xA3;
  buf[pos++] = (char)0xAC;
  pos = score_voice_append_num(buf, pos, (uint16_t)sizeof(buf), points);
  buf[pos++] = (char)k_gbk_point[0];
  buf[pos++] = (char)k_gbk_point[1];
  ctx->total_score += points;

  buf[pos] = '\0';
  return score_voice_speak(ctx, buf);
}
