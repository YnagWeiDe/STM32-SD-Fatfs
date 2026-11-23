#include "ili9341.h"
#include "main.h"

extern uint16_t Asii0816[96][16];

_lcd_dev lcddev;

static inline void LCD_SPI_WaitForTransmissionComplete(SPI_TypeDef *SPIx)
{
    while (!LL_SPI_IsActiveFlag_TXE(SPIx));
    while (LL_SPI_IsActiveFlag_BSY(SPIx));
}

/**
 * @brief  发送命令到ST7789显示屏
 * @param  cmd: 要发送的命令字节
 * @note   发送命令时，DC引脚需要置低，CS引脚先置低后置高
 * @retval None
 */
void LCD_SendCmd(uint8_t cmd)
{
    LCD_DC_Clr();    // 命令模式
    LCD_CS_Clr();    // 片选使能
    
		    // 使用DMA发送命令
    //HAL_SPI_Transmit_DMA(&hspi2, &cmd, 1);
    //while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY);
	LL_SPI_TransmitData8(SPI2,cmd);
	LCD_SPI_WaitForTransmissionComplete(SPI2);

	
//  HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    
    LCD_CS_Set();    // 片选禁用
}

/**
 * @brief  发送数据到ST7789显示屏
 * @param  data: 要发送的数据字节
 * @note   发送数据时，DC引脚需要置高，CS引脚先置低后置高
 * @retval None
 */
void LCD_SendData(uint8_t data)
{
    LCD_DC_Set();    // 数据模式
    LCD_CS_Clr();    // 片选使能
    
	LL_SPI_TransmitData8(SPI2,data);
  LCD_SPI_WaitForTransmissionComplete(SPI2);
	
//    HAL_SPI_Transmit(&hspi2, &data, 1, HAL_MAX_DELAY);
    
    LCD_CS_Set();    // 片选禁用
}

/**
 * @brief  向LCD写入寄存器和数据
 * @param  reg: 寄存器地址
 * @param  data: 要写入的数据
 * @retval None
 */
void LCD_WriteReg(uint8_t reg, uint16_t data)
{
    LCD_SendCmd(reg);
    LCD_SendData(data);
}

/**
 * @brief  准备向LCD GRAM写入数据
 * @note   在写入像素数据之前需要调用此函数
 * @param  None
 * @retval None
 */
void LCD_WriteRAM_Prepare(void)
{
    LCD_SendCmd(lcddev.wramcmd);
}

/**
 * @brief  向LCD写入16位数据
 * @param  data: 要写入的16位数据
 * @retval None
 */
void LCD_WriteData_16Bit(uint16_t data)
{
    LCD_DC_Set();    // 数据模式
    LCD_CS_Clr();    // 片选使能
	
	    // 将16位数据拆分为高8位和低8位
    uint8_t data_high = (data >> 8) & 0xFF;
    uint8_t data_low = data & 0xFF;
	
		    // 使用LL库发送两个字节
    LL_SPI_TransmitData8(SPI2, data_high);
    LCD_SPI_WaitForTransmissionComplete(SPI2);
    
    LL_SPI_TransmitData8(SPI2, data_low);
    LCD_SPI_WaitForTransmissionComplete(SPI2);
	
	
		
	    // 使用DMA发送两个字节
    //uint8_t buffer[2] = {data_high, data_low};
  //  HAL_SPI_Transmit_DMA(&hspi2, buffer, 2);

    // 等待DMA传输完成
    //while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY);
    
    // 先发送高8位，再发送低8位
//    HAL_SPI_Transmit(&hspi2, (uint8_t*)&data + 1, 1, HAL_MAX_DELAY);
//    HAL_SPI_Transmit(&hspi2, (uint8_t*)&data, 1, HAL_MAX_DELAY);
    
    LCD_CS_Set();    // 片选禁用
}
/**
 * @brief  快速向LCD写入16位数据（不检查传输完成标志）
 * @param  data: 要写入的16位数据
 * @retval None
 */
void LCD_WriteData_16Bit_Fast(uint16_t data)
{
    // 将16位数据拆分为高8位和低8位
    uint8_t data_high = (data >> 8) & 0xFF;
    uint8_t data_low = data & 0xFF;

    // 使用LL库发送两个字节
    LL_SPI_TransmitData8(SPI2, data_high);
    LL_SPI_TransmitData8(SPI2, data_low);
}

void LCD_RESET(void)
{
	LCD_RST_Clr();
	HAL_Delay(50);
	LCD_RST_Set();
	HAL_Delay(50);
}

void LCD_BackLight_On(void){
	HAL_GPIO_WritePin(LCD_LED_GPIO_Port, LCD_LED_Pin, GPIO_PIN_SET);
}

void LCD_BackLight_Off(void){
	HAL_GPIO_WritePin(LCD_LED_GPIO_Port, LCD_LED_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  设置LCD显示窗口
 * @details 设置显示窗口后可以连续发送颜色数据，无需重复设置坐标
 * @param  startX: 窗口起点x轴坐标
 * @param  startY: 窗口起点y轴坐标
 * @param  width:  窗口宽度
 * @param  height: 窗口高度
 * @note   设置显示区域后会自动开启写入模式(RAMWR)
 * @retval None
 */
void LCD_SetWindow(uint16_t startX, uint16_t startY, uint16_t width, uint16_t height)
{
    uint16_t endX = startX + width - 1;
    uint16_t endY = startY + height - 1;
    
    // 设置列地址范围
    LCD_SendCmd(0x2A);        // Column Address Set
    LCD_SendData(startX >> 8);     // 起始列地址高8位
    LCD_SendData(startX & 0xFF);   // 起始列地址低8位
    LCD_SendData(endX >> 8);       // 结束列地址高8位
    LCD_SendData(endX & 0xFF);     // 结束列地址低8位

    // 设置行地址范围
    LCD_SendCmd(0x2B);        // Row Address Set
    LCD_SendData(startY >> 8);     // 起始行地址高8位
    LCD_SendData(startY & 0xFF);   // 起始行地址低8位
    LCD_SendData(endY >> 8);       // 结束行地址高8位
    LCD_SendData(endY & 0xFF);     // 结束行地址低8位
    
    // 开始写入显存
    LCD_SendCmd(0x2C);        // Memory Write
}

/**
 * @brief  设置LCD光标位置
 * @param  x: x坐标
 * @param  y: y坐标
 * @retval None
 */
void LCD_SetCursor(uint16_t x, uint16_t y)
{
    LCD_SetWindow(x, y, 1, 1);
}

/**
 * @brief  使用指定颜色填充整个屏幕
 * @param  color: 16位RGB565格式的颜色值
 * @note   使用连续写入模式快速填充
 * @retval None
 */
void LCD_Clear(uint16_t color)
{
    uint32_t total_pixels = lcddev.width * lcddev.height;
	
	uint8_t color_high = color >> 8;
    uint8_t color_low = color & 0xFF;
    
    // 创建一个缓冲区，用于DMA传输
    uint8_t buffer[2] = {color_high, color_low};
    
    // 设置全屏显示窗口
    LCD_SetWindow(0, 0, lcddev.width-1, lcddev.height-1);
    
    // 连续发送颜色数据
    LCD_DC_Set();    // 数据模式
    LCD_CS_Clr();    // 片选使能
	

	
//    for(uint32_t i = 0; i < total_pixels; i++) {
//        HAL_SPI_Transmit(&hspi2, &color_high, 1, 10);
//        HAL_SPI_Transmit(&hspi2, &color_low, 1, 10);
//    }
	 for(uint32_t i = 0; i < total_pixels; i++) {
        LCD_WriteData_16Bit(color);
    }
	 		 LCD_SPI_WaitForTransmissionComplete(SPI2);
    
		    // 等待最后一次传输完成
    
		
    LCD_CS_Set();    // 片选禁用
}

/**
 * @brief  设置LCD显示方向A
 * @param  direction: 显示方向
 *         0: 0度    (默认竖屏)
 *         1: 90度   (顺时针转90度)
 *         2: 180度  (顺时针转180度)
 *         3: 270度  (顺时针转270度)
 * @note   设置方向的同时会更新LCD设备的宽度和高度参数
 * @retval None
 */
void LCD_SetDirection(uint8_t direction)
{
    // 设置基本的LCD命令
    lcddev.setxcmd = 0x2A;    // 列地址设置命令
    lcddev.setycmd = 0x2B;    // 行地址设置命令
    lcddev.wramcmd = 0x2C;    // 写GRAM命令
    
    switch(direction)
    {
        case 0:     // 0度
            lcddev.width = LCD_WIDTH;
            lcddev.height = LCD_HEIGHT;
            // BGR=1,MY=0,MX=0,MV=0: 正常显示，不翻转，不旋转
            LCD_SendCmd(0x36);
            LCD_SendData((1<<3)|(0<<6)|(0<<7));
            break;
            
        case 1:     // 90度
            lcddev.width = LCD_HEIGHT;
            lcddev.height = LCD_WIDTH;
            // BGR=1,MY=0,MX=1,MV=1: X镜像，行列交换
            LCD_SendCmd(0x36);
            LCD_SendData((1<<3)|(0<<7)|(1<<6)|(1<<5));
            break;
            
        case 2:     // 180度
            lcddev.width = LCD_WIDTH;
            lcddev.height = LCD_HEIGHT;
            // BGR=1,MY=1,MX=1,MV=0: X和Y都镜像
            LCD_SendCmd(0x36);
            LCD_SendData((1<<3)|(1<<6)|(1<<7));
            break;
            
        case 3:     // 270度
            lcddev.width = LCD_HEIGHT;
            lcddev.height = LCD_WIDTH;
            // BGR=1,MY=1,MX=0,MV=1: Y镜像，行列交换
            LCD_SendCmd(0x36);
            LCD_SendData((1<<3)|(1<<7)|(1<<5));
            break;
            
        default:
            break;
    }
}

void LCD_Init(void)
{
	
    // 复位LCD
    LCD_RESET();
    HAL_Delay(100);
 
    // 开始初始化LCD
    LCD_SendCmd(0xCF);

    LCD_SendData(0x00);
    LCD_SendData(0xD9);
    LCD_SendData(0X30);
    
    LCD_SendCmd(0xED);
    LCD_SendData(0x64);
    LCD_SendData(0x03);
    LCD_SendData(0X12);
    LCD_SendData(0X81);
    
    LCD_SendCmd(0xE8);
    LCD_SendData(0x85);
    LCD_SendData(0x10);
    LCD_SendData(0x7A);
    
    LCD_SendCmd(0xCB);
    LCD_SendData(0x39);
    LCD_SendData(0x2C);
    LCD_SendData(0x00);
    LCD_SendData(0x34);
    LCD_SendData(0x02);
    
    LCD_SendCmd(0xF7);
    LCD_SendData(0x20);
    
    LCD_SendCmd(0xEA);
    LCD_SendData(0x00);
    LCD_SendData(0x00);
    
    // Power Control
    LCD_SendCmd(0xC0);
    LCD_SendData(0x1B);
    
    LCD_SendCmd(0xC1);
    LCD_SendData(0x12);
    
    // VCM Control
    LCD_SendCmd(0xC5);
    LCD_SendData(0x08);
    LCD_SendData(0x26);
    
    LCD_SendCmd(0xC7);
    LCD_SendData(0XB7);
    
    // Memory Access Control
    LCD_SendCmd(0x36);
    LCD_SendData(0x08);
    
    LCD_SendCmd(0x3A);
    LCD_SendData(0x55);
    
    LCD_SendCmd(0xB1);
    LCD_SendData(0x00);
    LCD_SendData(0x1A);
    
    // Display Function Control
    LCD_SendCmd(0xB6);
    LCD_SendData(0x0A);
    LCD_SendData(0xA2);
    
    // 3Gamma Function
    LCD_SendCmd(0xF2);
    LCD_SendData(0x00);
    
    LCD_SendCmd(0x26);
    LCD_SendData(0x01);
    
    // Set Gamma
    LCD_SendCmd(0xE0);
    LCD_SendData(0x0F);
    LCD_SendData(0x1D);
    LCD_SendData(0x1A);
    LCD_SendData(0x0A);
    LCD_SendData(0x0D);
    LCD_SendData(0x07);
    LCD_SendData(0x49);
    LCD_SendData(0x66);
    LCD_SendData(0x3B);
    LCD_SendData(0x07);
    LCD_SendData(0x11);
    LCD_SendData(0x01);
    LCD_SendData(0x09);
    LCD_SendData(0x05);
    LCD_SendData(0x04);
    
    LCD_SendCmd(0XE1);
    LCD_SendData(0x00);
    LCD_SendData(0x18);
    LCD_SendData(0x1D);
    LCD_SendData(0x02);
    LCD_SendData(0x0F);
    LCD_SendData(0x04);
    LCD_SendData(0x36);
    LCD_SendData(0x13);
    LCD_SendData(0x4C);
    LCD_SendData(0x07);
    LCD_SendData(0x13);
    LCD_SendData(0x0F);
    LCD_SendData(0x2E);
    LCD_SendData(0x2F);
    LCD_SendData(0x05);
    
    // 设置显示区域
    LCD_SendCmd(0x2B);
    LCD_SendData(0x00);
    LCD_SendData(0x00);
    LCD_SendData(0x01);
    LCD_SendData(0x3F);
    
    LCD_SendCmd(0x2A);
    LCD_SendData(0x00);
    LCD_SendData(0x00);
    LCD_SendData(0x00);
    LCD_SendData(0xEF);
		
    
    // 退出睡眠模式
    LCD_SendCmd(0x11);
    HAL_Delay(120);
    
    // 开启显示
    LCD_SendCmd(0x29);
    
    // 设置显示方向
    LCD_SetDirection(USE_HORIZONTAL);
    
    // 开启背光并清屏
    LCD_BackLight_On();
    LCD_Clear(WHITE);
}


/*************************以下为GUI**********************************************************************************/


/**
 * @brief  在指定位置画点
 * @param  startX: 点的X坐标
 * @param  startY: 点的Y坐标
 * @param  color:  点的颜色(RGB565格式)
 * @note   使用SetWindow函数设置1x1的显示区域后写入颜色数据
 * @retval None
 */
void LCD_DrawPoint(uint16_t startX, uint16_t startY, uint16_t color)
{
	// 设置1x1的显示窗口
	LCD_SetWindow(startX, startY, 1, 1);
	LCD_WriteData_16Bit(color);
}


/*******************************************************************
 * @name       :画线
 * @function   :Draw a line between two points
 * @parameters :	x1:the bebinning x coordinate of the line
				y1:the bebinning y coordinate of the line
				x2:the ending x coordinate of the line
				y2:the ending y coordinate of the line
 * @retvalue   :None
********************************************************************/
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	uint16_t t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 

	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //设置单步方向 
	else if(delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//水平线 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//画线输出 
	{  
		LCD_DrawPoint(uRow,uCol, color);//画点 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
} 

/**
 * @brief  在指定位置画实心圆
 * @param  centerX: 圆心的X坐标
 * @param  centerY: 圆心的Y坐标
 * @param  radius:  圆的半径
 * @param  color:   圆的颜色(RGB565格式)
 * @retval None
 */
void LCD_DrawFilledCircle(uint16_t centerX, uint16_t centerY, uint16_t radius, uint16_t color)
{
    int16_t x = 0;
    int16_t y = radius;
    int16_t d = 1 - radius;
    int16_t deltaE = 3;
    int16_t deltaSE = -2 * radius + 5;

    // 画初始的八个点
    LCD_DrawLine(centerX - radius, centerY, centerX + radius, centerY, color);

    while (y > x)
    {
        if (d < 0) // 选择东点
        {
            d += deltaE;
            deltaE += 2;
            deltaSE += 2;
        }
        else // 选择东南点
        {
            d += deltaSE;
            deltaE += 2;
            deltaSE += 4;
            y--;
        }
        x++;

        // 画对称的八个点
        LCD_DrawLine(centerX - x, centerY + y, centerX + x, centerY + y, color);
        LCD_DrawLine(centerX - x, centerY - y, centerX + x, centerY - y, color);
        LCD_DrawLine(centerX - y, centerY + x, centerX + y, centerY + x, color);
        LCD_DrawLine(centerX - y, centerY - x, centerX + y, centerY - x, color);
				HAL_Delay(50);
    }
}

void LCD_ShowChar(uint16_t x,uint16_t y,char ch,uint16_t color,uint16_t backgroundColor,uint8_t size,uint8_t mode){

	uint8_t temp;
	uint8_t pos,t;
	const uint16_t *pfont;
	
	if(x>lcddev.width || y> lcddev.height) return;
	
	if((ch<32)||(ch>126)) return;
	
	if(size != FONT_SIZE_SMALL)return;
	
	pfont = Asii0816[ch-32];
	
	LCD_SetWindow(x,y,8,16);
	LCD_WriteRAM_Prepare();
	
	for(pos = 0;pos <16;pos++){
		temp = pfont[pos] &0xff;//获取点阵数据
		
		for(t = 0;t<8;t++){
			if(temp &(0x80 >> t)){
				LCD_WriteData_16Bit(color);
			}else{
				if(mode == FONT_OPAQUE){
					LCD_WriteData_16Bit(backgroundColor);
				}else{
					// 透明模式，不绘制背景
					// 这里需要根据实际需求处理，可能需要先读取屏幕当前颜色
				}
			}
		}
	}
}

void LCD_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t backgroundColor, uint8_t size, uint8_t mode){
	
	uint16_t x0 = x;
	
	while(*str != '\0'){
		if(*str == '\n'){//有溢出的可能
			y+=size;
			x = x0;
		}
		else if(*str == '\r'){//回车无视
		
		}else{
			LCD_ShowChar(x,y,*str,color,backgroundColor,size,mode);
			x+=8;//移动
		}
		
		if(x+8>lcddev.width){
			x=x0;
			y+=size;
		}
		str++;
	}

}

void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint16_t color, uint16_t backgroundColor, uint8_t size, uint8_t mode){
    uint8_t t, temp;
    uint8_t enshow = 0;
    char str[12];
    
    // 将数字转换为字符串
    for(t = 0; t < len; t++)
    {
        temp = (num / LCD_Pow(10, len - t - 1)) % 10;//计算10的幂
        
        if(enshow == 0 && t < (len - 1))
        {
            if(temp == 0)
            {
                str[t] = ' ';
                continue;
            }
            else enshow = 1;
        }
        
        str[t] = temp + '0';
    }
    
    str[len] = '\0';
    
    // 显示数字字符串
    LCD_ShowString(x, y, str, color, backgroundColor, size, mode);
}

void LCD_ShowSignedNum(uint16_t x, uint16_t y, int32_t num, uint8_t len, uint16_t color, uint16_t backgroundColor, uint8_t size, uint8_t mode)
{
    if(num >= 0)
    {
        LCD_ShowChar(x, y, '+', color, backgroundColor, size, mode);
        LCD_ShowNum(x + 8, y, num, len, color, backgroundColor, size, mode);
    }
    else
    {
        LCD_ShowChar(x, y, '-', color, backgroundColor, size, mode);
        LCD_ShowNum(x + 8, y, -num, len, color, backgroundColor, size, mode);
    }
}

void LCD_ShowFloatNum(uint16_t x, uint16_t y, float num, uint8_t int_len, uint8_t frac_len, uint16_t color, uint16_t backgroundColor, uint8_t size, uint8_t mode)
{
    uint32_t int_part = (uint32_t)num;
    uint32_t frac_part = (uint32_t)((num - int_part) * LCD_Pow(10, frac_len));
    
    // 显示整数部分
    LCD_ShowNum(x, y, int_part, int_len, color, backgroundColor, size, mode);
    x += int_len * 8;
    
    // 显示小数点
    LCD_ShowChar(x, y, '.', color, backgroundColor, size, mode);
    x += 8;
    
    // 显示小数部分
    LCD_ShowNum(x, y, frac_part, frac_len, color, backgroundColor, size, mode);
}

void LCD_ShowHexNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint16_t color, uint16_t backgroundColor, uint8_t size, uint8_t mode)
{
    uint8_t t, temp;
    char str[10];
    
    // 添加"0x"前缀
    LCD_ShowString(x, y, "0x", color, backgroundColor, size, mode);
    x += 16;
    
    // 将十六进制数字转换为字符串
    for(t = 0; t < len; t++)
    {
        temp = (num >> (4 * (len - t - 1))) & 0x0F;
        
        if(temp < 10)
            str[t] = temp + '0';
        else
            str[t] = temp - 10 + 'A';
    }
    
    str[len] = '\0';
    
    // 显示十六进制数字字符串
    LCD_ShowString(x, y, str, color, backgroundColor, size, mode);
}




uint32_t LCD_Pow(uint32_t m, uint32_t n)
{
    uint32_t result = 1;
    
    while(n--)
    {
        result *= m;
    }
    
    return result;
}

