#include "app.h"
#include "main.h"
#include "lpbam_user.h"
#include "lpdma.h"
#include "usart.h"
#include "syn6288e.h"
#include "score_voice.h"
#include "model_infer.h"
#include "NanoEdgeAI.h"
#include <stdio.h>

                       /**陀螺仪量程**/
#define LSM6_ACC_FS_G      16
#define LSM6_GYRO_FS_DPS   2000
#define APP_ENABLE_CSV     0
/*定义状态*/
typedef enum
{
  APP_LSM_SAMPLE,
  APP_LSM_PARSE,
  APP_LSM_INFER,
  APP_LSM_VOICE_WAIT,
  APP_LSM_PRINT,
} app_state_t;
//数据存储区域   
extern imu_traj_frame_t imu_traj[IMU_FRAME_COUNT]; 
      /*状态机使用变量*/
#define APP_VOICE_WAIT_TIMEOUT_MS  8000U
#define APP_VOICE_QUERY_GAP_MS     100U
      /*音频模块调用变量*/
SYN6288E_Handle_t g_syn6288e;
static ScoreVoice_Handle_t s_voice;
static app_state_t s_state;
static uint8_t s_infer_ready;
static uint32_t s_voice_wait_t0;
             /**由寄存器值转加速度角速度值处理函数**/
// static int32_t lsm6_raw_to_mg(int16_t raw)
// {
//   return ((int32_t)raw * LSM6_ACC_FS_G * 1000L) / 32768L;
// }

// static int32_t lsm6_raw_to_cdps(int16_t raw)
// {
//   return ((int32_t)raw * LSM6_GYRO_FS_DPS * 100L) / 32768L;
// }
// static void printf_mg(int32_t mg)
// {
//   int32_t abs_mg = mg < 0 ? -mg : mg;
//   if (mg < 0)
//   {
//     printf(" -%ld.%03ld", (long)abs_mg / 1000, (long)abs_mg % 1000);
//   }
//   else
//   {
//     printf(" %ld.%03ld", (long)abs_mg / 1000, (long)abs_mg % 1000);
//   }
// }
// static void printf_cdps(int32_t cdps)
// {
//   int32_t abs_v = cdps < 0 ? -cdps : cdps;
//   if (cdps < 0)
//   {
//     printf(" -%ld.%02ld", (long)abs_v / 100, (long)abs_v % 100);
//   }
//   else
//   {
//     printf(" %ld.%02ld", (long)abs_v / 100, (long)abs_v % 100);
//   }
// }
// static void print_traj_csv(void)
// {
//   printf("idx,ax(g),ay,az,gx(dps),gy,gz\r\n");
//   for (uint32_t i = 0U; i < IMU_FRAME_COUNT; i++)
//   {
//     printf("%lu,", (unsigned long)i);
//     printf_mg(lsm6_raw_to_mg(imu_traj[i].ax));
//     printf(",");
//     printf_mg(lsm6_raw_to_mg(imu_traj[i].ay));
//     printf(",");
//     printf_mg(lsm6_raw_to_mg(imu_traj[i].az));
//     printf(",");
//     printf_cdps(lsm6_raw_to_cdps(imu_traj[i].gx));
//     printf(",");
//     printf_cdps(lsm6_raw_to_cdps(imu_traj[i].gy));
//     printf(",");
//     printf_cdps(lsm6_raw_to_cdps(imu_traj[i].gz));
//     printf("\r\n");
//   }
// }

//          /**进入stop2模式**/
//static void run_lpbam_sample(void)
//{
//  //开启LPBAM采集
//  LPBAM_User_BeginSampling();
//  //挂起系统滴答
//  HAL_SuspendTick();
//  while (!LPBAM_User_IsSampleDone())
//  {
//    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
//    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_STOPF);
//    SystemClock_Config();
//  }
//  //恢复系统滴答
//  HAL_ResumeTick();
//  //重新配置时钟
//  SystemClock_Config();
//  //停止LPBAM采集
//  LPBAM_User_StopSampleTimer();
//  HAL_Delay(20U);
//  MX_USART1_UART_Init();   /* 恢复 BLE 用的 USART1，不是调试口 */
//}

static void app_init_infer_stack(void)
{
  HAL_StatusTypeDef vst;

  if (s_infer_ready != 0U)
  {
    return;
  }

  if (Model_Init() != 0)
  {
    printf("NanoEdge init failed\r\n");
  }

  MX_USART3_UART_Init();
  (void)SYN6288E_Init(&g_syn6288e, &huart3);
  USART3_StartRxIT();
  printf("infer: syn6288 st=0x%02X\r\n", (unsigned)g_syn6288e.last_status);

  g_syn6288e.wait_idle = false;
  vst = ScoreVoice_Init(&s_voice, &g_syn6288e);
  g_syn6288e.wait_idle = true;
  printf("infer: voice init tts=%d st=0x%02X\r\n", (int)vst, (unsigned)g_syn6288e.last_status);

  s_infer_ready = 1U;
}
static void run_lpbam_sample(void)
{
  printf("waiting motion start (INT1->PB4)...\r\n");
  fflush(stdout);
  LPBAM_User_BeginSampling();
  HAL_SuspendTick();
  while (!LPBAM_User_IsSampleDone())
  {
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_STOPF);
    SystemClock_RestoreAfterStop2();
  }
  HAL_ResumeTick();
  SystemClock_RestoreAfterStop2();
  LPBAM_User_StopSampleTimer();
  HAL_Delay(20U);
  MX_USART1_UART_Init();
  if (LPBAM_User_HasSampleError())
  {
    printf("lpbam sample error\r\n");
  }
  else
  {
    printf("lpbam sample done (%u frames)\r\n", (unsigned)IMU_FRAME_COUNT);
  }
}
static void voice_uart_ensure(void)
{
  /* 只恢复 UART，不再固定 WaitIdle(5s)；播完由 VOICE_WAIT 保证 */
  MX_USART3_UART_Init();
  USART3_StartRxIT();
  SYN6288E_OnUartReady(&g_syn6288e);
  HAL_Delay(20U);
}

/* 返回 1：已发起播报，需进入 VOICE_WAIT；0：无需等待 */
static uint8_t run_inference_and_voice(void)
{
  Model_Output_t out;
  const char *action_name;
  int16_t points;
  HAL_StatusTypeDef vst;
  uint32_t t0;
  uint32_t model_ms;

  voice_uart_ensure();

  t0 = HAL_GetTick();
  if (Model_Run(imu_traj, IMU_FRAME_COUNT, &out) != 0)
  {
    printf("model infer failed\r\n");
    return 0U;
  }
  model_ms = HAL_GetTick() - t0;

  action_name = neai_get_class_name((int)out.label_id);
  if (action_name == NULL)
  {
    action_name = "unknown";
  }

  points = (int16_t)out.confidence;
  if (out.label_id == 3U)
  {
    printf("infer: model=%lums action=%s label=%u (idle, skip voice)\r\n",
           (unsigned long)model_ms,
           action_name,
           (unsigned)out.label_id);
    return 0U;
  }

  /* 只发命令，不在此处阻塞；播完由 APP_LSM_VOICE_WAIT 轮询 idle */
  g_syn6288e.wait_idle = false;
  t0 = HAL_GetTick();
  vst = ScoreVoice_AnnounceInference(&s_voice, out.label_id, out.confidence, points);
  printf("infer: model=%lums voice=%lums action=%s label=%u score=%d total=%ld tts=%d st=0x%02X\r\n",
         (unsigned long)model_ms,
         (unsigned long)(HAL_GetTick() - t0),
         action_name,
         (unsigned)out.label_id,
         (int)points,
         (long)ScoreVoice_GetTotal(&s_voice),
         (int)vst,
         (unsigned)g_syn6288e.last_status);

  return (vst == HAL_OK) ? 1U : 0U;
}

              /*对状态机进行初始化*/
void APP_Init(void)
{
  s_infer_ready = 0U;
  s_voice_wait_t0 = 0U;
  s_state = APP_LSM_SAMPLE;
}

void APP_Run(void)
{
  if (s_infer_ready != 0U)
  {
    ScoreVoice_Process(&s_voice);
  }

  switch (s_state)
  {
    case APP_LSM_SAMPLE:
      run_lpbam_sample();
      s_state = APP_LSM_PARSE;
      break;

    case APP_LSM_PARSE:
      IMU_Traj_LoadFromLpbam();
      s_state = APP_LSM_INFER;
      break;

    case APP_LSM_INFER:
      app_init_infer_stack();
      if (run_inference_and_voice() != 0U)
      {
        s_voice_wait_t0 = HAL_GetTick();
        printf("voice: waiting idle...\r\n");
        s_state = APP_LSM_VOICE_WAIT;
      }
      else
      {
        s_state = APP_LSM_PRINT;
      }
      break;

    case APP_LSM_VOICE_WAIT:
      /* 限频查询，避免每圈狂发 0x21 把芯片打满 */
      HAL_Delay(APP_VOICE_QUERY_GAP_MS);
      if (SYN6288E_IsSpeechDone(&g_syn6288e))
      {
        printf("voice: done (%lums)\r\n",
               (unsigned long)(HAL_GetTick() - s_voice_wait_t0));
        s_state = APP_LSM_PRINT;
      }
      else if ((HAL_GetTick() - s_voice_wait_t0) >= APP_VOICE_WAIT_TIMEOUT_MS)
      {
        printf("voice: timeout st=0x%02X\r\n", (unsigned)g_syn6288e.last_status);
        g_syn6288e.speech_active = false;
        s_state = APP_LSM_PRINT;
      }
      break;

    case APP_LSM_PRINT:
#if APP_ENABLE_CSV
      print_traj_csv();
#endif
      s_state = APP_LSM_SAMPLE;
      break;

    default:
      s_state = APP_LSM_SAMPLE;
      break;
  }
}