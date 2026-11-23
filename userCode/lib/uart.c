//UART.c
unsigned char UART1_Rx_Buf[MAX_REC_LENGTH] = {0}; //USART1存储接收数据
unsigned char UART1_Rx_flg = 0;                   //USART1接收完成标志
unsigned int  UART1_Rx_cnt = 0;                   //USART1接受数据计数器
unsigned char UART1_temp[REC_LENGTH] = {0};       //USART1接收数据缓存

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance==USART1)
  {
    UART1_Rx_Buf[UART1_Rx_cnt] = UART1_temp[0];
    UART1_Rx_cnt++;
    if(0x0a == UART1_temp[0])
    {
      UART1_Rx_flg = 1;
    }
    HAL_UART_Receive_IT(&huart1,(uint8_t *)UART1_temp,REC_LENGTH);
  }
}
