/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "fatfs.h"
#include "i2s.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "I2SD.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
uint16_t DAC_SIGNAL_Sinus[32] = {
2047, 2447, 2831, 3185, 3498, 3750, 3939, 4056,
4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
2047, 1647, 1263, 909, 599, 344, 155, 38,
0, 38, 155, 344, 599, 909, 1263, 1647
};

uint16_t DAC_SIGNAL_Triangle[32] = {
0, 256, 512, 768, 1024, 1279, 1535, 1791,
2047, 2303, 2559, 2815, 3071, 3326, 3582, 3838,
4095, 3838, 3582, 3326, 3071, 2815, 2559, 2303,
2047, 1791, 1535, 1279, 1024, 768, 512, 256
};

uint16_t DAC_SIGNAL_Sawtooth[32] = {
0, 132, 264, 396, 528, 660, 792, 924,
1057, 1189, 1321, 1453, 1585, 1717, 1849, 1981,
2113, 2245, 2377, 2509, 2641, 2773, 2905, 3037,
3170, 3302, 3434, 3566, 3698, 3830, 3962, 4095
};

uint16_t DAC_SIGNAL_Square[32] = {
0, 4095
};

I2SD_HandleTypeDef hi2sd;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// I2SD 初始化
// 文件系统对象
//FATFS fs;          // FatFS 工作区
//FIL file;          // 文件对象
//FRESULT res;       // 操作结果
//UINT bytesRead;    // 实际读取字节数
//uint8_t buffer[512]; // 用于存储读取的音频数据（每次读取512字节）

//// SD 卡初始化和挂载文件系统
//void SD_Card_Init(void) {
//    res = f_mount(&fs, "", 1);  // 挂载 SD 卡到根目录（""）
//    if (res != FR_OK) {
//        printf("FatFS mount failed with error %d\r\n", res);
//        return;
//    }
//    printf("SD card mounted successfully\r\n");

//    // 打开文件（读取模式）
//    res = f_open(&file, "0:/ZARD1.wav", FA_READ);  // 假设文件在 SD 卡根目录
//    if (res != FR_OK) {
//        printf("Failed to open file: %d\r\n", res);
//        return;
//    }
//    printf("File opened successfully\r\n");
//}

//// 读取文件并播放音频
//void Read_File_And_Play(void) {
//    // 不断从文件中读取数据并播放

//  
//        res = f_read(&file, buffer, sizeof(buffer), &bytesRead);  // 读取 512 字节
//        if (res != FR_OK || bytesRead == 0) {
//            // 文件读取结束或读取失败，停止播放
//            printf("File read complete or error occurred\r\n");
////            break;
//        }
//					// 打印读取的前256字节（调试用）
//					printf("First 256 bytes of the file:\r\n");
//					for (UINT i = 0; i < 256; i++) {
//							printf("0x%02X ", buffer[i]);
//							if ((i + 1) % 16 == 0) {
//									printf("\r\n");
//							}
//					}
//					 HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t*)buffer, bytesRead );  // 16-bit 数据，需要转为 half-word 格式

//        // 将读取的音频数据发送到 I2S 播放
//       

//        // 这里可以加入一些延时（例如通过定时器/软件延时）来确保缓冲区填充完成再继续下一次读取
//        // 以免数据溢出。通常，DMA 会在传输完一个缓冲区后触发中断。

//        // 检查 DMA 是否完成上一轮传输，如果已完成，再进行下一轮读取填充
//        // 可以通过设置标志位（如 halfEmpty/fullEmpty）来控制
//    
//}

//// 停止和卸载文件系统
//void SD_Card_DeInit(void) {
//    f_close(&file);  // 关闭文件
//    f_mount(NULL, "", 1);  // 卸载文件系统
//    printf("SD card deinitialized and file closed\r\n");
//}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2S2_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  MX_FATFS_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
	I2SD_Init(&hi2sd);
	I2SD_PlayWav(&hi2sd, "ZARD2.wav");
	while(1){
		I2SD_Process(&hi2sd);
		printf("Is ok");
		HAL_Delay(1000);
	}

	
//    // 初始化 SD 卡并挂载 FatFS
//    SD_Card_Init();

//    // 读取并播放文件
//    Read_File_And_Play();

//    // 停止和卸载 SD 卡
//    SD_Card_Det();2
//		Get_SDCard_Capacity();
//		ReadFromSD("0:/ZARD.wav",4096*4);
//	printf("1");
//	
//	HAL_I2S_Transmit_DMA(&hi2s2,DAC_SIGNAL_Sinus,32);
//	printf("2");
//	Get_SDCard_Capacity();


//	I2SD_InitExample();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
        // 处理 I2SD 播放逻辑
//        I2SD_Process(&hi2sd);  // 持续更新缓冲区并播放音频
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
