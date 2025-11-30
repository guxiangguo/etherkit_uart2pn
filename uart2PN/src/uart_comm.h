#ifndef	__UART_COMM_H
#define	__UART_COMM_H
#include <rtthread.h>

//通用串口通讯
#define GSC_DATA_LEN                 1024


typedef enum
{
	WaitSOF1=1,
	WaitSOF2,
	WaitCMD,
	WaitLenlow,
	WaitLenHigh,
	WaitData,
	WaitCRC
}e_waitState;

#pragma pack(1)
typedef struct
{
	rt_uint8_t sof1;
	rt_uint8_t sof2;
	rt_uint8_t cmdMax;
	rt_uint8_t cmdMin;
	rt_uint16_t datlenMax;
	rt_uint16_t queueHead;
	rt_uint16_t dataLen;
	e_waitState state;
}t_rcvProc;


typedef struct
{
	rt_uint8_t sof[2];	                         //帧头
	rt_uint8_t cmd;		                           //命令
	rt_uint16_t datlen;	                         //数据长度n
	rt_uint8_t dat[GSC_DATA_LEN];	               //n-2个数据
	rt_uint16_t crc;		                         //检验值
}t_sFrameUART;
#pragma pack()

unsigned short CRC16(unsigned char *puchMsg, unsigned short usDataLen);
rt_uint8_t FindProtocol(struct rt_ringbuffer *q_uart, t_rcvProc *rvP, t_sFrameUART *rvF);

#endif

