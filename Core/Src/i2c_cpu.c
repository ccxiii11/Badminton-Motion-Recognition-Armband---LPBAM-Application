#include "i2c_cpu.h"
    /*开启I2C在cpu下对传感器初始化*/
void I2C3_CPU_Init(void)
{
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x00100D14;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

void I2C3_CPU_DeInit(void)
{
  if (HAL_I2C_DeInit(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
  GPIO_InitTypeDef gpio_init = {0};
  RCC_PeriphCLKInitTypeDef periph_clk = {0};

  if (hi2c->Instance != I2C3)
  {
    return;
  }

  periph_clk.PeriphClockSelection = RCC_PERIPHCLK_I2C3;
  periph_clk.I2c3ClockSelection = RCC_I2C3CLKSOURCE_MSIK;
  if (HAL_RCCEx_PeriphCLKConfig(&periph_clk) != HAL_OK)
  {
    Error_Handler();
  }

  __HAL_RCC_GPIOC_CLK_ENABLE();
  gpio_init.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  gpio_init.Mode = GPIO_MODE_AF_OD;
  gpio_init.Pull = GPIO_NOPULL;
  gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
  gpio_init.Alternate = GPIO_AF4_I2C3;
  HAL_GPIO_Init(GPIOC, &gpio_init);

  __HAL_RCC_I2C3_CLK_ENABLE();
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance != I2C3)
  {
    return;
  }

  __HAL_RCC_I2C3_CLK_DISABLE();
  HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0 | GPIO_PIN_1);
}
