#ifndef __MODEL_INFER_H__
#define __MODEL_INFER_H__

#include "main.h"
#include <stdint.h>

typedef struct {
  uint16_t label_id;
  uint8_t confidence;
} Model_Output_t;

int Model_Init(void);
int Model_Run(const imu_traj_frame_t *traj, uint32_t frame_count, Model_Output_t *out);

#endif /* __MODEL_INFER_H__ */
