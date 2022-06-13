#include "app_config.h"
#include "os/os_api.h"
#include "asm/clock.h"
#include "system/timer.h"
#include "asm/cpu.h"
#include "generic/typedef.h"
#include "generic/gpio.h"
#include "debug.h"
#include "hrs3603.h"
#include "hrSensor_manage.h"
#include "system/os/os_api.h"
#include "printf.h"
#include <stdint.h>
#include <stdlib.h>
#include "hr_sensor/BD_health_math.h"

#if TCFG_HRS1662_EN
uint16_t bp_data_up,bp_data_down,bp_difdata;//---定义脱手阀值寄存器
uint8_t data_okflag = 0;//---定义接收数据帧标志
uint16_t bd_hrs_data[35]={0};//---定义读取传感器数据存储寄存器
uint32_t bd_data_result[15]={0};//---定义处理函数执行结果保存寄存器
#define HRS1662_IIC_READ   0X71
#define HRS1662_IIC_WRITE  0X70
static int32_t bd_get_data_timer_50 = 0;
void Delay_ms(u16 delay_time)
{
    int delay = (delay_time + 9) / 10;
    os_time_dly(delay);
}
void bd_timers_stop(void)
{
    printf("[hrs1662]%s",__func__);
	if(bd_get_data_timer_50)
	{
		sys_timer_del(bd_get_data_timer_50);
		bd_get_data_timer_50 = 0;
		os_time_dly(2);
	}
	return ;
}
void bd_timers_start(void)
{
    printf("[hrs1662]%s",__func__);
	if(!bd_get_data_timer_50)
	bd_get_data_timer_50 = sys_timer_add(NULL, BD_16XX_dataread, 50);
	return ;
}
void bd_50ms_timer_cfg(bool en)
{
	printf("[hrs1662]-----%s\n  en = %d",__func__,en);
    if(en)bd_timers_start();
    else  bd_timers_stop();
}

void LED_PWM_On(uint8_t duty_data)
{
    gpio_direction_output(IO_PORTB_03, 0);
    //printf("[hrs1662]%s %d",__func__,duty_data);
    return ;
}

void LED_PWM_Off(void)
{
    gpio_direction_output(IO_PORTB_03, 1);
    //printf("[hrs1662]%s",__func__);
     return ;
}


uint8_t BD_I2C_Write(uint8_t slave_address,uint8_t cmd_reg,uint8_t data)
{
    uint8_t data_buf = data;
    uint8_t write_len;
    write_len = hrsensor_write_nbyte(slave_address, cmd_reg, &data_buf, 1);
    //printf("[hrs1662]%s addr:0x%x,data:0x%x,slave_address:0x%x,write_len:%d",__func__,cmd_reg, data,slave_address,write_len);
    if (write_len) {
        return 1; //succeed
    } else {
        return 0;//fail
    }
}
uint8_t BD_I2C_Read(uint8_t slave_address,uint8_t cmd_reg,uint8_t *state_commu)
{
    uint8_t data_buf = 0;
    uint8_t *res = state_commu;
    uint8_t read_en;
    read_en = hrsensor_read_nbyte(slave_address+1, cmd_reg, &data_buf, 1);
    if(read_en)
    {
        *res = 1;
    }else{
        *res = 0;
    }
    //printf("[hrs1662]%s addr:0x%x , data:0x%x ,res = %d,slave_address:0x%x",__func__,cmd_reg, data_buf,*res,slave_address);
    return data_buf;
}
uint8_t BD_16XX_init_start(void)//---函数名可依据各自平台定义
{
    //printf("[hrs1662]%s",__func__);
    uint8_t temp1,temp2,temp3; //---客户其他函数
    BD_heartbp_init();//---算法初始化放在所有函数之前，即第一位
    temp1=BD_Sensor_Init();//---传感器初始化放，放在第二位
    temp2=BD_Sensor_Start();//---开启量测函数，放在第三位
    bp_data_up=27000;//---脱手检测判断阀值 1，此值要根据实际结构来做调试
    bp_data_down=500;//---脱手检测判断阀值 2，此值要根据实际结构来做调试
    bp_difdata=80;//---脱手检测判断阀值 3，此值要根据实际结构来做调试
    bd_50ms_timer_cfg(true);
    temp3=temp1&&temp2;
    //printf("[hrs1662]temp3 %d",temp3);
    return (temp3);//---初始化与开启结果返回：失败--返回 0，成功--返回 1
}
void BD_16XX_close(void)
{
    //printf("[hrs1662]%s",__func__);
    BD_Sensor_Close(); //---客户其他函数
    bd_50ms_timer_cfg(false);
    return;
}

static uint8_t get_op_hrs_results;
static uint8_t get_op_hrs_wear_results;
static uint8_t get_op_spo2_results;
static uint8_t get_op_spo2_wear_results;

uint8_t get_hrs_wear_results(void)
{
	return get_op_hrs_wear_results;
}
uint8_t get_hrs_results(void)

{
	return get_op_hrs_results ;
}
uint8_t get_spo2_wear_results(void)
{
	return get_op_spo2_wear_results;
}
uint8_t get_spo_result(void)
{
	return get_op_spo2_results ;
}

void BD_16XX_dataread(void) //定时器50ms
{

    uint8_t temp; //---客户其他函数
    temp=BD_Sensor_Data_Read(bd_hrs_data);//--0---数据未满一帧,1--满一帧,2--通讯 异常
    /* printf("[hrs1662]%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",bd_data_result[0],bd_data_result[1],\
            bd_data_result[2], bd_data_result[3], bd_data_result[4], bd_data_result[5],\
            bd_data_result[6],bd_data_result[7],bd_data_result[8],bd_data_result[9],\
            bd_data_result[10],bd_data_result[11],bd_data_result[12],bd_data_result[13],\
            bd_data_result[14]);// 打 印 结果数据以便调试*/
    // printf("[hrs1662]%s %d",__func__,temp);
    if(1==temp)//---判断一帧数据是否读取完毕
    {
        data_okflag=1;//---全局标志变量，在一帧数据读完时被置 1
        BD_16XX_datapro();
    }
    get_op_hrs_wear_results  = bd_data_result[4];
    get_op_spo2_wear_results = bd_data_result[4];
    get_op_hrs_results       = bd_data_result[0];
    get_op_spo2_results      = bd_data_result[3];
    return;
}

void BD_16XX_datapro(void)
{
    //printf("[hrs1662]%s",__func__);
    uint8_t temp; //---客户其他函数
    if(1==data_okflag)//---判断是否已经接收到一帧数据
    {
        //printf("=========================");
        data_okflag=0;//---帧标志清零
        temp=BD_health_pro(8,bd_hrs_data, bd_data_result);
        //printf("[hrs1662]%s %d",__func__,temp);
        if(8==temp)//--数据处理函数执行 OK
        {
            /*printf("[hrs1662]%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",bd_data_result[0],bd_data_result[1],\
            bd_data_result[2], bd_data_result[3], bd_data_result[4], bd_data_result[5],\
            bd_data_result[6],bd_data_result[7],bd_data_result[8],bd_data_result[9],\
            bd_data_result[10],bd_data_result[11],bd_data_result[12],bd_data_result[13],\
            bd_data_result[14]);// 打 印 结果数据以便调试*/
        }
    }
    return ;
}
  //备注： bd_data_result 存放处理结果---[0]-心率,[1]-血压高压,[2]-血压低压,[3]-血 氧,[4]-脱手状态,[5]~[14]为调试参数

static uint8_t	hrs_enable_swich = 0;
static uint8_t	spo_enable_swich = 0;
uint8_t get_hrs_enable_status(void)
{
return hrs_enable_swich;
}
uint8_t get_spo_enable_status(void)
{
return spo_enable_swich;
}

int BD_16XX_io_ctl(u8 cmd, void *arg)
{
	printf("[hrs1662]%s cmd = %d",__func__,cmd);
    switch (cmd) {
    case HR_SENSOR_ENABLE:
      printf("HR_SENSOR_ENABLE333");
        if(BD_16XX_init_start()){
        hrs_enable_swich = 1;
        printf("[hrs1662]bd 1662 init succese");
        }
        else{
        printf("[hrs1662]bd 1662 init failed !!!");
        }
        break;
    case HR_SENSOR_DISABLE:
     printf("HR_SENSOR_ENABLE444");
        BD_16XX_close();
        hrs_enable_swich = 0;
        break;
    case HR_SEARCH_SENSOR:
        BD_heartbp_init();//---算法初始化放在所有函数之前，即第一位
        BD_Sensor_Init();
        char res = 1;
		memcpy(arg, &res, 1);
        break;
    case SPO2_SENSOR_ENABLE:
        printf("SPO2_SENSOR_ENABLE111");
        if(BD_16XX_init_start()){
        printf("[hrs1662]bd 1662 init succese");
        }
        else{
        printf("[hrs1662]bd 1662 init failed !!!");
        }
        break;

    break;
    case SPO2_SENSOR_DISABLE: 
     printf("SPO2_SENSOR_ENABLE222");
     BD_16XX_close();
    break;   
    case SPO2_SENSOR_READ_DATA:
     printf("SPO2_SENSOR_ENABLE333");
    break;   
    default:
        break;
    }
    return 0;
}
/*
int Hrs3605_io_ctl(u8 cmd, void *arg)
{
	//printf("%s cmd = %d",__func__,cmd);
    switch (cmd) {
    case HR_SENSOR_ENABLE://0
#ifdef HRS_ALG_LIB
	if(!!hx3605_hrs_enable())
	{

	    printf("faild");
		break;
	}
	hrs_enable_swich = 1;
#endif
	break;
    case HR_SENSOR_DISABLE://1
#ifdef HRS_ALG_LIB
        hx3605_hrs_disable();
	hrs_enable_swich = 0;

            break;

#endif
        break;
    case SPO2_SENSOR_ENABLE://2
#ifdef SPO2_ALG_LIB
		if(hx3605_spo2_enable())
		{
            printf("faild");
            break;
		}
		spo_enable_swich = 1;
#endif
		break;
    case SPO2_SENSOR_DISABLE://3
#ifdef SPO2_ALG_LIB
        hx3605_spo2_disable();
		spo_enable_swich = 0;

            break;
#endif
    case HR_SENSOR_READ_DATA://4
		//printf("HR_SENSOR_READ_DATA=get_op_hrs_results = %d\n",get_op_hrs_results);
		return 1;
		break;
    case SPO2_SENSOR_READ_DATA://5
        //返回血氧传感器数据
		//printf("SPO2_SENSOR_READ_DATA=get_op_spo2_results = %d\n",get_op_spo2_results);
		return 1;
		break;
    case HR_SEARCH_SENSOR://6
		if (!hx3605_chip_check())
		{
			printf("hx3690l check id failed!\r\n");
		break;
		}
		char res = 1;
		memcpy(arg, &res, 1);

		break;
    default:
        break;
    }
    return 0;
}*/
/*
int BD_16XX_io_ctl(u8 cmd, void *arg)
{
	printf("[hrs1662]%s cmd = %d",__func__,cmd);
    switch (cmd) {
    case HR_SENSOR_ENABLE:
    printf("[hrs1662]1111111111");

        hrsensortimes = arg;
        cbuf_init(&hrsensor_cbuf, hrsensorcbuf, 24 * sizeof(int));
        hrs3603_hr_enable();
        break;
    case HR_SENSOR_DISABLE:
    printf("[hrs1662]222222222");
        hrsensortimes = arg;
        cbuf_clear(&hrsensor_cbuf);
        hrs3603_disable();
        break;
    case SPO2_SENSOR_ENABLE:
    printf("[hrs1662]333333333");
        hrsensortimes = arg;
        cbuf_init(&hrsensor_cbuf, hrsensorcbuf, 24 * sizeof(int));
        hrs3603_spo2_enable();
        break;
    case SPO2_SENSOR_DISABLE:
    printf("[hrs1662]44444444");
        hrsensortimes = arg;
        cbuf_clear(&hrsensor_cbuf);
        hrs3603_disable();
        break;
    case HR_SENSOR_READ_DATA:
        BD_16XX_datapro();
    printf("[hrs1662]555555555");
        //返回心率传感器数据
        return read_hrs3603_result(arg);
        break;
    case SPO2_SENSOR_READ_DATA:
     printf("[hrs1662]66666666");
        //返回血氧传感器数据
        return read_hrs3603_spo2_result(arg);
        break;
    case HR_SEARCH_SENSOR:
     printf("[hrs1662]77777777");
        if (Hrs3603_chip_check_id() == true) {
            char res = 1;
            memcpy(arg, &res, 1);
        } else {
            char res = 0;
            memcpy(arg, &res, 1);
        }
        break;
    default:
        break;
    }
    return 0;
}*/
REGISTER_HR_SENSOR(hrSensor) = {
    .logo = "hx1662",
    .heart_rate_sensor_init = BD_16XX_init_start,
    .heart_rate_sensor_check = NULL,
    .heart_rate_sensor_ctl = BD_16XX_io_ctl,
} ;

static u8 hx1622_idle_query(void)
{
    return 1;
}

REGISTER_LP_TARGET(hx1662_lp_target) = {
    .name = "hx1662",
    .is_idle = hx1622_idle_query,
};
#endif
