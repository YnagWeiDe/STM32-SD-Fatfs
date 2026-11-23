#include "SDFaths.h"
#include "main.h"
#include "stdio.h"

#define SD_CS_GPIO_Port GPIOA
#define SD_CS_Pin GPIO_PIN_4
#define SPI hspi1

uint8_t DFF=0xFF;
uint8_t test;
uint8_t SD_TYPE=0x00;

#define MAX  512
uint8_t work[MAX];


MSD_CARDINFO SD0_CardInfo;

//////////////////////////////////////////////////////////////
//片选
//////////////////////////////////////////////////////////////
void SD_CS(uint8_t p){
	if(p==0){	
		HAL_GPIO_WritePin(SD_CS_GPIO_Port,SD_CS_Pin,GPIO_PIN_SET);
		}else{
		HAL_GPIO_WritePin(SD_CS_GPIO_Port,SD_CS_Pin,GPIO_PIN_RESET);
		}
}
///////////////////////////////////////////////////////////////
//发送命令，发完释放
//////////////////////////////////////////////////////////////
int SD_sendcmd(uint8_t cmd,uint32_t arg,uint8_t crc){
	uint8_t r1;
  uint8_t retry;

  SD_CS(0);
	HAL_Delay(20);
  SD_CS(1);
	do{
		retry=spi_readwrite(DFF);
	}while(retry!=0xFF);

  spi_readwrite(cmd | 0x40);
  spi_readwrite(arg >> 24);
  spi_readwrite(arg >> 16);
  spi_readwrite(arg >> 8);
  spi_readwrite(arg);
  spi_readwrite(crc);
  if(cmd==CMD12)spi_readwrite(DFF);
  do
	{
		r1=spi_readwrite(0xFF);
	}while(r1&0X80);
	
	return r1;
}


/////////////////////////////////////////////////////////////
//SD卡初始化
////////////////////////////////////////////////////////////
uint8_t SD_init(void)
{
	uint8_t r1;	
	uint8_t buff[6] = {0};
	uint16_t retry; 
	uint8_t i;
	
//	MX_SPI3_Init();
	SPI_setspeed(SPI_BAUDRATEPRESCALER_256);
	SD_CS(0);
	for(retry=0;retry<10;retry++){
			spi_readwrite(DFF);
	}
	
	//SD卡进入IDLE状态
	do{
		r1 = SD_sendcmd(CMD0 ,0, 0x95);	
	}while(r1!=0x01);
	
	//查看SD卡的类型
	SD_TYPE=0;
	r1 = SD_sendcmd(CMD8, 0x1AA, 0x87);
	if(r1==0x01){
		for(i=0;i<4;i++)buff[i]=spi_readwrite(DFF);	//Get trailing return value of R7 resp
		if(buff[2]==0X01&&buff[3]==0XAA)//卡是否支持2.7~3.6V
		{
			retry=0XFFFE;
			do
			{
				SD_sendcmd(CMD55,0,0X01);	//发送CMD55
				r1=SD_sendcmd(CMD41,0x40000000,0X01);//发送CMD41
			}while(r1&&retry--);
			if(retry&&SD_sendcmd(CMD58,0,0X01)==0)//鉴别SD2.0卡版本开始
			{
				for(i=0;i<4;i++)buff[i]=spi_readwrite(0XFF);//得到OCR值
				if(buff[0]&0x40){
					SD_TYPE=V2HC;
				}else {
					SD_TYPE=V2;
				}						
			}
		}else{
			SD_sendcmd(CMD55,0,0X01);			//发送CMD55
			r1=SD_sendcmd(CMD41,0,0X01);	//发送CMD41
			if(r1<=1)
			{		
				SD_TYPE=V1;
				retry=0XFFFE;
				do //等待退出IDLE模式
				{
					SD_sendcmd(CMD55,0,0X01);	//发送CMD55
					r1=SD_sendcmd(CMD41,0,0X01);//发送CMD41
				}while(r1&&retry--);
			}else//MMC卡不支持CMD55+CMD41识别
			{
				SD_TYPE=MMC;//MMC V3
				retry=0XFFFE;
				do //等待退出IDLE模式
				{											    
					r1=SD_sendcmd(CMD1,0,0X01);//发送CMD1
				}while(r1&&retry--);  
			}
			if(retry==0||SD_sendcmd(CMD16,512,0X01)!=0)SD_TYPE=ERR;//错误的卡
		}
	}
	SD_CS(0);
	SPI_setspeed(SPI_BAUDRATEPRESCALER_2);
	if(SD_TYPE)return 0;
	else return 1;
}


//读取指定长度数据
uint8_t SD_ReceiveData(uint8_t *data, uint16_t len)
{
    uint8_t r1;
    uint32_t timeout = 10000;  // ← 新增：超时计数（约 10ms @ 10MHz SPI，调小/大根据你的时钟速）

    SD_CS(1);									   
    
    // 等待数据令牌 0xFE（忙等版，无延时）
    do {
        r1 = spi_readwrite(0xFF);
        if (--timeout == 0) {  // ← 新增：超时检查
            printf("SD_ReceiveData TIMEOUT! Last r1=0x%02X, len=%d\r\n", r1, len);  // ← 调试：打印最后 r1 + len，帮助定位
            SD_CS(0);  // ← 安全退出：拉高 CS
            return 1;  // ← 错误返回，让 FatFS 知晓（重试或失败）
        }
    } while (r1 != 0xFE);

    // 收到令牌后，读数据（原逻辑不变）
    while (len--) {
        *data = spi_readwrite(0xFF);
        data++;
    }
    
    // CRC 忽略（原逻辑）
    spi_readwrite(0xFF);
    spi_readwrite(0xFF); 										  		
    
    SD_CS(0);  // ← 补充：显式取消片选（原代码漏了？）
    return 0;  // 成功
}
//向sd卡写入一个数据包的内容 512字节
uint8_t SD_SendBlock(uint8_t*buf,uint8_t cmd)
{	
	uint16_t t;	
uint8_t r1;	
	do{
		r1=spi_readwrite(0xFF);
	}while(r1!=0xFF);
	
	spi_readwrite(cmd);
	if(cmd!=0XFD)//不是结束指令
	{
		for(t=0;t<512;t++)spi_readwrite(buf[t]);//提高速度,减少函数传参时间
	    spi_readwrite(0xFF);//忽略crc
	    spi_readwrite(0xFF);
		t=spi_readwrite(0xFF);//接收响应
		if((t&0x1F)!=0x05)return 2;//响应错误									  					    
	}						 									  					    
    return 0;//写入成功
}

//获取CID信息
uint8_t SD_GETCID (uint8_t *cid_data)
{
		uint8_t r1;
	  r1=SD_sendcmd(CMD10,0,0x01); //读取CID寄存器
		if(r1==0x00){
			r1=SD_ReceiveData(cid_data,16);
		}
		SD_CS(0);
		if(r1)return 1;
		else return 0;
}
//获取CSD信息
uint8_t SD_GETCSD(uint8_t *csd_data){
		uint8_t r1;	 
    r1=SD_sendcmd(CMD9,0,0x01);//发CMD9命令，读CSD寄存器
    if(r1==0)
	{
    	r1=SD_ReceiveData(csd_data, 16);//接收16个字节的数据 
    }
	SD_CS(0);//取消片选
	if(r1)return 1;
	else return 0;
}
//获取SD卡的总扇区数
uint32_t SD_GetSectorCount(void)
{
    uint8_t csd[16];
    uint32_t Capacity;  
    uint8_t n;
		uint16_t csize;  					    
	//取CSD信息，如果期间出错，返回0
    if(SD_GETCSD(csd)!=0) return 0;	    
    //如果为SDHC卡，按照下面方式计算
    if((csd[0]&0xC0)==0x40)	 //V2.00的卡
    {	
		csize = csd[9] + ((uint16_t)csd[8] << 8) + 1;
		Capacity = (uint32_t)csize << 10;//得到扇区数	 		   
    }else//V1.XX的卡
    {	
		n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
		csize = (csd[8] >> 6) + ((uint16_t)csd[7] << 2) + ((uint16_t)(csd[6] & 3) << 10) + 1;
		Capacity= (uint32_t)csize << (n - 9);//得到扇区数   
    }
    return Capacity;
}
int MSD0_GetCardInfo(PMSD_CARDINFO SD0_CardInfo)
{
  uint8_t r1;
  uint8_t CSD_Tab[16];
  uint8_t CID_Tab[16];

  /* Send CMD9, Read CSD */
  r1 = SD_sendcmd(CMD9, 0, 0xFF);
  if(r1 != 0x00)
  {
    return r1;
  }

  if(SD_ReceiveData(CSD_Tab, 16))
  {
	return 1;
  }

  /* Send CMD10, Read CID */
  r1 = SD_sendcmd(CMD10, 0, 0xFF);
  if(r1 != 0x00)
  {
    return r1;
  }

  if(SD_ReceiveData(CID_Tab, 16))
  {
	return 2;
  }  
  /* Byte 0 */
  SD0_CardInfo->CSD.CSDStruct = (CSD_Tab[0] & 0xC0) >> 6;
  SD0_CardInfo->CSD.SysSpecVersion = (CSD_Tab[0] & 0x3C) >> 2;
  SD0_CardInfo->CSD.Reserved1 = CSD_Tab[0] & 0x03;
  /* Byte 1 */
  SD0_CardInfo->CSD.TAAC = CSD_Tab[1] ;
  /* Byte 2 */
  SD0_CardInfo->CSD.NSAC = CSD_Tab[2];
  /* Byte 3 */
  SD0_CardInfo->CSD.MaxBusClkFrec = CSD_Tab[3];
  /* Byte 4 */
  SD0_CardInfo->CSD.CardComdClasses = CSD_Tab[4] << 4;
  /* Byte 5 */
  SD0_CardInfo->CSD.CardComdClasses |= (CSD_Tab[5] & 0xF0) >> 4;
  SD0_CardInfo->CSD.RdBlockLen = CSD_Tab[5] & 0x0F;
  /* Byte 6 */
  SD0_CardInfo->CSD.PartBlockRead = (CSD_Tab[6] & 0x80) >> 7;
  SD0_CardInfo->CSD.WrBlockMisalign = (CSD_Tab[6] & 0x40) >> 6;
  SD0_CardInfo->CSD.RdBlockMisalign = (CSD_Tab[6] & 0x20) >> 5;
  SD0_CardInfo->CSD.DSRImpl = (CSD_Tab[6] & 0x10) >> 4;
  SD0_CardInfo->CSD.Reserved2 = 0; /* Reserved */
  SD0_CardInfo->CSD.DeviceSize = (CSD_Tab[6] & 0x03) << 10;
  /* Byte 7 */
  SD0_CardInfo->CSD.DeviceSize |= (CSD_Tab[7]) << 2;
  /* Byte 8 */
  SD0_CardInfo->CSD.DeviceSize |= (CSD_Tab[8] & 0xC0) >> 6;
  SD0_CardInfo->CSD.MaxRdCurrentVDDMin = (CSD_Tab[8] & 0x38) >> 3;
  SD0_CardInfo->CSD.MaxRdCurrentVDDMax = (CSD_Tab[8] & 0x07);
  /* Byte 9 */
  SD0_CardInfo->CSD.MaxWrCurrentVDDMin = (CSD_Tab[9] & 0xE0) >> 5;
  SD0_CardInfo->CSD.MaxWrCurrentVDDMax = (CSD_Tab[9] & 0x1C) >> 2;
  SD0_CardInfo->CSD.DeviceSizeMul = (CSD_Tab[9] & 0x03) << 1;
  /* Byte 10 */
  SD0_CardInfo->CSD.DeviceSizeMul |= (CSD_Tab[10] & 0x80) >> 7;
  SD0_CardInfo->CSD.EraseGrSize = (CSD_Tab[10] & 0x7C) >> 2;
  SD0_CardInfo->CSD.EraseGrMul = (CSD_Tab[10] & 0x03) << 3;
  /* Byte 11 */
  SD0_CardInfo->CSD.EraseGrMul |= (CSD_Tab[11] & 0xE0) >> 5;
  SD0_CardInfo->CSD.WrProtectGrSize = (CSD_Tab[11] & 0x1F);
  /* Byte 12 */
  SD0_CardInfo->CSD.WrProtectGrEnable = (CSD_Tab[12] & 0x80) >> 7;
  SD0_CardInfo->CSD.ManDeflECC = (CSD_Tab[12] & 0x60) >> 5;
  SD0_CardInfo->CSD.WrSpeedFact = (CSD_Tab[12] & 0x1C) >> 2;
  SD0_CardInfo->CSD.MaxWrBlockLen = (CSD_Tab[12] & 0x03) << 2;
  /* Byte 13 */
  SD0_CardInfo->CSD.MaxWrBlockLen |= (CSD_Tab[13] & 0xc0) >> 6;
  SD0_CardInfo->CSD.WriteBlockPaPartial = (CSD_Tab[13] & 0x20) >> 5;
  SD0_CardInfo->CSD.Reserved3 = 0;
  SD0_CardInfo->CSD.ContentProtectAppli = (CSD_Tab[13] & 0x01);
  /* Byte 14 */
  SD0_CardInfo->CSD.FileFormatGrouop = (CSD_Tab[14] & 0x80) >> 7;
  SD0_CardInfo->CSD.CopyFlag = (CSD_Tab[14] & 0x40) >> 6;
  SD0_CardInfo->CSD.PermWrProtect = (CSD_Tab[14] & 0x20) >> 5;
  SD0_CardInfo->CSD.TempWrProtect = (CSD_Tab[14] & 0x10) >> 4;
  SD0_CardInfo->CSD.FileFormat = (CSD_Tab[14] & 0x0C) >> 2;
  SD0_CardInfo->CSD.ECC = (CSD_Tab[14] & 0x03);
  /* Byte 15 */
  SD0_CardInfo->CSD.CSD_CRC = (CSD_Tab[15] & 0xFE) >> 1;
  SD0_CardInfo->CSD.Reserved4 = 1;

  if(SD0_CardInfo->CardType == V2HC)
  {
	 /* Byte 7 */
	 SD0_CardInfo->CSD.DeviceSize = (uint16_t)(CSD_Tab[8]) *256;
	 /* Byte 8 */
	 SD0_CardInfo->CSD.DeviceSize += CSD_Tab[9] ;
  }

  SD0_CardInfo->Capacity = SD0_CardInfo->CSD.DeviceSize * MSD_BLOCKSIZE * 1024;
  SD0_CardInfo->BlockSize = MSD_BLOCKSIZE;

  /* Byte 0 */
  SD0_CardInfo->CID.ManufacturerID = CID_Tab[0];
  /* Byte 1 */
  SD0_CardInfo->CID.OEM_AppliID = CID_Tab[1] << 8;
  /* Byte 2 */
  SD0_CardInfo->CID.OEM_AppliID |= CID_Tab[2];
  /* Byte 3 */
  SD0_CardInfo->CID.ProdName1 = CID_Tab[3] << 24;
  /* Byte 4 */
  SD0_CardInfo->CID.ProdName1 |= CID_Tab[4] << 16;
  /* Byte 5 */
  SD0_CardInfo->CID.ProdName1 |= CID_Tab[5] << 8;
  /* Byte 6 */
  SD0_CardInfo->CID.ProdName1 |= CID_Tab[6];
  /* Byte 7 */
  SD0_CardInfo->CID.ProdName2 = CID_Tab[7];
  /* Byte 8 */
  SD0_CardInfo->CID.ProdRev = CID_Tab[8];
  /* Byte 9 */
  SD0_CardInfo->CID.ProdSN = CID_Tab[9] << 24;
  /* Byte 10 */
  SD0_CardInfo->CID.ProdSN |= CID_Tab[10] << 16;
  /* Byte 11 */
  SD0_CardInfo->CID.ProdSN |= CID_Tab[11] << 8;
  /* Byte 12 */
  SD0_CardInfo->CID.ProdSN |= CID_Tab[12];
  /* Byte 13 */
  SD0_CardInfo->CID.Reserved1 |= (CID_Tab[13] & 0xF0) >> 4;
  /* Byte 14 */
  SD0_CardInfo->CID.ManufactDate = (CID_Tab[13] & 0x0F) << 8;
  /* Byte 15 */
  SD0_CardInfo->CID.ManufactDate |= CID_Tab[14];
  /* Byte 16 */
  SD0_CardInfo->CID.CID_CRC = (CID_Tab[15] & 0xFE) >> 1;
  SD0_CardInfo->CID.Reserved2 = 1;

  return 0;  
}


//写SD卡
//buf:数据缓存区
//sector:起始扇区
//cnt:扇区数
//返回值:0,ok;其他,失败.
uint8_t SD_WriteDisk(uint8_t*buf,uint32_t sector,uint8_t cnt)
{
	uint8_t r1;
	if(SD_TYPE!=V2HC)sector *= 512;//转换为字节地址
	if(cnt==1)
	{
		r1=SD_sendcmd(CMD24,sector,0X01);//读命令
		if(r1==0)//指令发送成功
		{
			r1=SD_SendBlock(buf,0xFE);//写512个字节	   
		}
	}else
	{
		if(SD_TYPE!=MMC)
		{
			SD_sendcmd(CMD55,0,0X01);	
			SD_sendcmd(CMD23,cnt,0X01);//发送指令	
		}
 		r1=SD_sendcmd(CMD25,sector,0X01);//连续读命令
		if(r1==0)
		{
			do
			{
				r1=SD_SendBlock(buf,0xFC);//接收512个字节	 
				buf+=512;  
			}while(--cnt && r1==0);
			r1=SD_SendBlock(0,0xFD);//接收512个字节 
		}
	}   
	SD_CS(0);//取消片选
	return r1;//
}	
//读SD卡
//buf:数据缓存区
//sector:扇区
//cnt:扇区数
//返回值:0,ok;其他,失败.
uint8_t SD_ReadDisk(uint8_t*buf,uint32_t sector,uint8_t cnt)
{
    uint8_t r1;
    if(SD_TYPE!=V2HC)sector <<= 9;//转换为字节地址
    
    // 临时改：不管 cnt 多大，都用单扇区 CMD17 循环读（避免 CMD18 卡）
    for (uint8_t i = 0; i < cnt; i++) {
        r1=SD_sendcmd(CMD17, sector + i, 0x01);//单扇区读命令
        if(r1!=0){  // 命令失败
            printf("CMD17 fail at sector %d\r\n", sector + i);
            return r1;
        }
        r1=SD_ReceiveData(buf + i*512, 512);//接收512个字节
        if(r1!=0){
            printf("Receive fail at sector %d\r\n", sector + i);
            return r1;
        }
        HAL_Delay(1);  // 加小延时，让 SD 卡喘口气
    }
    
    SD_CS(0);//取消片选
    return 0;
}




uint8_t spi_readwrite(uint8_t Txdata){
	uint8_t Rxdata;	
	HAL_SPI_TransmitReceive(&hspi1,&Txdata,&Rxdata,1,100);
	return Rxdata;
}
//SPI1波特率设置
void SPI_setspeed(uint8_t speed){
	hspi1.Init.BaudRatePrescaler = speed;
}


void Get_SDCard_Capacity(void)
{
	FRESULT result;
	FATFS FS;
	FATFS *fs;
	DWORD fre_clust,AvailableSize,UsedSize;  
	uint16_t TotalSpace;
	uint8_t res;
	
	res = SD_init();		//SD卡初始化
	if(res == 1)
	{
		printf("SD卡初始化失败! \r\n");		
	}
	else
	{
		printf("SD卡初始化成功！ \r\n");		
	}
	
	/* 挂载 */
	res=f_mount(&FS,"0:",1);		//挂载
	if (res != FR_OK)
	{
		printf("FileSystem Mounted Failed (%d)\r\n", result);
	}

	res = f_getfree("0:", &fre_clust, &fs);  /* 根目录 */
	if ( res == FR_OK ) 
	{
		TotalSpace=(uint16_t)(((fs->n_fatent - 2) * fs->csize ) / 2 /1024);
		AvailableSize=(uint16_t)((fre_clust * fs->csize) / 2 /1024);
		UsedSize=TotalSpace-AvailableSize;              
		/* Print free space in unit of MB (assuming 512 bytes/sector) */
		printf("\r\n%d MB total drive space.\r\n""%lu MB available.\r\n""%lu MB  used.\r\n",TotalSpace, AvailableSize,UsedSize);
	}
	else 
	{
		printf("Get SDCard Capacity Failed (%d)\r\n", result);
	}		
} 


void WritetoSD(char filename[], BYTE write_buff[], uint8_t bufSize)
{
	FATFS fs;
	FIL file;
	uint8_t res=0;
	UINT Bw;	

	res = SD_init();		//SD卡初始化
	
	if(res == 1)
	{
		printf("SD卡初始化失败! \r\n");		
	}
	else
	{
		printf("SD卡初始化成功！ \r\n");		
	}
	
	res=f_mount(&fs,"0:",1);		//挂载
	
//	if(test_sd == 0)		//用于测试格式化
	if(res == FR_NO_FILESYSTEM)		//没有文件系统，格式化
	{
//		test_sd =1;				//用于测试格式化
		printf("没有文件系统! \r\n");		
		res = f_mkfs("", 0, 0,work,MAX);		//格式化sd卡
		if(res == FR_OK)
		{
			printf("格式化成功! \r\n");		
			res = f_mount(NULL,"0:",1); 		//格式化后先取消挂载
			res = f_mount(&fs,"0:",1);			//重新挂载	
			if(res == FR_OK)
			{
				printf("SD卡已经成功挂载，可以进进行文件写入测试!\r\n");
			}	
		}
		else
		{
			printf("格式化失败! \r\n");		
		}
	}
	else if(res == FR_OK)
	{
		printf("挂载成功! \r\n");		
	}
	else
	{
		printf("挂载失败! \r\n");
	}	

	res = f_open(&file,filename,FA_OPEN_ALWAYS |FA_WRITE);
	if((res & FR_DENIED) == FR_DENIED)
	{
		printf("卡存储已满，写入失败!\r\n");		
	}
	
	f_lseek(&file, f_size(&file));//确保写词写入不会覆盖之前的数据
	if(res == FR_OK)
	{
		printf("打开成功/创建文件成功！ \r\n");		
		res = f_write(&file,write_buff,bufSize,&Bw);		//写数据到SD卡
		if(res == FR_OK)
		{
			printf("文件写入成功！ \r\n");			
		}
		else
		{
			printf("文件写入失败！ \r\n");
		}		
	}
	else
	{
		printf("打开文件失败!\r\n");
	}	
	
	f_close(&file);						//关闭文件		
	f_mount(NULL,"0:",1);		 //取消挂载
	
}

void ReadFromSD(char filename[], uint32_t read_len)
{
    FATFS fs;
    FIL file;
    FRESULT res;
    UINT Br;
    BYTE buffer[512];
    uint32_t total_read = 0;
    
    // 初始化SD卡
    res = SD_init();
    if (res != 0) {
        printf("SD卡初始化失败！\r\n");
        return;
    }

    // 挂载文件系统
    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK) {
        printf("文件系统挂载失败！错误代码: %d\r\n", res);
        return;
    }

    // 打开文件
    res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
        printf("打开文件失败！错误代码: %d\r\n", res);
        f_mount(NULL, "0:", 1);
        return;
    }

    printf("开始读取文件：%s\r\n", filename);
    do {
        res = f_read(&file, buffer, sizeof(buffer), &Br);
        if (res != FR_OK) {
            printf("文件读取失败！错误代码: %d\r\n", res);
            break;
        }

        // 输出读取的数据（可以替换为其他处理方式）
        for (UINT i = 0; i < Br; i++) {
            putchar(buffer[i]);
        }

        total_read += Br;
        if (read_len != 0 && total_read >= read_len)
            break;

    } while (Br == sizeof(buffer));

    printf("\r\n读取完成，共读取 %lu 字节。\r\n", total_read);

    f_close(&file);
    f_mount(NULL, "0:", 1);
}


void ShowSDDirectory(char *path)
{
    FATFS fs;
    FRESULT res;
    DIR dir;
    FILINFO fno;

    res = SD_init();
    if (res != 0) {
        printf("SD卡初始化失败！\r\n");
        return;
    }

    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK) {
        printf("文件系统挂载失败！错误代码: %d\r\n", res);
        return;
    }

    res = f_opendir(&dir, path);  // 打开目录
    if (res != FR_OK) {
        printf("无法打开目录: %s 错误代码: %d\r\n", path, res);
        f_mount(NULL, "0:", 1);
        return;
    }

    printf("目录 %s 下的文件列表：\r\n", path);
    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break; // 结束
        if (fno.fattrib & AM_DIR)
            printf("[DIR]  %s\r\n", fno.fname);
        else
            printf("[FILE] %s  (%lu 字节)\r\n", fno.fname, fno.fsize);
    }

    f_closedir(&dir);
    f_mount(NULL, "0:", 1);
		
}

void CheckFileExists(const char* filename)
{
    FATFS fs;
    FILINFO fno;
    FRESULT res;
    
    // 初始化 SD 卡
    if (SD_init() != 0) {
        printf("SD卡初始化失败\r\n");
        return;
    }
    
    // 挂载文件系统
    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK) {
        printf("挂载失败: %d\r\n", res);
        return;
    }
    
    // 检查文件是否存在
    res = f_stat(filename, &fno);
    if (res == FR_OK) {
        printf("文件存在: %s, 大小: %lu 字节\r\n", filename, fno.fsize);
    } else {
        printf("文件不存在或无法访问: %s, 错误: %d\r\n", filename, res);
    }
    
    // 列出目录内容，确认文件位置
    printf("列出目录内容:\r\n");
    DIR dir;
    res = f_opendir(&dir, "0:/img");
    if (res == FR_OK) {
        while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != 0) {
            printf("  %s (%lu 字节)\r\n", fno.fname, fno.fsize);
        }
        f_closedir(&dir);
    } else {
        printf("无法打开目录 0:/img, 错误: %d\r\n", res);
    }
    
    f_mount(NULL, "0:", 1);
}



///////////////////////////END//////////////////////////////////////



