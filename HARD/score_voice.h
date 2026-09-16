/**
  ******************************************************************************
  * @file    score_voice.h
  * @brief   ���� SYN6288E ���������ģ��
  * @note    USART3 ������ SYN6288E���������������ͨ�� API ����
  ******************************************************************************
  */
#ifndef __SCORE_VOICE_H__
#define __SCORE_VOICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "syn6288e.h"
#include <stdint.h>
#include <stdbool.h>

/** ���֧�ֵı�����Ŀ���� */
#define SCORE_VOICE_MAX_ITEMS   16U

/** ���δ���¼�������÷֣� */
typedef struct {
  uint8_t item_id;       /**< ������Ŀ��ţ�1~16 */
  int16_t points;        /**< ���ε÷֣���Ϊ������ʾ�۷֣� */
  bool announce_item;    /**< �Ƿ񲥱�"��X����ε÷�XX��" */
} ScoreVoice_Event_t;

/** ͳһ������������ */
typedef enum {
  SCORE_VOICE_INPUT_START = 0,  /**< ������ʼ�����㲢���� */
  SCORE_VOICE_INPUT_END,        /**< ����������������������ܷ� */
  SCORE_VOICE_INPUT_RESET,      /**< ����������������� */
  SCORE_VOICE_INPUT_REPORT,     /**< ��������ǰ�ܷ� */
  SCORE_VOICE_INPUT_ADD,        /**< �ӷֲ����� */
  SCORE_VOICE_INPUT_SUB,        /**< ���ֲ����� */
  SCORE_VOICE_INPUT_SET,        /**< �����ֲܷ����� */
  SCORE_VOICE_INPUT_ITEM,       /**< �����ֲ����� */
} ScoreVoice_InputType_t;

/** ͳһ��������ṹ�� */
typedef struct {
  ScoreVoice_InputType_t type;  /**< �������� */
  int32_t value;                /**< ����ֵ���Ӽ�/����/����÷֣� */
  uint8_t item_id;              /**< ��Ŀ��ţ�������ʱʹ�ã� */
} ScoreVoice_Input_t;

/** �������ģ���� */
typedef struct {
  SYN6288E_Handle_t *tts;                       /**< �󶨵� SYN6288E ���� */
  int32_t total_score;                          /**< ��ǰ�ۼ��ܷ� */
  int32_t item_scores[SCORE_VOICE_MAX_ITEMS];   /**< ����Ŀ�ۼƵ÷� */
} ScoreVoice_Handle_t;

/** ��ʼ���������ģ�飬�� SYN6288E ������������ʾ */
HAL_StatusTypeDef ScoreVoice_Init(ScoreVoice_Handle_t *ctx, SYN6288E_Handle_t *tts);

/** �����ֺܷ͸���Ŀ���� */
void ScoreVoice_Reset(ScoreVoice_Handle_t *ctx);

/** ����ͳһ����ṹ��ִ�д��/�������� */
HAL_StatusTypeDef ScoreVoice_HandleInput(ScoreVoice_Handle_t *ctx, const ScoreVoice_Input_t *input);

/** Ӧ�õ��δ���¼��������Ӽ��֣����������� */
HAL_StatusTypeDef ScoreVoice_ApplyEvent(ScoreVoice_Handle_t *ctx, const ScoreVoice_Event_t *event);

/** �Ӽ��ֲ�����������ǰ�ܷ֣� */
HAL_StatusTypeDef ScoreVoice_AddPoints(ScoreVoice_Handle_t *ctx, int16_t points);

/** ֱ�������ֲܷ����� */
HAL_StatusTypeDef ScoreVoice_SetTotal(ScoreVoice_Handle_t *ctx, int32_t total);

/** ��������ǰ�ܷ� */
HAL_StatusTypeDef ScoreVoice_ReportTotal(ScoreVoice_Handle_t *ctx);

/** ����������ʼ������ */
HAL_StatusTypeDef ScoreVoice_AnnounceStart(ScoreVoice_Handle_t *ctx);

/** �������������������ܷ� */
HAL_StatusTypeDef ScoreVoice_AnnounceEnd(ScoreVoice_Handle_t *ctx);

/** ��ѯ���� SYN6288E ״̬����ѭ���е��ã� */
void ScoreVoice_Process(ScoreVoice_Handle_t *ctx);

HAL_StatusTypeDef ScoreVoice_AnnounceInference(ScoreVoice_Handle_t *ctx,
                                               uint16_t label_id,
                                               uint8_t confidence,
                                               int16_t points);

/** ��ȡ��ǰ�ۼ��ܷ� */
int32_t ScoreVoice_GetTotal(const ScoreVoice_Handle_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* __SCORE_VOICE_H__ */
