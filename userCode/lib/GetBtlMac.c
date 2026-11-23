#include "GetBtlMac.h"

// 全局变量
static char hc05_rx_buffer[HC05_RX_BUFFER_SIZE];
static uint16_t hc05_rx_index = 0;
static volatile uint8_t hc05_rx_complete = 0;
static HC05_HandleTypeDef *hc05_instance = NULL;

/**
  * @brief  HC05初始化
  * @param  hhc05: HC05句柄
  * @param  huart: UART句柄
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_Init(HC05_HandleTypeDef *hhc05, UART_HandleTypeDef *huart)
{
    if (hhc05 == NULL || huart == NULL)
        return HC05_ERROR;
    
    hhc05->huart = huart;
    hhc05->mode = HC05_MODE_NORMAL;
    hhc05->is_connected = 0;
    hhc05->baudrate = 38400;
    strcpy(hhc05->device_name, "HC-05");
    strcpy(hhc05->pin_code, "1234");
    
    // 保存实例指针供中断使用
    hc05_instance = hhc05;
    
    // 启动UART接收中断
    HAL_UART_Receive_IT(huart, (uint8_t*)hc05_rx_buffer, 1);
    
    return HC05_OK;
}

/**
  * @brief  发送AT指令
  * @param  hhc05: HC05句柄
  * @param  command: AT指令
  * @param  response: 响应缓冲区
  * @param  timeout: 超时时间
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_SendATCommand(HC05_HandleTypeDef *hhc05, const char *command, char *response, uint32_t timeout)
{
    char tx_buffer[HC05_TX_BUFFER_SIZE];
    uint32_t start_tick = HAL_GetTick();
    
    if (hhc05 == NULL || hhc05->huart == NULL)
        return HC05_ERROR;
    
    // 准备发送数据
    snprintf(tx_buffer, sizeof(tx_buffer), "%s\r\n", command);
    
    // 重置接收状态
    hc05_rx_index = 0;
    hc05_rx_complete = 0;
    memset(hc05_rx_buffer, 0, sizeof(hc05_rx_buffer));
    
    // 发送命令
    if (HAL_UART_Transmit(hhc05->huart, (uint8_t*)tx_buffer, strlen(tx_buffer), timeout) != HAL_OK)
        return HC05_ERROR;
    
    printf("Sent: %s", tx_buffer);
    
    // 等待响应
    while (!hc05_rx_complete && (HAL_GetTick() - start_tick < timeout))
    {
        // 等待接收完成
    }
    
    if (response != NULL)
    {
        strncpy(response, hc05_rx_buffer, HC05_RX_BUFFER_SIZE - 1);
        response[HC05_RX_BUFFER_SIZE - 1] = '\0';
    }
    
    printf("Received: %s\r\n", hc05_rx_buffer);
    
    return hc05_rx_complete ? HC05_OK : HC05_TIMEOUT;
}

/**
  * @brief  进入AT模式
  * @param  hhc05: HC05句柄
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_EnterATMode(HC05_HandleTypeDef *hhc05)
{
    // 拉高KEY引脚进入AT模式（根据实际硬件连接调整）
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_Delay(100);
    
    hhc05->mode = HC05_MODE_AT;
    return HC05_OK;
}

/**
  * @brief  退出AT模式
  * @param  hhc05: HC05句柄
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_ExitATMode(HC05_HandleTypeDef *hhc05)
{
    // 拉低KEY引脚退出AT模式
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_Delay(100);
    
    hhc05->mode = HC05_MODE_NORMAL;
    return HC05_OK;
}

/**
  * @brief  测试AT指令
  * @param  hhc05: HC05句柄
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_Test(HC05_HandleTypeDef *hhc05)
{
    char response[64];
    return HC05_SendATCommand(hhc05, "AT", response, HC05_TIMEOUT);
}

/**
  * @brief  获取版本信息
  * @param  hhc05: HC05句柄
  * @param  version: 版本信息缓冲区
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_GetVersion(HC05_HandleTypeDef *hhc05, char *version)
{
    return HC05_SendATCommand(hhc05, "AT+VERSION", version, HC05_TIMEOUT);
}

/**
  * @brief  获取蓝牙地址
  * @param  hhc05: HC05句柄
  * @param  address: 地址缓冲区
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_GetAddress(HC05_HandleTypeDef *hhc05, char *address)
{
    return HC05_SendATCommand(hhc05, "AT+ADDR", address, HC05_TIMEOUT);
}

/**
  * @brief  设置设备名称
  * @param  hhc05: HC05句柄
  * @param  name: 设备名称
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_SetName(HC05_HandleTypeDef *hhc05, const char *name)
{
    char command[64];
    snprintf(command, sizeof(command), "AT+NAME=%s", name);
    return HC05_SendATCommand(hhc05, command, NULL, HC05_TIMEOUT);
}

/**
  * @brief  获取设备名称
  * @param  hhc05: HC05句柄
  * @param  name: 名称缓冲区
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_GetName(HC05_HandleTypeDef *hhc05, char *name)
{
    return HC05_SendATCommand(hhc05, "AT+NAME", name, HC05_TIMEOUT);
}

/**
  * @brief  设置PIN码
  * @param  hhc05: HC05句柄
  * @param  pin: PIN码
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_SetPIN(HC05_HandleTypeDef *hhc05, const char *pin)
{
    char command[32];
    snprintf(command, sizeof(command), "AT+PSWD=%s", pin);
    return HC05_SendATCommand(hhc05, command, NULL, HC05_TIMEOUT);
}

/**
  * @brief  获取PIN码
  * @param  hhc05: HC05句柄
  * @param  pin: PIN码缓冲区
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_GetPIN(HC05_HandleTypeDef *hhc05, char *pin)
{
    return HC05_SendATCommand(hhc05, "AT+PSWD", pin, HC05_TIMEOUT);
}

/**
  * @brief  设置波特率
  * @param  hhc05: HC05句柄
  * @param  baudrate: 波特率
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_SetBaudrate(HC05_HandleTypeDef *hhc05, uint32_t baudrate)
{
    char command[32];
    uint8_t baud_code;
    
    // 映射波特率到HC05代码
    switch (baudrate)
    {
        case 1200:   baud_code = 1; break;
        case 2400:   baud_code = 2; break;
        case 4800:   baud_code = 3; break;
        case 9600:   baud_code = 4; break;
        case 19200:  baud_code = 5; break;
        case 38400:  baud_code = 6; break;
        case 57600:  baud_code = 7; break;
        case 115200: baud_code = 8; break;
        default:     baud_code = 4; break; // 默认9600
    }
    
    snprintf(command, sizeof(command), "AT+UART=%lu,0,0", baudrate);
    return HC05_SendATCommand(hhc05, command, NULL, HC05_TIMEOUT);
}

/**
  * @brief  获取波特率
  * @param  hhc05: HC05句柄
  * @param  baudrate: 波特率缓冲区
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_GetBaudrate(HC05_HandleTypeDef *hhc05, char *baudrate)
{
    return HC05_SendATCommand(hhc05, "AT+UART", baudrate, HC05_TIMEOUT);
}

/**
  * @brief  恢复默认设置
  * @param  hhc05: HC05句柄
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_RestoreDefaults(HC05_HandleTypeDef *hhc05)
{
    return HC05_SendATCommand(hhc05, "AT+ORGL", NULL, HC05_TIMEOUT);
}

/**
  * @brief  扫描设备（仅主模式）
  * @param  hhc05: HC05句柄
  * @retval HC05状态
  */
HC05_StatusTypeDef HC05_ScanDevices(HC05_HandleTypeDef *hhc05)
{
    return HC05_SendATCommand(hhc05, "AT+INQM=1,9,48", NULL, HC05_TIMEOUT);
}

/**
  * @brief  UART接收完成中断回调
  * @param  huart: UART句柄
  * @retval None
  */
void HC05_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)  // UART2中断
    {
        if (hc05_rx_index < HC05_RX_BUFFER_SIZE - 1)
        {
            // 检查是否收到完整响应（以\n结尾）
            if (hc05_rx_buffer[0] == '\n' || hc05_rx_buffer[0] == '>')
            {
                hc05_rx_complete = 1;
            }
            else
            {
                hc05_rx_index++;
            }
        }
        else
        {
            // 缓冲区满，标记完成
            hc05_rx_complete = 1;
        }
        
        // 重新启动接收
        if (!hc05_rx_complete)
        {
            HAL_UART_Receive_IT(huart, (uint8_t*)&hc05_rx_buffer[hc05_rx_index], 1);
        }
    }
}

/**
  * @brief  处理接收到的数据
  * @retval None
  */
void HC05_ProcessReceivedData(void)
{
    if (hc05_rx_complete)
    {
        // 这里可以添加数据处理逻辑
        // 例如解析连接状态、接收的数据等
        
        // 重置接收状态
        hc05_rx_index = 0;
        hc05_rx_complete = 0;
        memset(hc05_rx_buffer, 0, sizeof(hc05_rx_buffer));
        
        // 重新启动接收
        if (hc05_instance != NULL && hc05_instance->huart != NULL)
        {
            HAL_UART_Receive_IT(hc05_instance->huart, (uint8_t*)hc05_rx_buffer, 1);
        }
    }
}

/**
  * @brief  打印所有HC05信息
  * @param  hhc05: HC05句柄
  * @retval None
  */
void HC05_PrintAllInfo(HC05_HandleTypeDef *hhc05)
{
    char buffer[128];
    
    printf("\r\n=== HC05 Bluetooth Module Info ===\r\n");
    
    // 测试AT指令
    if (HC05_Test(hhc05) == HC05_OK)
        printf("AT Test: OK\r\n");
    else
        printf("AT Test: FAILED\r\n");
    
    HAL_Delay(200);
    
    // 获取版本
    if (HC05_GetVersion(hhc05, buffer) == HC05_OK)
        printf("Version: %s", buffer);
    else
        printf("Version: FAILED\r\n");
    
    HAL_Delay(200);
    
    // 获取地址
    if (HC05_GetAddress(hhc05, buffer) == HC05_OK)
        printf("Address: %s", buffer);
    else
        printf("Address: FAILED\r\n");
    
    HAL_Delay(200);
    
    // 获取名称
    if (HC05_GetName(hhc05, buffer) == HC05_OK)
        printf("Name: %s", buffer);
    else
        printf("Name: FAILED\r\n");
    
    HAL_Delay(200);
    
    // 获取PIN码
    if (HC05_GetPIN(hhc05, buffer) == HC05_OK)
        printf("PIN: %s", buffer);
    else
        printf("PIN: FAILED\r\n");
    
    HAL_Delay(200);
    
    // 获取波特率
    if (HC05_GetBaudrate(hhc05, buffer) == HC05_OK)
        printf("Baudrate: %s", buffer);
    else
        printf("Baudrate: FAILED\r\n");
    
    printf("=== End of HC05 Info ===\r\n\r\n");
}