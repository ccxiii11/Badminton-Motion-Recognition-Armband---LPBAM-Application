#ifndef __LSM_REG_H
#define __LSM_REG_H

//芯片型号
#define LSM_WHO_AM_I           0x0F

//数据寄存器
#define  LSM_OUTX_L_G          0X22 
#define  LSM_OUTX_H_G          0X23
#define  LSM_OUTY_L_G          0X24
#define  LSM_OUTY_H_G          0X25
#define  LSM_OUTZ_L_G          0X26
#define  LSM_OUTZ_H_G          0X27
#define  LSM_OUTX_L_A          0X28
#define  LSM_OUTX_H_A          0X29
#define  LSM_OUTY_L_A          0X2A
#define  LSM_OUTY_H_A          0X2B
#define  LSM_OUTZ_L_A          0X2C
#define  LSM_OUTZ_H_A          0X2D


//控制寄存器
#define   LSM_CTRL_A           0x10
#define   LSM_CTRL_G           0x11
#define   LSM_CTRL_MAIN_BDU    0x12
#define   LSM_CTRL_POWER       0x01
#define   LSM_G_CONFIG         0x15
#define   LSM_A_CONFIG         0x17

#define   LSM_FUNC_CFG_ACCESS  0x01U
#define   LSM_CTRL9            0x18U
#define   LSM_FUNCTIONS_ENABLE 0x50U
#define   LSM_INACTIVITY_DUR   0x54U
#define   LSM_TAP_CFG0         0x56U
#define   LSM_WAKE_UP_THS      0x5BU
#define   LSM_WAKE_UP_DUR      0x5CU
#define   LSM_MD1_CFG          0x5EU
#define   LSM_WAKE_UP_SRC      0x45U
#define   LSM_ALL_INT_SRC      0x1DU
#define   LSM_MLC1_SRC         0x70U
#define   LSM_MLC_INT1         0x0DU

#define   LSM_EMB_FUNC_ACCESS  0x80U
#define   LSM_MD1_CFG_INT1_WU  0x20U

#endif