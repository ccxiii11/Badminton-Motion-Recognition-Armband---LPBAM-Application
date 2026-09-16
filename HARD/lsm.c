#include "lsm.h"
#include "i2c_cpu.h"

#define LSM_ADDRESS 0xD4U

void LSM_WriteReg(uint8_t RegAddress, uint8_t Data)
{
  (void)HAL_I2C_Mem_Write(&hi2c3, LSM_ADDRESS, RegAddress, I2C_MEMADD_SIZE_8BIT, &Data, 1, 1000);
}

uint8_t LSM_ReadReg(uint8_t RegAddress)
{
  uint8_t data = 0;

  (void)HAL_I2C_Mem_Read(&hi2c3, LSM_ADDRESS, RegAddress, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
  return data;
}

void LSM_Init(void)
{
  LSM_WriteReg(LSM_CTRL_POWER, 0x04);
  HAL_Delay(10);
  LSM_WriteReg(LSM_CTRL_A, 0x08);
  LSM_WriteReg(LSM_CTRL_G, 0x08);
  LSM_WriteReg(LSM_CTRL_MAIN_BDU, 0x44);
  LSM_WriteReg(LSM_A_CONFIG, 0x03);
  LSM_WriteReg(LSM_G_CONFIG, 0x04);
}

void LSM_ConfigureMotionStartInt(void)
{
  /* MLC 默认占用 INT1；改为 wake-up，动作开始时硬件中断触发 INT1→PB4→EXTI4 */
  LSM_WriteReg(LSM_FUNC_CFG_ACCESS, LSM_EMB_FUNC_ACCESS);
  LSM_WriteReg(LSM_MLC_INT1, 0x00U);
  LSM_WriteReg(LSM_FUNC_CFG_ACCESS, 0x00U);

  /* AN5763 §5.3 wake-up → INT1，LIR=0 脉冲模式可重复触发 */
  LSM_WriteReg(LSM_INACTIVITY_DUR, 0x34U);
  LSM_WriteReg(LSM_TAP_CFG0, 0x00U);
  LSM_WriteReg(LSM_WAKE_UP_THS, 0x02U);
  LSM_WriteReg(LSM_WAKE_UP_DUR, 0x02U);
  LSM_WriteReg(LSM_MD1_CFG, LSM_MD1_CFG_INT1_WU);
  LSM_WriteReg(LSM_FUNCTIONS_ENABLE, 0x80U);
  HAL_Delay(10U);
  (void)LSM_ReadReg(LSM_WAKE_UP_SRC);
  (void)LSM_ReadReg(LSM_ALL_INT_SRC);
}

void LSM_ClearMotionInt(void)
{
  (void)LSM_ReadReg(LSM_WAKE_UP_SRC);
}

uint8_t LSM_ReadMlcResult(void)
{
  return LSM_ReadReg(LSM_MLC1_SRC);
}

uint8_t LSM_GetID(void)
{
  return LSM_ReadReg(LSM_WHO_AM_I);
}

uint8_t LSM_WaitReady(uint32_t timeout_ms)
{
  uint32_t t0 = HAL_GetTick();
  uint8_t id = 0U;

  while ((HAL_GetTick() - t0) < timeout_ms)
  {
    id = LSM_GetID();
    if ((id != 0x00U) && (id != 0xFFU))
    {
      return id;
    }
    HAL_Delay(20U);
  }

  return id;
}
