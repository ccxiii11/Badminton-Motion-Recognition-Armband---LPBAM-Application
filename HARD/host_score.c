/**
  ******************************************************************************
  * @file    host_score.c
  * @brief   ���� / Edge Nano ģ�ʹ�ֽ���ʵ��
  ******************************************************************************
  */
#include "host_score.h"
#include <string.h>

/*
 * Ĭ�ϼƷֹ���� ���� �밴 Edge Nano ʵ��ѵ����ǩ�޸� label_id / item_id / points
 * ʾ����label 0=��ȷ����+10�֣�label 1=Υ��-5�֣�label 2=���Ŀ��+20��
 */
static const HostScore_Rule_t k_default_rules[] = {
    { .label_id = 0U, .item_id = 0U, .points = 10,  .min_confidence = 60U },
    { .label_id = 1U, .item_id = 0U, .points = 10,  .min_confidence = 60U },
    { .label_id = 2U, .item_id = 0U, .points = 10,  .min_confidence = 60U },
    { .label_id = 3U, .item_id = 0U, .points = 10,  .min_confidence = 60U },
    { .label_id = 4U, .item_id = 0U, .points = 10,  .min_confidence = 60U },
    { .label_id = 5U, .item_id = 0U, .points = 10,  .min_confidence = 60U },
};

/** ������֡ XOR У��ֵ */
static uint8_t host_score_xor(const uint8_t *data, uint8_t len)
{
  uint8_t xor_val = 0U;
  uint8_t i;

  for (i = 0U; i < len; i++) {
    xor_val ^= data[i];
  }
  return xor_val;
}

/** ����������ת�����������ģ�� */
static HAL_StatusTypeDef host_score_dispatch_voice(HostScore_Handle_t *ctx,
                                                 ScoreVoice_InputType_t type,
                                                 int32_t value,
                                                 uint8_t item_id)
{
  ScoreVoice_Input_t input;

  if ((ctx == NULL) || (ctx->voice == NULL)) {
    return HAL_ERROR;
  }

  input.type = type;
  input.value = value;
  input.item_id = item_id;
  return ScoreVoice_HandleInput(ctx->voice, &input);
}

/** ���÷��¼��ύ������ģ�鲥�� */
static HAL_StatusTypeDef host_score_apply_points(HostScore_Handle_t *ctx,
                                                 uint8_t item_id,
                                                 int16_t points,
                                                 bool announce_item)
{
  ScoreVoice_Event_t event;

  if (ctx == NULL) {
    return HAL_ERROR;
  }

  if (points == 0) {
    return HAL_OK;
  }

  event.item_id = item_id;
  event.points = points;
  event.announce_item = announce_item;
  return ScoreVoice_ApplyEvent(ctx->voice, &event);
}

/** ������������֡��ִ�ж�Ӧ���/�������� */
static HAL_StatusTypeDef host_score_handle_frame(HostScore_Handle_t *ctx,
                                                 uint8_t cmd,
                                                 const uint8_t *payload,
                                                 uint8_t len)
{
  HostScore_Inference_t infer;
  int16_t points;
  int16_t raw_points;

  if (ctx == NULL) {
    return HAL_ERROR;
  }

  switch (cmd) {
  case HOST_SCORE_CMD_START:
    return host_score_dispatch_voice(ctx, SCORE_VOICE_INPUT_START, 0, 0U);

  case HOST_SCORE_CMD_END:
    return host_score_dispatch_voice(ctx, SCORE_VOICE_INPUT_END, 0, 0U);

  case HOST_SCORE_CMD_RESET:
    return host_score_dispatch_voice(ctx, SCORE_VOICE_INPUT_RESET, 0, 0U);

  case HOST_SCORE_CMD_REPORT:
    return host_score_dispatch_voice(ctx, SCORE_VOICE_INPUT_REPORT, 0, 0U);

  case HOST_SCORE_CMD_INFER:
    if (len < 7U) {
      return HAL_ERROR;
    }
    infer.item_id = payload[0];
    infer.label_id = (uint16_t)(payload[1] | ((uint16_t)payload[2] << 8));
    infer.confidence = payload[3];
    raw_points = (int16_t)(payload[4] | ((int16_t)payload[5] << 8));
    infer.announce_item = ((payload[6] & 0x01U) != 0U);
    infer.points = raw_points;
    return HostScore_SubmitInference(ctx, &infer);

  case HOST_SCORE_CMD_RAW_ADD:
    if (len < 4U) {
      return HAL_ERROR;
    }
    points = (int16_t)(payload[1] | ((int16_t)payload[2] << 8));
    return host_score_apply_points(ctx, payload[0], points, ((payload[3] & 0x01U) != 0U));

  default:
    return HAL_ERROR;
  }
}

/**
 * @brief  ��ʼ�����ش�ֽ����
 *         rules �� NULL ʱʹ�� k_default_rules Ĭ�Ϲ����
 */
HAL_StatusTypeDef HostScore_Init(HostScore_Handle_t *ctx,
                                 ScoreVoice_Handle_t *voice,
                                 const HostScore_Rule_t *rules,
                                 uint8_t rule_count)
{
  if ((ctx == NULL) || (voice == NULL)) {
    return HAL_ERROR;
  }

  memset(ctx, 0, sizeof(*ctx));
  ctx->voice = voice;
  ctx->rules = (rules != NULL) ? rules : k_default_rules;
  ctx->rule_count = (rules != NULL) ? rule_count : (uint8_t)(sizeof(k_default_rules) / sizeof(k_default_rules[0]));
  ctx->default_min_confidence = 70U;
  ctx->default_points = 0;
  ctx->parse_state = HOST_SCORE_PARSE_SYNC;
  return HAL_OK;
}

/**
 * @brief  �� label_id ����������÷�
 *         ���ŶȲ������ƥ�����ʱ���� 0�����Ʒ֣�
 */
int16_t HostScore_LookupPoints(const HostScore_Handle_t *ctx,
                               uint16_t label_id,
                               uint8_t item_id,
                               uint8_t confidence)
{
  uint8_t i;

  if (ctx == NULL) {
    return 0;
  }

  for (i = 0U; i < ctx->rule_count; i++) {
    const HostScore_Rule_t *rule = &ctx->rules[i];

    if (rule->label_id != label_id) {
      continue;
    }
    if ((rule->item_id != 0U) && (rule->item_id != item_id)) {
      continue;
    }
    if (confidence < rule->min_confidence) {
      continue;
    }
    return rule->points;
  }

  if (confidence < ctx->default_min_confidence) {
    return 0;
  }
  return ctx->default_points;
}

/**
 * @brief  �ύ Edge Nano �������
 *         infer->points �� 0 ʱ������ָ������Ϊ׼�����������
 */
HAL_StatusTypeDef HostScore_SubmitInference(HostScore_Handle_t *ctx,
                                            const HostScore_Inference_t *infer)
{
  int16_t points;

  if ((ctx == NULL) || (infer == NULL)) {
    return HAL_ERROR;
  }

  if (infer->points != 0) {
    points = infer->points;
  } else {
    points = HostScore_LookupPoints(ctx, infer->label_id, infer->item_id, infer->confidence);
  }

  if (points == 0) {
    return HAL_OK;
  }

  return host_score_apply_points(ctx, infer->item_id, points, infer->announce_item);
}

HAL_StatusTypeDef HostScore_SubmitRawPoints(HostScore_Handle_t *ctx,
                                            uint8_t item_id,
                                            int16_t points,
                                            bool announce_item)
{
  return host_score_apply_points(ctx, item_id, points, announce_item);
}

/** ״̬�����ֽڽ�����������֡��0xA5 | CMD | LEN | PAYLOAD | XOR */
void HostScore_FeedByte(HostScore_Handle_t *ctx, uint8_t byte)
{
  uint8_t checksum;

  if (ctx == NULL) {
    return;
  }

  switch (ctx->parse_state) {
  case HOST_SCORE_PARSE_SYNC:
    if (byte == HOST_SCORE_MAGIC) {
      ctx->rx_buf[0] = byte;
      ctx->rx_len = 1U;
      ctx->parse_state = HOST_SCORE_PARSE_CMD;
    }
    break;

  case HOST_SCORE_PARSE_CMD:
    ctx->frame_cmd = byte;
    ctx->rx_buf[ctx->rx_len++] = byte;
    ctx->parse_state = HOST_SCORE_PARSE_LEN;
    break;

  case HOST_SCORE_PARSE_LEN:
    ctx->frame_len = byte;
    ctx->rx_buf[ctx->rx_len++] = byte;
    if (ctx->frame_len > (HOST_SCORE_RX_BUF_SIZE - 4U)) {
      ctx->parse_state = HOST_SCORE_PARSE_SYNC;
      break;
    }
    ctx->frame_idx = 0U;
    ctx->parse_state = (ctx->frame_len == 0U) ? HOST_SCORE_PARSE_CHECKSUM : HOST_SCORE_PARSE_PAYLOAD;
    break;

  case HOST_SCORE_PARSE_PAYLOAD:
    ctx->rx_buf[ctx->rx_len++] = byte;
    ctx->frame_idx++;
    if (ctx->frame_idx >= ctx->frame_len) {
      ctx->parse_state = HOST_SCORE_PARSE_CHECKSUM;
    }
    break;

  case HOST_SCORE_PARSE_CHECKSUM:
    checksum = host_score_xor(ctx->rx_buf, ctx->rx_len);
    if (checksum == byte) {
      (void)host_score_handle_frame(ctx, ctx->frame_cmd, &ctx->rx_buf[3], ctx->frame_len);
    }
    ctx->parse_state = HOST_SCORE_PARSE_SYNC;
    break;

  default:
    ctx->parse_state = HOST_SCORE_PARSE_SYNC;
    break;
  }
}

void HostScore_Process(HostScore_Handle_t *ctx)
{
  if ((ctx == NULL) || (ctx->voice == NULL)) {
    return;
  }
  ScoreVoice_Process(ctx->voice);
}
