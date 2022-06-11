/******************************************************************************/
#ifndef _DEF_BD_HEALTH_H_
#define _DEF_BD_HEALTH_H_

/*******自定义函数**********/
/*
uint8_t get_hrs_results(void);
uint8_t get_spo_result(void);
uint8_t get_spo2_wear_results(void);
uint8_t get_hrs_wear_results(void);
uint8_t get_hrs_enable_status(void);
uint8_t get_spo_enable_status(void);
*/
//////***************define const value**********/////////
//////*************define data type****************////////
/*
typedef   signed char     int8_t;
typedef   signed short    int16_t;
typedef   signed int     int32_t;
typedef unsigned char     uint8_t;
typedef unsigned short    uint16_t;
typedef unsigned int     uint32_t;

typedef int32_t  s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;
#ifndef bool
#define bool unsigned char

#endif
*/
///////////-----------------函数说明------------------////////
///////--------需要用户自行定义外部函数说明----------/////////
extern uint16_t bp_data_up,bp_data_down,bp_difdata;
///////******时间延迟函数说明--START********/////////
////------函数介绍:
///---时间延迟函数，需要客户提供
///---形参说明：delay_time为需要延迟的时间数值(单位ms)
extern void Delay_ms(u16 delay_time);
//////*********时间延迟函数--END*************///////

///////******PWM-LED驱动开启函数--START********/////////
////------函数介绍:
///---开启LED的PWM驱动，高电平无效，低电平有效
///---形参说明：duty_data为PWM驱动信号有效电平的占空比(单位%)
extern void LED_PWM_On(uint8_t duty_data);
//////*********PWM-LED驱动函数--END*************///////

///////******PWM-LED驱动关闭函数--START********/////////
////------函数介绍:
///---关闭LED的PWM驱动
extern void LED_PWM_Off(void);
//////*********PWM-LED驱动函数--END*************///////

//////******I2C通讯写函数说明--START*********///////
////------函数介绍：
///---库文件中部分函数需要调用客户芯片的I2C接口(客户芯片的I2C为主机)向传感器(从机)发送命令，
//需要客户依据自己芯片硬件来编写此I2C写函数；
///---形参说明：slave_address为外设传感器从机地址，cmd_reg为将要填写数据的寄存器地址或者其他命令标识
//data为要填写的数据
///---返回值说明：1---写OK，0---写失败
extern uint8_t BD_I2C_Write(uint8_t slave_address,uint8_t cmd_reg,uint8_t data);
//////************I2C写函数--END*************///////

//////******I2C通讯读函数说明--START*********///////
////------函数介绍：
///---库文件中部分函数需要调用客户芯片的I2C接口(客户芯片I2C为主机)从传感器(从机)读取数据，
//需要客户依据自己芯片硬件来编写此I2C读函数；
///---形参说明：slave_address为外设传感器从机地址，cmd_reg为将要读数据的寄存器地址，
//state_commu指针指向存储通讯结果状态的变量地址(1--通讯读取成功，0--通讯读取失败)
///---返回值说明：所读取寄存器中的数值
extern uint8_t BD_I2C_Read(uint8_t slave_address,uint8_t cmd_reg,uint8_t *state_commu);
//////************I2C读函数--END*************///////
//////----------用户自定义外部函数说明--END-----------//////////

//////////---------供客户使用的算法库函数说明--START-----------////////////

//////*******算法初始化函数--START******/////////
////------函数介绍：
///---算法库参数初始化
///---放入位置：在整个客户系统上电后进入主程序之前和运行其他接口函数之前，
//每次断电后重新上电均要运行一次此函数
void BD_heartbp_init(void);
//////***********函数--END*********///////////

///////************传感器初始化函数*************//////////
///---返回值：1--初始化OK，0--初始化失败
//---位置：每次传感器断电后重新上电后均要执行一次此函数（此函数必须放在除BD_heartbp_init(void)函数之外的其他函数之前）
uint8_t BD_Sensor_Init(void);
//////***********函数--END*********///////////


///////*********开启传感器量测**********//////////
///---返回值：1--开启OK，0--开启失败
uint8_t BD_Sensor_Start(void);
//////***********函数--END*********///////////

///////**********关闭传感器量测**********//////////
///---返回值：1--关闭OK，0--关闭失败
uint8_t BD_Sensor_Close(void);
//////***********函数--END*********///////////

///////**********定时(50毫秒)读取传感器数据**********//////////
///---形参说明：in_buf数组指针，存储从传感器读取的量测数据
///---返回值说明：
//    0--表示一帧数据尚未读取完毕，
//    1--表示一帧数据读取完毕，
//    2--表示通讯异常
uint8_t BD_Sensor_Data_Read(uint16_t *in_buf);
//////***********定时读取函数--END*********///////////


//////***********数据处理函数START********///////////
////------形参参数说明：
//--cmd_ack:量测对象参数(8-心率，血压，血氧)
//--rxdata_buf：接收数据组存储指针地址
//--bd_heartbp_buf: 输出数据组存储指针地址

////-----返回值说明：
//--7：算法处理中
//--8: 算法处理完毕，[0]-心率,[1]-高压,[2]-低压,[3]-血氧,[4]-脱手状态，[5]~[14]为调试参数
//--14：错误数据或者无效对象数据
///---放入位置：建议放在主循环程序里面而不是中断子程序里面(算法执行一次的时间较长，因此不建议放在中断里面)，当接收到一组数据后才执行此函数
uint8_t BD_health_pro(uint8_t cmd_ack,uint16_t*rxdata_buf,uint32_t *bd_heartbp_buf);
//////*********数据处理函数END**************////////////
/////-----------------库函数说明END-----------/////////
void BD_16XX_datapro(void);
void BD_16XX_dataread(void);
void BD_16XX_close(void);
uint8_t BD_16XX_init_start(void);
void bd_50ms_timer_cfg(bool en);
void bd_timers_stop(void);
void bd_timers_start(void);
//1.50ms 硬件定时器
//调试上传值 手机
//

#endif
//\************************END OF FILE***********************\//
