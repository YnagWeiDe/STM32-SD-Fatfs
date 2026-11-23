#ifndef HC05_H
#define HC05_H

#include "main.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"

// HC05状态定义
typedef enum {
    HC05_OK = 0,
    HC05_ERROR,
    HC05_TIMEOUT,
    HC05_BUSY
} HC05_StatusTypeDef;

// HC05工作模式
typedef enum {
    HC05_MODE_NORMAL = 0,
    HC05_MODE_AT
} HC05_ModeTypeDef;

// HC05结构体
typedef struct {
    UART_HandleTypeDef *huart;
    HC05_ModeTypeDef mode;
    uint8_t is_connected;
    char device_name[32];
    char pin_code[8];
    uint32_t baudrate;
} HC05_HandleTypeDef;

// 接收缓冲区
#define HC05_RX_BUFFER_SIZE 256
#define HC05_TX_BUFFER_SIZE 128
#define HC05_TIMEOUT 1000

// 函数声明
HC05_StatusTypeDef HC05_Init(HC05_HandleTypeDef *hhc05, UART_HandleTypeDef *huart);
HC05_StatusTypeDef HC05_SendATCommand(HC05_HandleTypeDef *hhc05, const char *command, char *response, uint32_t timeout);
HC05_StatusTypeDef HC05_EnterATMode(HC05_HandleTypeDef *hhc05);
HC05_StatusTypeDef HC05_ExitATMode(HC05_HandleTypeDef *hhc05);
HC05_StatusTypeDef HC05_Test(HC05_HandleTypeDef *hhc05);
HC05_StatusTypeDef HC05_GetVersion(HC05_HandleTypeDef *hhc05, char *version);
HC05_StatusTypeDef HC05_GetAddress(HC05_HandleTypeDef *hhc05, char *address);
HC05_StatusTypeDef HC05_SetName(HC05_HandleTypeDef *hhc05, const char *name);
HC05_StatusTypeDef HC05_GetName(HC05_HandleTypeDef *hhc05, char *name);
HC05_StatusTypeDef HC05_SetPIN(HC05_HandleTypeDef *hhc05, const char *pin);
HC05_StatusTypeDef HC05_GetPIN(HC05_HandleTypeDef *hhc05, char *pin);
HC05_StatusTypeDef HC05_SetBaudrate(HC05_HandleTypeDef *hhc05, uint32_t baudrate);
HC05_StatusTypeDef HC05_GetBaudrate(HC05_HandleTypeDef *hhc05, char *baudrate);
HC05_StatusTypeDef HC05_RestoreDefaults(HC05_HandleTypeDef *hhc05);
HC05_StatusTypeDef HC05_ScanDevices(HC05_HandleTypeDef *hhc05);

// 中断回调函数
void HC05_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HC05_ProcessReceivedData(void);

// 调试函数
void HC05_PrintAllInfo(HC05_HandleTypeDef *hhc05);

#endif /* HC05_H */