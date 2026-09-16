/**
  ******************************************************************************
  * @file    host_score.h
  * @brief   主控 / Edge Nano 模型打分接入层
  *
  * 数据流：Edge Nano 推理 -> 主控计分 -> host_score -> score_voice -> SYN6288E
  ******************************************************************************
  */
#ifndef __HOST_SCORE_H__
#define __HOST_SCORE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "score_voice.h"
#include <stdint.h>
#include <stdbool.h>

/** 主控通信帧头魔数 */
#define HOST_SCORE_MAGIC              0xA5U
/** 接收缓冲区大小 */
#define HOST_SCORE_RX_BUF_SIZE        64U
/** 最大计分规则条数 */
#define HOST_SCORE_MAX_RULES          32U

/** 主控发往语音模块的命令字 */
typedef enum {
  HOST_SCORE_CMD_START   = 0x01U,  /**< 比赛开始 */
  HOST_SCORE_CMD_END     = 0x02U,  /**< 比赛结束 */
  HOST_SCORE_CMD_RESET   = 0x03U,  /**< 清零分数 */
  HOST_SCORE_CMD_REPORT  = 0x04U,  /**< 播报总分 */
  HOST_SCORE_CMD_INFER   = 0x10U,  /**< Edge Nano 模型推理结果 */
  HOST_SCORE_CMD_RAW_ADD = 0x11U,  /**< 主控直接指定加减分 */
} HostScore_Cmd_t;

/** Edge Nano 单次推理结果（主控解析模型输出后填入） */
typedef struct {
  uint8_t  item_id;        /**< 比赛项目编号 1~16，0 表示不区分项目 */
  uint16_t label_id;       /**< 模型输出类别 ID（与训练标签一致） */
  uint8_t  confidence;     /**< 推理置信度 0~100 */
  int16_t  points;         /**< 主控指定得分；为 0 时按规则表自动换算 */
  bool     announce_item;  /**< 是否播报分项得分 */
} HostScore_Inference_t;

/** 模型标签到得分的映射规则 */
typedef struct {
  uint16_t label_id;       /**< 模型类别 ID */
  uint8_t  item_id;        /**< 对应比赛项目，0 表示不限项目 */
  int16_t  points;         /**< 满足条件时的得分 */
  uint8_t  min_confidence; /**< 最低置信度门槛（低于则不计分） */
} HostScore_Rule_t;

/** 主控打分接入句柄 */
typedef struct {
  ScoreVoice_Handle_t *voice;           /**< 绑定的语音打分模块 */
  const HostScore_Rule_t *rules;        /**< 计分规则表指针 */
  uint8_t rule_count;                   /**< 规则表条目数 */
  uint8_t default_min_confidence;       /**< 无匹配规则时的默认置信度门槛 */
  int16_t default_points;               /**< 无匹配规则时的默认得分 */
  uint8_t rx_buf[HOST_SCORE_RX_BUF_SIZE]; /**< 帧接收缓冲区 */
  uint8_t rx_len;                       /**< 当前已接收字节数 */
  uint8_t frame_cmd;                    /**< 正在解析的命令字 */
  uint8_t frame_len;                    /**< 正在解析的 payload 长度 */
  uint8_t frame_idx;                    /**< payload 已接收字节计数 */
  enum {
    HOST_SCORE_PARSE_SYNC = 0,     /**< 等待帧头 0xA5 */
    HOST_SCORE_PARSE_CMD,          /**< 等待命令字 */
    HOST_SCORE_PARSE_LEN,          /**< 等待长度字节 */
    HOST_SCORE_PARSE_PAYLOAD,      /**< 接收 payload */
    HOST_SCORE_PARSE_CHECKSUM,     /**< 等待校验字节 */
  } parse_state;                   /**< 帧解析状态 */
} HostScore_Handle_t;

/** 初始化主控打分接入层，rules 为 NULL 时使用默认规则表 */
HAL_StatusTypeDef HostScore_Init(HostScore_Handle_t *ctx,
                                 ScoreVoice_Handle_t *voice,
                                 const HostScore_Rule_t *rules,
                                 uint8_t rule_count);

/** 提交 Edge Nano 推理结果，自动换算得分并语音播报 */
HAL_StatusTypeDef HostScore_SubmitInference(HostScore_Handle_t *ctx,
                                            const HostScore_Inference_t *infer);

/** 主控直接指定加减分（跳过规则表） */
HAL_StatusTypeDef HostScore_SubmitRawPoints(HostScore_Handle_t *ctx,
                                            uint8_t item_id,
                                            int16_t points,
                                            bool announce_item);

/** 逐字节喂入主控数据帧（I2C/SPI 中断中调用） */
void HostScore_FeedByte(HostScore_Handle_t *ctx, uint8_t byte);

/** 轮询处理语音模块（主循环中调用） */
void HostScore_Process(HostScore_Handle_t *ctx);

/** 根据 label_id 和置信度查表换算得分 */
int16_t HostScore_LookupPoints(const HostScore_Handle_t *ctx,
                               uint16_t label_id,
                               uint8_t item_id,
                               uint8_t confidence);

#ifdef __cplusplus
}
#endif

#endif /* __HOST_SCORE_H__ */
