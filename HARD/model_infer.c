#include "model_infer.h"
#include "NanoEdgeAI.h"

static float input_signal[NEAI_INPUT_SIGNAL_LENGTH * NEAI_INPUT_AXIS_NUMBER];
static float probabilities[NEAI_NUMBER_OF_CLASSES];

static uint32_t select_peak_frame(const imu_traj_frame_t *traj, uint32_t frame_count)
{
  uint32_t best = 0U;
  int64_t best_mag = -1;

  for (uint32_t i = 0U; i < frame_count; i++)
  {
    int32_t ax = (int32_t)traj[i].ax;
    int32_t ay = (int32_t)traj[i].ay;
    int32_t az = (int32_t)traj[i].az;
    int64_t mag = ((int64_t)ax * ax) + ((int64_t)ay * ay) + ((int64_t)az * az);

    if (mag > best_mag)
    {
      best_mag = mag;
      best = i;
    }
  }
  return best;
}

static void frame_to_input(const imu_traj_frame_t *frame, float *out)
{
  out[0] = (float)frame->ax;
  out[1] = (float)frame->ay;
  out[2] = (float)frame->az;
  out[3] = (float)frame->gx;
  out[4] = (float)frame->gy;
  out[5] = (float)frame->gz;
}

int Model_Init(void)
{
  return (neai_classification_init() == NEAI_OK) ? 0 : -1;
}

int Model_Run(const imu_traj_frame_t *traj, uint32_t frame_count, Model_Output_t *out)
{
  uint32_t peak_idx;
  int id_class = 0;
  enum neai_state state;
  float confidence_pct;

  if ((traj == NULL) || (out == NULL) || (frame_count == 0U))
  {
    return -1;
  }

  peak_idx = select_peak_frame(traj, frame_count);
  frame_to_input(&traj[peak_idx], input_signal);

  state = neai_classification(input_signal, probabilities, &id_class);
  if (state != NEAI_OK)
  {
    return -1;
  }
  if ((id_class < 0) || (id_class >= NEAI_NUMBER_OF_CLASSES))
  {
    return -1;
  }

  confidence_pct = probabilities[id_class] * 100.0f;
  if (confidence_pct < 0.0f)
  {
    confidence_pct = 0.0f;
  }
  if (confidence_pct > 100.0f)
  {
    confidence_pct = 100.0f;
  }

  out->label_id = (uint16_t)id_class;
  out->confidence = (uint8_t)(confidence_pct + 0.5f);
  return 0;
}
