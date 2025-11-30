#include <rtthread.h>
#include "hal_data.h"
#include <rtdevice.h>
#include <board.h>

#define DBG_TAG "rs485"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#include "uart_comm.h"
#include "app_gsdml.h"

#define RS485_NAME       "uart5"      /* 串口设备名称 */

#define LED_PIN_0    BSP_IO_PORT_14_PIN_3 /* Onboard LED pins */

#define CMD_UPDATA_INPUT_DATA                      0x04
#define CMD_UPDATA_OUTPUT_DATA                      0x05

t_rcvProc rpPanel;
t_sFrameUART rfPanel;

static t_sFrameUART sentFrame;

static const rt_uint8_t SOF1 = 0x8B;
static const rt_uint8_t SOF2 = 0xA2;

struct rt_ringbuffer *u5_rb;
static rt_device_t rs485;

extern uint8_t _inputdata[APP_GSDML_INTPUT1_DATA_SIZE];
extern uint8_t _outputdata[APP_GSDML_OUTPUT1_DATA_SIZE];

static void sendFrame(rt_uint8_t *pdat,rt_uint16_t len)	//补充CRC后发送
{
	rt_uint16_t crc;
	crc = CRC16(pdat,len);
	*(rt_uint16_t*)(pdat+len) = crc;    
    rt_device_write(rs485, 0, pdat, len+2);
}

static void sendFrame_HBL(rt_uint8_t cmd, rt_uint8_t *pdat, rt_uint16_t datlen)
{
	rt_uint16_t j=0;
	rt_uint8_t *senddat = sentFrame.dat;
	sentFrame.sof[0] = SOF1;
	sentFrame.sof[1] = SOF2;
	sentFrame.cmd = cmd;
	sentFrame.datlen = datlen;
	if(sentFrame.datlen>GSC_DATA_LEN)
	{
		//user_Assert(__FILE__,__LINE__);
		return;
	}
	for(j=0;j<datlen;j++)
		senddat[j] = pdat[j];	//data
	sendFrame((rt_uint8_t*)(&sentFrame), sentFrame.datlen+5);
}

static void FrameProcess(t_sFrameUART *pframe)
{
	rt_uint16_t tmp;
	switch(pframe->cmd)
	{
		case CMD_UPDATA_INPUT_DATA:
			// rt_kprintf("CMD_UPDATA_INPUT_DATA\r\n");
            memcpy(&_inputdata[0],&pframe->dat[0],APP_GSDML_INTPUT1_DATA_SIZE);
			break;		
		default:
			break;
	}
}

static void CommInit(void)
{
	memset(&rpPanel,0,sizeof(rpPanel));
	rpPanel.sof1 = SOF1;
	rpPanel.sof2 = SOF2;
	rpPanel.cmdMax = 0xA5;
	rpPanel.cmdMin = 1;
	rpPanel.datlenMax = 1024;
	rpPanel.state = WaitSOF1;
	memset(&rfPanel,0,sizeof(rfPanel));
}

static void rs485_rx_thread_entry(void *parameter)
{
   //rt_uint8_t ch;
   CommInit();
   bool toggle_led_state = false;

    while (1)
    {
        while (rt_ringbuffer_data_len(u5_rb)) {    // 有数据待处理
            // rt_ringbuffer_getchar(u5_rb,&ch);
            // rt_kprintf("%02X",ch);
            if(FindProtocol(u5_rb,&rpPanel,&rfPanel))
            {
                FrameProcess(&rfPanel);
                toggle_led_state = !toggle_led_state;
                rt_pin_write(LED_PIN_0, toggle_led_state);
            }
        }
        
        rt_thread_mdelay(1);
    }
}

static void rs485_tx_thread_entry(void *parameter)
{
    while (1)
    {
        sendFrame_HBL(CMD_UPDATA_OUTPUT_DATA,&_outputdata[0],sizeof(_outputdata));
        rt_thread_mdelay(100);
    }
}

int rs485_init(void)
{
    rt_err_t ret = RT_EOK;

    // 创建环形缓冲区
    u5_rb = rt_ringbuffer_create(1024); // 创建一个大小为1024字节的环形缓冲区

    rs485 = rt_device_find(RS485_NAME);
    if (!rs485)
    {
        LOG_E("find %s failed!", RS485_NAME);
        return RT_ERROR;
    }

    rt_device_open(rs485, RT_DEVICE_FLAG_RX_NON_BLOCKING | RT_DEVICE_FLAG_TX_BLOCKING);
    LOG_I("rs485 init sucessful!");

    rt_thread_t rs485_rx_thread = rt_thread_create("rs485_rx", rs485_rx_thread_entry, RT_NULL, 1024, 25, 10);
    if (rs485_rx_thread != RT_NULL)
    {
        rt_thread_startup(rs485_rx_thread);
    }
    else
    {
        ret = RT_ERROR;
    }    

    rt_thread_t rs485_tx_thread = rt_thread_create("rs485_tx", rs485_tx_thread_entry, RT_NULL, 1024, 24, 10);
    if (rs485_tx_thread != RT_NULL)
    {
        rt_thread_startup(rs485_tx_thread);
    }
    else
    {
        ret = RT_ERROR;
    } 

    return ret;
}
INIT_APP_EXPORT(rs485_init);
