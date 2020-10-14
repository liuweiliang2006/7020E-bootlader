/*
 * sim900a.c
 *
 *  Created on: 2017年5月7日
 *      Author: xianlee
 */

#include "stm32f0xx.h"
#include "stdio.h"
#include "stdlib.h"
#include "sim900a.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "iap.h"

uint8_t end_frame;
#define ENABLE_TIMEOUT 0
extern updata_t updata;
gsm_data_record gsm_global_data = { "\0", 0, 0};

static const char *modetbl[2] = { "TCP", "UDP" };

void sim900a_vcc_init()
{
    GPIO_InitTypeDef GPIO_InitStructureC;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
    GPIO_InitStructureC.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructureC.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructureC.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructureC);
    GPIO_ResetBits(GPIOC, GPIO_Pin_10);
    delay_ms(1000);
    GPIO_SetBits(GPIOC, GPIO_Pin_10);
}

void sim900a_gpio_init()
{
    GPIO_InitTypeDef GPIO_InitStructureC;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    GPIO_InitStructureC.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructureC.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructureC.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructureC);
    GPIO_ResetBits(GPIOA, GPIO_Pin_8);
}

void sim900a_poweron()
{
    if(sim900a_check_status()) {
        printf("%s %d:Sim900a is already power on!\n", __func__, __LINE__);
    } else {
        GPIO_SetBits(GPIOA, GPIO_Pin_8);
        delay_ms(1200);
        GPIO_ResetBits(GPIOA, GPIO_Pin_8);
        delay_ms(1000);
        printf("%s %d:Sim900a power on now!\n", __func__, __LINE__);
    }
}

int sim900a_check_status()
{
    int try_times = 5;

    /* hardware sync */
    do {
        if(sim900a_cmd_with_reply("AT", "OK", NULL, GSM_CMD_WAIT_SHORT)) {
            return 1;
        }
        delay_ms(100);
    } while(try_times--);

    return 0;
}

bool sim900a_cmd_with_reply (const char *cmd, const char * reply1, const char * reply2, uint32_t waittime)
{
	char *result = NULL;
    memset(&gsm_global_data, 0, sizeof(gsm_data_record));
    gsm_global_data.frame_len = 0;

    if((uint32_t)cmd < 0xFF) {
        sim900a_send_byte((uint32_t)cmd);
    } else {
        sim900a_usart("%s\r\n", cmd );
    }

    printf("--->%s\n", cmd);

    if ((reply1 == NULL) && (reply2 == NULL))
        return true;

#if ENABLE_TIMEOUT
    bool ret = 0;
    gsm_global_data.frame_buf[GSM_DATA_RECORD_MAX_LEN - 1] = '\0';
    TIME_LOOP_MS(waittime) {
        if ((reply1 != NULL) && (reply2 != NULL)) {
            ret = (( bool ) strstr(gsm_global_data.frame_buf, reply1)
                   || ( bool ) strstr(gsm_global_data.frame_buf, reply2));
        } else if (reply1 != 0) {
            ret = (( bool ) strstr(gsm_global_data.frame_buf, reply1));
        } else {
            ret = (( bool ) strstr(gsm_global_data.frame_buf, reply2));
        }

        if(ret) {
            break;
        }
    }

    return ret;
#else
    delay_ms(waittime);
//////////////////
//    for(uint32_t i = 0; i < sizeof(gsm_global_data.frame_buf); i++)
//    {
//        printf("%2x", gsm_global_data.frame_buf[i]);
//    }
//    printf("\r\n");
		for(int i = 0; i <gsm_global_data.frame_len; i++)
		{
			printf("%c",gsm_global_data.frame_buf[i]);
		}
		printf("\r\n");
		
		result = strstr(gsm_global_data.frame_buf, "CSQ");
		if(result)  //result != NULL
		{
			if(strstr(gsm_global_data.frame_buf, "CSQ: 0,0") || strstr(gsm_global_data.frame_buf, "CSQ: 99,99"))
			{
				return 0;
			}
		}
		
		
    if ((reply1 != 0) && (reply2 != 0)) {
        return (( bool ) strstr(gsm_global_data.frame_buf, reply1)
                || ( bool ) strstr(gsm_global_data.frame_buf, reply2));
    } else if (reply1 != 0) {
        return (( bool ) strstr(gsm_global_data.frame_buf, reply1));

    } else {
        return (( bool ) strstr(gsm_global_data.frame_buf, reply2));

    }
#endif
}

int sim900a_close_net()
{
    int ret = 0;
    ret = sim900a_cmd_with_reply("AT+CIPCLOSE=1", "CLOSE OK", NULL, GSM_CMD_WAIT_SHORT);
    ret = sim900a_cmd_with_reply("AT+CIPSHUT", "SHUT OK", NULL, GSM_CMD_WAIT_SHORT);

    return ret;
}

int sim900a_tcpudp_connect(uint8_t mode, char* ipaddr, char* port)
{
    char net_info[256] = {0};

    snprintf(net_info, 256, "AT+CIPSTART=\"%s\",\"%s\",\"%s\"", modetbl[mode],
             ipaddr, port);

    /* 判断ALREADY返回值能跟明确的指明连接已经建立 */
    if(!sim900a_cmd_with_reply(net_info, "ALREADY", NULL, GSM_CMD_WAIT_LONG)) {
        return -1;
    }

    return 1;
}

bool  GPRS_SetAPN(char *Name, char *username, char *password)
{
    char str[64];
    uint8_t answer;
    sprintf(str, "AT+CSTT=\"%s\",\"%s\",\"%s\"\r\n", "kopagas", "0", "0");
    answer = sim900a_cmd_with_reply(str, "\r\nOK\r\n", NULL, GSM_CMD_WAIT_SHORT);
    if(answer == 1)
    {
        printf("\r\nGPRS_SetAPN(\"%s\",\"%s\",\"%s\") ---> OK\r\n", Name, username, password);
        return true;
    }
    else
    {
        printf("\r\nGPRS_SetAPN(\"%s\",\"%s\",\"%s\") ---> ERROR\r\n", Name, username, password);
        return false;
    }
}

/* assume network status was connected */
int sim900a_net_send(char* data)
{
    if (sim900a_cmd_with_reply("AT+CIPSEND", ">", NULL, GSM_CMD_WAIT_LONG)) {
        delay_us(500);
        sim900a_usart("%s", data);
        delay_us(50);
        if (sim900a_cmd_with_reply((char*) 0X1A, "OK", NULL, 1864)) {
            printf("send success!\n");
        } else {
            printf("send failed!\n");
            return -1;
        }
    } else {
        sim900a_cmd_with_reply((char*) 0X1B, NULL, NULL, GSM_CMD_WAIT_NONE);	//ESC,取消发送
        return -1;
    }

    return 1;
}

int sim900a_http_init()
{
    int ret = -1;
    int whiletime_out = 3;

    sim900a_cmd_with_reply("AT+CHTTPDISCON=0", NULL, NULL, GSM_CMD_WAIT_NORMAL);//断开
    sim900a_cmd_with_reply("AT+CHTTPDESTROY=0", NULL, NULL, GSM_CMD_WAIT_NORMAL);//注销
    return 0;
}

int sim900a_http_get(char* url)
{
		char find_ch = '/';
    char url_at[128] = {0};
		char url_path[256] = {0};
    int ret = -1;
    int i = 0;
    int ret_start = 0;
    int whiletime_out = 50000;
		volatile uint8_t u8_find_pos = 0;
		char ch_uarl_send[128] = {0};
		uint8_t find_cnt = 0;
		uint8_t location = 0;
		char path[256] = {0};
		
		for(i=0;i<strlen(url);i++)
		{
			ch_uarl_send[i] = url[i];
			if(find_ch == url[i])
			{
				location = i;
				break;
			}
		}
		for(i=location;i<strlen(url);i++)
		{
			path[i-location] = url[i];

		}
//		path = url+location;

    //将请求的url格式化到命令中
		printf("url:%s\r\n",url);
    snprintf(url_at, 128, "AT+CHTTPCREATE=\"http://%s\"", ch_uarl_send);
		snprintf(url_path, 256, "AT+CHTTPTOFS=0,\"%s\"", path);

    printf("%s %d:%s\n", __func__, __LINE__, url_at);

    do {
//        ret = sim900a_cmd_with_reply(url_at, "OK", NULL, GSM_CMD_WAIT_LONG);
//			 ret = sim900a_cmd_with_reply("AT+CHTTPCREATE=\"http://39.101.176.192:8080/\"", "OK", NULL, GSM_CMD_WAIT_LONG);
//			ret = sim900a_cmd_with_reply(ch_uarl_send, "OK", NULL, GSM_CMD_WAIT_LONG);
			  ret = sim900a_cmd_with_reply(url_at, "OK", NULL, GSM_CMD_WAIT_LONG);
        if(ret) {
            printf("%s %d:Success to connect to server!\n", __func__, __LINE__);
            break;
        } else {
					printf("%s %d:failed to connect to server!\n", __func__, __LINE__);
            break;
        }
    } while(!ret);

    sim900a_cmd_with_reply("AT+CHTTPCON=0", "OK", NULL, GSM_CMD_WAIT_LONG);//建立连接
//    sim900a_cmd_with_reply("AT+CHTTPTOFS=0,\"/bin/APP1.bin\"", "OK", NULL, GSM_CMD_WAIT_LONG);//下载
//		sim900a_cmd_with_reply("AT+CHTTPTOFS=0,\"/bin/app1.bin\"", "OK", NULL, GSM_CMD_WAIT_LONG);//下载
		sim900a_cmd_with_reply(url_path, "OK", NULL, GSM_CMD_WAIT_LONG);//下载


    // waiting for action cmd return
    //下载完成大概90S左右，这里给100S
    printf("\nDowloading:");
		
        do {
                if(end_frame==1)
                {
                    char *strbuf = strstr((const char*)gsm_global_data.frame_buf, "+CHTTPTOFSOK");
                    if(strbuf != NULL) 
                    {
                        updata.FILESIZE=atoi(&strbuf[16]);
                        printf("\ngsm_global_data start1\n");
                        for(i = 0; i < 100; i++)
                        {
                            printf("i=%d,%c\n",i, strbuf[i]);
                        }
                        printf("\ngsm_global_data end1\n");
                        printf("\ngsm_global_data start2\n");
                        for(i = 0; i < GSM_DATA_RECORD_MAX_LEN - 20; i++)
                        {
                            printf("i=%d,%c\n",i, gsm_global_data.frame_buf[i]);
                        }
                        printf("\ngsm_global_data end2\n");
                        printf("File Size: %d\n", updata.FILESIZE);
                        printf("\n%s %d:HTTP GET finished!\n", __func__, __LINE__);
                        break;
                    }     
                    else 
                    {
                        printf(">");
                        delay_ms(2);
                    }
                    gsm_global_data.frame_len=0;
                    memset(&gsm_global_data, 0, sizeof(gsm_data_record));
                    end_frame=0;
                    
                    whiletime_out--;
                    if(whiletime_out == 0)break;
                }

        } while(1);

    ret = 0;
    return ret;
}

int sim900a_http_read(int start, int size, char** data)
{
    int ret = -1;
    int i = 0;
    int read_len = 0;
    int ret_start = 0;
    char read_range[48] = {0};
    char cmd_str[16] = {0};

//    snprintf(read_range, 48, "AT+HTTPREAD=%d,%d", start, size);
    snprintf(read_range, 48, "AT+CFLR=%d,%d", start, size);

    do {
        gsm_global_data.frame_len = 0;
        if(size == 0) {
            ret = sim900a_cmd_with_reply("AT+CFLR", "+CFLR", NULL, GSM_CMD_WAIT_LONG);
        } else {
            ret = sim900a_cmd_with_reply(read_range, "+CFLR", NULL, size ? (size / 10) : 2000);
        }

        if(ret) {
            printf("Success to read data from server!\n");
            break;
        } else {
//            printf("Failed to read data, retry...!\n");
            delay_ms(1000);
        }
    } while(1);

//    for(i=0;i<GSM_DATA_RECORD_MAX_LEN - 20;i++) {
//        if(gsm_global_data.frame_buf[i] == '+' &&
//           gsm_global_data.frame_buf[i+1] == 'H' &&
//           gsm_global_data.frame_buf[i+2] == 'T' &&
//           gsm_global_data.frame_buf[i+3] == 'T' &&
//           gsm_global_data.frame_buf[i+4] == 'P') {
//            ret_start = i;
//            printf("Found data start pos!\r\n");
//            break;
//        }
//    }

//    gsm_global_data.frame_buf[GSM_DATA_RECORD_MAX_LEN -1] = '\0';

//    // 获取读取到的数据长度信息
//    sscanf(gsm_global_data.frame_buf+ret_start, "%[^:]:%d", cmd_str, &read_len);
//    // 长度信息所占长度
//    i = snprintf(cmd_str, 16, "%d", read_len);

//    // +HTTPREAD: xxxx
//    *data = gsm_global_data.frame_buf+ret_start + 1 + 9 + 1 + i + 1;
//#ifdef GSM_DEVICE_SIM800
//    *data = gsm_global_data.frame_buf+ret_start + 1 + 9 + 1 + i + 1 + 1;
//#endif

//    printf("Read http data pos:%p,len:%d\n", *data, read_len);

    *data = gsm_global_data.frame_buf + 31;//有效字节开始算
    unsigned int len1 = atoi(&gsm_global_data.frame_buf[18]);//长度，表明后面是512字节了

    for(int x = 0; x < 512; x++)
    {
        printf("%2X", gsm_global_data.frame_buf[x + 31]);
    }
		printf("\r\n");
    printf("len1=%d\r\n", len1);
    read_len = len1;

    return read_len;
}
