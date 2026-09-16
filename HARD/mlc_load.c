#include "mlc_load.h"
#include "mlc.h"
#include "lsm.h"
#include "main.h"

static int mlc_apply_sensor_config(uint8_t sensor_idx)
{
  const struct mems_conf_op *ops;
  uint32_t len;
  uint32_t i;

  if (sensor_idx >= MLC_SENSORS_NUM)
  {
    return -1;
  }

  ops = mlc_confs[sensor_idx].list;
  len = mlc_confs[sensor_idx].len;

  for (i = 0U; i < len; i++)
  {
    switch (ops[i].type)
    {
      case MEMS_CONF_OP_TYPE_WRITE:
        LSM_WriteReg(ops[i].address, ops[i].data);
        break;

      case MEMS_CONF_OP_TYPE_DELAY:
        HAL_Delay(ops[i].data);
        break;

      case MEMS_CONF_OP_TYPE_READ:
        (void)LSM_ReadReg(ops[i].address);
        break;

      case MEMS_CONF_OP_TYPE_POLL_SET:
      case MEMS_CONF_OP_TYPE_POLL_RESET:
      default:
        break;
    }
  }

  return 0;
}

int MLC_LoadConfig(void)
{
  if (mlc_apply_sensor_config(0) != 0)
  {
    return -1;
  }

  HAL_Delay(20U);
  return 0;
}
