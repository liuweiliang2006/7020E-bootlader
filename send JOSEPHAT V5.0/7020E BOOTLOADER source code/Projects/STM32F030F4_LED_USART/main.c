/**
  ****************************************************************************** 
  * IAP SPIFLASH TO STMFLASH FOR STM32F030XC
  * VERSION: V1.0
  * DATE:2019-09-10
  ******************************************************************************
	* P4_METER 
	* BJZL Technology Ltd. 
  ******************************************************************************
  */
//FLASH: 0X08000000~0x08007FFF,Loader:32K,
//FLASH：0x08008000~0x08020FFF,APP1:100K
//FLASH：0x08021000~0x08039FFF,APP1:100K
//FLASH：0x0803A000~0x0803FFFF,:24K
//
//     +-------+                +-----------------+
//	   | STM32 |			          | TCP/HTTP SERVER |
//     +---+---+                +------+----------+
//		   |        TCP SOCKET         |
//		   |-------------------------->|
//		   |                           |
//		   |  TCP <FIRMWARE URL MD5>   |
//		   |<--------------------------|
//       |                           |
//		   |  HTTP GET URL OF FIRMWARE |
//	     |-------------------------->|
//		   |                           |
//		   |  TCP <REPORT STATUS>      |
//       |-------------------------->|
//		      .		THE     END        .

#include "stdlib.h"
#include "stdint.h"
#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "stmflash.h"
#include "iap.h"
#include "sim900a.h"
#include "beep.h"
#include "md5.h"
#include "spi_flash.h"
#include "motor.h"
#include "MB85RS16A.h"

#define SERVER_IPADDR	"47.95.200.195"
#define SERVER_PORT		"6060"
#define FIRMWARE_URL	"47.95.200.195:8080/bin/userapp-mini.bin"
#define MAIN_DEBUG 1

//#define SERVER_IPADDR	"39.101.176.192:5057/"
//#define SERVER_PORT		"8080"
//#define FIRMWARE_URL	"bin/APP1.bin"
//#define MAIN_DEBUG 1

//updata_t updata = {.URL_ADDR = SERVER_IPADDR}; //ip,port
//REAL_DATA_PARAM_t REAL_DATA_PARAM;
updata_t updata;
REAL_DATA_PARAM_t REAL_DATA_PARAM;

//bool valueStatus = false;

extern MotorStatus_t gassembleStatus;

void sys_init()
{
    delay_init();
		__enable_irq();
    usart3_init(115200);
	/*增加tim7监视串口*/
	 tim7_init();
	/*******   ****/
    led_init();
    beep_init();
	  Moto_Init();
		SPI_FLASH_Init();
	  SPI_FLASH_VCC_ON();
    usart1_init(115200);
	
	  delay_ms(1000);	
}

void read_sflash()
{
	  uint32_t FlashID = 0;
	  char i;
//    FlashID = SPI_FLASH_ReadID();
		FlashID = SPI_FRAM_ReadID();
		printf("\r\n---P4Meter Bootloader Init!---\r\n");
		if(FlashID == 0x47F0101)
	  printf("\r\n FlashID is 0x%X\r\n", FlashID);
		delay_ms(500);
//		MB85RS16A_WRITE(BOOTFLAG_ADDR, (uint8_t*)&updata, sizeof(updata_t));
//		delay_ms(500);
//		SPI_FLASH_BufferRead((uint8_t*)&updata,BOOTFLAG_ADDR,sizeof(updata_t));
		MB85RS16A_READ(BOOTFLAG_ADDR,(uint8_t*)&updata,sizeof(updata_t));
		delay_ms(500);
		printf("\r\n UPDATAFLAG：0x%X \r\n", updata.BOOTFLAG);
		printf("\r\n FILESIZE：%d \r\n", updata.FILESIZE);
		printf("\nMD5:");
    for(i=0;i<16;i++) printf("%02x", updata.MD5CODE[i]);
		printf("\r\n URL: %s\r\n",updata.URL_ADDR);

		REAL_DATA_PARAM_READ();
}
	
int main(void)
{
		uint32_t whiletime_out = 10;
	  char jumpAPP = 0,copy_err = 0;
		char *url_from_server;
	  int8_t UPDATAFLAG = 0;
	  uint8_t keyValue = 0;
    sys_init(); 
		printf("simcom7020E boot start\r\n");
//		updata.BOOTFLAG = 0x00;
////	updata.FILESIZE =0;
//		MB85RS16A_WRITE(BOOTFLAG_ADDR,(uint8_t*)&updata,sizeof(updata_t));
//		url_from_server = "39.101.176.192:8080/bin/app1.bin";
//		sim900a_http_get(url_from_server);
	
	  read_sflash();
//		while(1);
	  gassembleStatus = motor_null;
	
	  if(strcmp(REAL_DATA_PARAM.TankLockStatus, "0")==0)
		{
				strcpy(REAL_DATA_PARAM.TankLockStatus,"1");
				gassembleStatus = motor_close;
			  printf("1\r\n");
			  REAL_DATA_PARAM_Write();
			
			  ENBOOST_PWR(1); //6.5伏电压,给传感器,电磁阀供电 1.1mA
				
				assemble_test();
				
				//ENBOOST_PWR(0); //6.5伏电压,给传感器,电磁阀供电 1.1mA
		}
//		
//		delay_ms(1000);
//			iap_load_app(FLASH_APP1_ADDR);
		
		gassembleStatus = motor_null;
		keyValue = GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0);
		//printf("keyValue:%d\r\n",keyValue);
	  if(keyValue == 0)
		{
			if(strcmp(REAL_DATA_PARAM.TankLockStatus, "1")==0)
      {
				strcpy(REAL_DATA_PARAM.TankLockStatus,"0");
				gassembleStatus = motor_open;
				REAL_DATA_PARAM_Write();
				printf("2\r\n");
				ENBOOST_PWR(1); //6.5伏电压,给传感器,电磁阀供电 1.1mA
				delay_ms(3000);
				assemble_test();
				
				ENBOOST_PWR(0); //6.5伏电压,给传感器,电磁阀供电 1.1mA
			}
//			else
//			{
//			  strcpy(REAL_DATA_PARAM.TankLockStatus,"1");
//				gassembleStatus = motor_close;
//				REAL_DATA_PARAM_Write();
//				printf("2\r\n");
//				ENBOOST_PWR(1); //6.5伏电压,给传感器,电磁阀供电 1.1mA
//				
//				assemble_test();
//				
//				ENBOOST_PWR(0); //6.5伏电压,给传感器,电磁阀供电 1.1mA
//			}
		}
	  
		if(updata.BOOTFLAG == 0xAA)
//		if(1)
    {
				sim900a_vcc_init();
		  	sim900a_gpio_init();
				sim900a_poweron();
				while(!sim900a_cmd_with_reply("AT", "OK", NULL, GSM_CMD_WAIT_SHORT)) {
            delay_ms(300);
            whiletime_out--;
            if(whiletime_out == 0)break;
        }
        

        /* 频段 */				
//        while(!sim900a_cmd_with_reply("AT+CBAND=1,3,5,8", "OK", NULL, GSM_CMD_WAIT_SHORT)) {
				whiletime_out = 10;
				while(!sim900a_cmd_with_reply("AT+CBAND=20", "OK", NULL, GSM_CMD_WAIT_SHORT)) {
            delay_ms(300);
            whiletime_out--;
            if(whiletime_out == 0)break;
        }
        whiletime_out = 10;

        /* 检查SIM卡是否插入 */
        while(!sim900a_cmd_with_reply("AT+CPIN?", "READY", NULL, GSM_CMD_WAIT_SHORT)) {
            delay_ms(300);
            whiletime_out--;
            if(whiletime_out == 0)break;
        }

        /* 查信号质量 */
        whiletime_out = 35;
        while(!sim900a_cmd_with_reply("AT+CSQ", "OK", NULL, GSM_CMD_WAIT_SHORT)) {
            delay_ms(600);
            whiletime_out--;
            if(whiletime_out == 0)break;
        }
        whiletime_out = 35;
        /* 查询是否注册 */
        while(!(sim900a_cmd_with_reply("AT+CGREG?", "0,5", NULL, GSM_CMD_WAIT_SHORT) || \
							sim900a_cmd_with_reply("AT+CGREG?", "0,1", NULL, GSM_CMD_WAIT_SHORT))) {
            delay_ms(600);
            whiletime_out--;
            if(whiletime_out == 0)break;
        }

        whiletime_out = 10;
        /* 设置接收的数据显示+IPD */
        while(!sim900a_cmd_with_reply("AT+CIPHEAD=1", "OK", NULL, GSM_CMD_WAIT_SHORT)) {
            delay_ms(300);
            whiletime_out--;
            if(whiletime_out == 0)break;
        }

        if(!(sim900a_cmd_with_reply("AT+CGREG?", "0,5", NULL, GSM_CMD_WAIT_SHORT) || \
							sim900a_cmd_with_reply("AT+CGREG?", "0,1", NULL, GSM_CMD_WAIT_SHORT)))
				{
					updata.BOOTFLAG = 0x22;
					MB85RS16A_WRITE(BOOTFLAG_ADDR,(uint8_t*)&updata,sizeof(updata_t));
					printf("no connect,jump to app\r\n");
					iap_load_app(FLASH_APP2_ADDR);
//					__enable_irq();
//				  usart1_init(115200);
					printf("no connect,jump to app 2\r\n");
				}
				else
				{
						while(1)
						{
							 do 
							 {
									 if(0 == sim900a_http_init()) 
										{
												break;
										}
							 } while(1);

								UPDATAFLAG = update_from_sim900a(FLASH_APP1_ADDR, updata.URL_ADDR, updata.MD5CODE);
								if(UPDATAFLAG == 0)
								{
										updata.BOOTFLAG = 0x22;//置固件更新成功标志位
		//								SPI_FLASH_BufferWrite((uint8_t*)&updata,BOOTFLAG_ADDR,sizeof(updata_t));
										MB85RS16A_WRITE(BOOTFLAG_ADDR,(uint8_t*)&updata,sizeof(updata_t));
										printf("Success to write new firmware, jump to app now!\r\n");					
		//								sim900a_net_send("Success to write new firmware, jump to app now!\n");
										iap_load_app(FLASH_APP1_ADDR);
									
										iap_load_app(FLASH_APP2_ADDR);	
										updata.BOOTFLAG = 0xAA;//置固件更新失败,重新联网下载
										jumpAPP = 1;
		//								SPI_FLASH_BufferWrite((uint8_t*)&updata,BOOTFLAG_ADDR,sizeof(updata_t));
										MB85RS16A_WRITE(BOOTFLAG_ADDR,(uint8_t*)&updata,sizeof(updata_t));
										printf("Jump to new firmware failed, reboot now!\r\n");
		//								sim900a_net_send("Success to write new firmware, jump to app now!\n");

										iap_load_app(FLASH_BASE);
										break;
								}
								else if(UPDATAFLAG == -1)
								{
									 updata.BOOTFLAG = 0x22;//置固件更新成功标志位
		//								SPI_FLASH_BufferWrite((uint8_t*)&updata,BOOTFLAG_ADDR,sizeof(updata_t));
										MB85RS16A_WRITE(BOOTFLAG_ADDR,(uint8_t*)&updata,sizeof(updata_t));
										printf("no Success to write new firmware, jump to app 1!\r\n");
		//								sim900a_net_send("Success to write new firmware, jump to app now!\n");
										iap_load_app(FLASH_APP1_ADDR);
										printf("no Success to write new firmware, jump to app 2!\r\n");
										iap_load_app(FLASH_APP2_ADDR);
									
										updata.BOOTFLAG = 0xAA;//置固件更新失败,重新联网下载
		//								SPI_FLASH_BufferWrite((uint8_t*)&updata,BOOTFLAG_ADDR,sizeof(updata_t));
										MB85RS16A_WRITE(BOOTFLAG_ADDR,(uint8_t*)&updata,sizeof(updata_t));
										printf("Jump to new firmware failed, reboot now!\r\n");
		//								sim900a_net_send("Success to write new firmware, jump to app now!\n");
										iap_load_app(FLASH_BASE);
										break;
								}
								else
								{
									copy_err++;
									if(copy_err>3)
									{
										#ifdef MAIN_DEBUG
										printf("---BOOTLOAD FALL---\r\n");
										#endif 
										
										iap_load_app(FLASH_APP2_ADDR);
										
										updata.BOOTFLAG = 0xAA;//置固件更新失败，原固件不可用
		//								SPI_FLASH_BufferWrite((uint8_t*)&updata,BOOTFLAG_ADDR,sizeof(updata_t));
										MB85RS16A_WRITE(BOOTFLAG_ADDR,(uint8_t*)&updata,sizeof(updata_t));			
										iap_load_app(FLASH_BASE);
										break;		
									}	
								}					
						 }
			    }
			}
		  else if(updata.BOOTFLAG == 0x22) 
			{
				    printf("---BOOTLOAD 22 jump 2---\r\n");
						iap_load_app(FLASH_APP2_ADDR);
								
						updata.BOOTFLAG = 0xAA;
//								SPI_FLASH_BufferWrite((uint8_t*)&updata,BOOTFLAG_ADDR,sizeof(updata_t));
						MB85RS16A_WRITE(BOOTFLAG_ADDR,(uint8_t*)&updata,sizeof(updata_t));	
				    printf("---BOOTLOAD 22 jump FLASH_BASE---\r\n");
						iap_load_app(FLASH_BASE);
			}
			else
			{
				printf("\r\n---JUMP TO APP %d---\r\n",jumpAPP);
				delay_ms(500);
				iap_load_app(FLASH_APP1_ADDR);//跳转到运行区
				printf("3\r\n");
				iap_load_app(FLASH_APP2_ADDR);
				printf("4\r\n");
				updata.BOOTFLAG = 0xAA;//置固件更新失败,重新联网下载
//								SPI_FLASH_BufferWrite((uint8_t*)&updata,BOOTFLAG_ADDR,sizeof(updata_t));
				MB85RS16A_WRITE(BOOTFLAG_ADDR,(uint8_t*)&updata,sizeof(updata_t));
				printf("Jump to new firmware failed, reboot now!\n");
//								sim900a_net_send("Success to write new firmware, jump to app now!\n");
				iap_load_app(FLASH_BASE);
				#ifdef MAIN_DEBUG
				printf("---NO PROGAM---\r\n");
				#endif 
			}	
			
			iap_load_app(FLASH_BASE);
}




