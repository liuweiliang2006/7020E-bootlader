#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "sim900a.h"
#include "stmflash.h"
#include "iap.h"
#include "beep.h"
#include "led.h"
#include "md5.h"
#include "spi_flash.h"

extern void sys_init();

//从GSM模块内部ram中一次读取的数据量
#define FIRMWARE_BLOCK	(1024)
//#define FIRMWARE_BLOCK	(512)
extern updata_t updata;
iapfun jump2app;
u16 iapbuf[1024];


//将数据写入到内部flash中
void iap_write_appbin(u32 appxaddr, u8 *appbuf, u32 appsize)
{
    u16 t;
    u16 i = 0;
    u16 temp;
    u32 fwaddr = appxaddr;
    u8 *dfu = appbuf;

    for(t = 0; t < appsize; t += 2) {
        temp = (u16)dfu[1] << 8;
        temp += (u16)dfu[0];
        dfu += 2;
        iapbuf[i++] = temp;
        if (i == 1024) {
            i = 0;
            STMFLASH_Write(fwaddr, iapbuf, 1024);
            fwaddr += 2048;
        }
    }

    if(i)
        STMFLASH_Write(fwaddr, iapbuf, i);
}


void IAPTOAPP_Set(u32 appxaddr)
{
    int i ;

    RCC->CFGR = 0;

    for(i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0XFFFFFFFF;
        NVIC->ICPR[i]  = 0xFFFFFFFF;
    }
    USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
    USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
    __set_PRIMASK(1);
//			__set_BASEPRI(0);
//			__set_FAULTMASK(0);//CORTEX-M3
    __set_PSP(*(vu32*)appxaddr);
    __set_CONTROL(0);
    __disable_irq();


}


//跳转到指定地址
void iap_load_app(u32 appxaddr)
{
    IAPTOAPP_Set(appxaddr);

    if(((*(vu32*)appxaddr) & 0x2FFE0000) == 0x20000000)
    {
        jump2app = (iapfun) * (vu32*)(appxaddr + 4);
        MSR_MSP(*(vu32*)appxaddr);
        jump2app();
    } else {
        sys_init();
        printf("user app was broken!!!\n");
    }
}

//连接到TCP服务器
int connect_update_server(char* ip, char* port, int timeout)
{
    int times = 0;
    int ret = -1;

    sim900a_close_net();

    /* connect to update server */
    do {
        times++;
        ret = sim900a_tcpudp_connect(0, ip, port);
        if(ret == -1) {
            printf("Failed to connect to server %d times!\n", times);
        } else {
            printf("Success to connect to server!\n");
            break;
        }
//        LED0 = !LED1;
//        LED1 = !LED0;
        beep_ms(500);
    } while(times < timeout);

    return (ret ? 0 : -1);
}

//打印MD5字符串
void MD5Print(char* md5_str)
{
    int i = 0;

    printf("\nMD5:");
    for(i = 0; i < 16; i++) {
        printf("%02x\n", md5_str[i]);
    }
}

//从GSM模块中一段一段的读出数据并且校验
int check_firmware_in_sim900a(const char* md5)
{
    char temp_md5[16] = {0};
  //  int read_addr = 0;
    MD5_CTX md5_ctx;
		
		int32_t total_size = updata.FILESIZE;	
		int read_addr = 137441280; //buffer首地址
		int len1,len2;

    MD5Init(&md5_ctx);
		
		char data_buff[1024]={0};	  
		char *data = NULL;

    do {
      //  char* data = NULL;
        int read_len = -1;

        len1 = sim900a_http_read(read_addr, 512, &data);
	      memcpy(&data_buff[0],data,512);		
			  read_addr+=len1;
			
			  len2 = sim900a_http_read(read_addr, 512, &data);
	      memcpy(&data_buff[512],data,512);		
			  read_addr+=len2;
			
		
			  read_len=len1+len2;
			
        if(read_len > 0) {
         //   MD5Update(&md5_ctx, (unsigned char*)data_buff, read_len);
					
					if(total_size<1024&&total_size>0)
					{
						MD5Update(&md5_ctx, (unsigned char*)data_buff, total_size);
					}
					else
					{
						MD5Update(&md5_ctx, (unsigned char*)data_buff, read_len);
					}
					 total_size-=FIRMWARE_BLOCK;
           // read_addr += read_len;
            printf("\r\n READ_LEN:%d \r\n", read_len);
            printf("\r\n READ_ADDR:%d \r\n", read_addr);
            if(read_len == FIRMWARE_BLOCK) {
							if(total_size<=0)
							{
								break;
							}
                continue;
            } else if(read_len > FIRMWARE_BLOCK) {
                printf("MD5:Read error, read size bigger than expected!\n");
                return -1;
            } else {
                //last packet
                break;
            }
        } else if(read_len == 0) {
            printf("MD5:Read nothing!\n");
            break;
        } else {
            printf("MD5:Read error!\n");
            return -1;
        }
    } while(1);

    MD5Final(&md5_ctx, (unsigned char*)temp_md5);
    MD5Print(temp_md5);

    if(strncmp(temp_md5, md5, 16) == 0) {
        printf("GSM RAM md5 check ok, write to flash now!!!\n");
    } else {
        printf("GSM RAM md5 check failed, firmware broken!!!\n");
        return -1;
    }

    return 0;
}

//从内部flash中一段一段的读出数据并且校验
int check_firmware_in_sflash(const char* md5)
{

    char temp_md5[16] = {0};
    int read_addr = FLASH_APP1_ADDR;
    char i;
    MD5_CTX md5_ctx;
    MD5Init(&md5_ctx);
    uint32_t flie_size = updata.FILESIZE;

    do {
        int read_len = -1;
        printf("\r\n UPDATAFLAG:0x%X \r\n", updata.BOOTFLAG);
        printf("\r\n FILESIZE:%d \r\n", updata.FILESIZE);
        printf("\nMD5:");
        for(i = 0; i < 16; i++) {
            printf("%02x", updata.MD5CODE[i]);
        }

        STMFLASH_Read(read_addr, iapbuf, FIRMWARE_BLOCK);
//				SPI_FLASH_BufferRead((u8*)iapbuf,read_addr,FIRMWARE_BLOCK);

        if(flie_size < FIRMWARE_BLOCK)
        {
            read_len = flie_size;
        }
        else
        {
            flie_size -= FIRMWARE_BLOCK;
            read_len = FIRMWARE_BLOCK;
        }
        if(read_len > 0) {
            MD5Update(&md5_ctx, (unsigned char*)iapbuf, read_len);
            read_addr += read_len;
            printf("\r\n READ_LEN:0x%X \r\n", read_len);
            printf("\r\n READ_ADDR:0x%X \r\n", read_addr);
            printf("\r\n DATA: %s \r\n", (u8*)iapbuf);
            if(read_len == FIRMWARE_BLOCK) {
                continue;
            } else if(read_len > FIRMWARE_BLOCK) {
                printf("MD5:Read error, read size bigger than expected!\n");
                return -1;
            } else {
                printf("\r\n break \r\n");
                //last packet
                break;
            }
        } else if(read_len == 0) {
            printf("MD5:Read nothing!\n");
            break;
        } else {
            printf("MD5:Read error!\n");
            return -1;
        }
    } while(1);

    MD5Final(&md5_ctx, (unsigned char*)temp_md5);
    MD5Print(temp_md5);

    if(strncmp(temp_md5, md5, 16) == 0) {
        printf("STM32FLASH md5 check ok, write to flash now!!!\n");
    } else {
        printf("STM32FLASH md5 check failed, firmware broken!!!\n");
        return -1;
    }

    return 0;
}
//从GSM模块中一段一段的读出数据并写入到flash中
int write_firmware_from_sim900a(uint32_t dst)
{
    uint32_t write_addr = dst;
    uint32_t len1, len2;
//    int read_addr = 0;
    int32_t total_size = updata.FILESIZE;
    int read_addr = 137441280; //buffer首地址
	
	  char data_buff[1024]={0};	  
		char *data = NULL;
    do {

        int read_len = -1;
				
        //read_len = sim900a_http_read(read_addr, FIRMWARE_BLOCK, &data);
        len1 = sim900a_http_read(read_addr, 512, &data);//读2次512
			  memcpy(&data_buff[0],data,512);
        read_addr += len1;
			
				printf("\n1-512--start\n");
				for(uint16_t i=0;i<512;i++){
					printf("%2d",data_buff[i]);
				}
				printf("\n1-512--end\n");
						
        len2 = sim900a_http_read(read_addr, 512, &data);
			  memcpy(&data_buff[512],data,512);
			  read_addr += len2;
			
				printf("\n2-512--start\n");
				for(uint16_t i=0;i<512;i++){
					printf("%2d",data_buff[i]);
				}
				printf("\n2-512--end\n");

        read_len = len1 + len2;
			
        if(read_len > 0) {
					  printf("\n1k--start\n");
					  for(uint16_t i=0;i<1024;i++){
							printf("%2d",data_buff[i]);
					  }
						printf("\n1k--end\n");
            iap_write_appbin(write_addr, (u8*)data_buff, read_len); 
            write_addr += read_len;
					
					  total_size -= FIRMWARE_BLOCK;
						
						

            if(read_len == FIRMWARE_BLOCK ) {
							if(total_size<=0)
							{
								break;
							}
                continue;
            } else if(read_len > FIRMWARE_BLOCK ) {
                printf("IAP:Read error, read size bigger than expected!\n");
                return -1;
            } else {
                //last packet
                break;
            }
        } else if(read_len == 0) {
            printf("IAP:Read nothing!\n");
            break;
        } else {
            printf("IAP:Read error!\n");
            return -1;
        }        
    } while(1);

    return 0;
}

//从指定的url下载固件，校验完成后写入
int update_from_sim900a(uint32_t dst, char* url, const char* md5)
{
	 printf("url:%s\r\n",url);
    if(0 != sim900a_http_get(url)) {
        printf("SIM700:do http get failed!\n");
        return -1;
    }

    if(md5) {
        if(0 != check_firmware_in_sim900a(md5)) {
            return -1;
        }
    }

    if(0 != write_firmware_from_sim900a(dst)) {
        printf("write userapp failed!\n");
        return -2;
    }

    if(md5) {
        printf("check firmware sflash!\n");
        if(0 != check_firmware_in_sflash(md5)) {
            return -2;
        }
    }
    return 0;
}
