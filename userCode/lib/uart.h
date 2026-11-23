#ifndef __UART_H
#define __UART_H

#ifdef __cplusplus
extern "C" {
#endif
  
#define REC_LENGTH  1
#define MAX_REC_LENGTH  1024 
  
extern unsigned char UART1_Rx_Buf[MAX_REC_LENGTH];
extern unsigned char UART1_Rx_flg ;
extern unsigned int  UART1_Rx_cnt ;
extern unsigned char UART1_temp[REC_LENGTH];
    
#ifdef __cplusplus
}
#endif

#endif 

