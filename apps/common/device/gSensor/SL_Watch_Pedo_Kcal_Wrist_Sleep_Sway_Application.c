//#include "app_config.h"
#include "gSensor/gSensor_manage.h"
#include "gSensor/SL_Watch_Pedo_Kcal_Wrist_Sleep_Sway_Algorithm.h"
#include "printf.h"

u8  volatile  gsensor_sc7a20_timer_en = 1;
static unsigned int gsensor_sc7a20_get_timer_500 = 0;
unsigned char sl_sleep_sys_hour;

unsigned int  SL_SC7A20_PEDO_KCAL_WRIST_SLEEP_SWAY_ALGO(void);

unsigned int  sc7a20_500ms_timer_start(void)
{
	printf("-----%s\n",__func__);

	if(!gsensor_sc7a20_get_timer_500)
	{
		gsensor_sc7a20_get_timer_500 = sys_timer_add(NULL,SL_SC7A20_PEDO_KCAL_WRIST_SLEEP_SWAY_ALGO, 500);
		gsensor_sc7a20_timer_en = 0;
	}
	return 0;
}
unsigned int  sc7a20_500ms_timer_stop(void)
{
	printf("-----%s\n",__func__);

	if(gsensor_sc7a20_get_timer_500)
	{
		sys_timer_del(gsensor_sc7a20_get_timer_500);
		gsensor_sc7a20_timer_en = 1;
		gsensor_sc7a20_get_timer_500 = 0;
		os_time_dly(2);
	}
	return 0;
}

unsigned char sc7a20_500ms_timer_cfg(bool en)
{
	printf("-----%s\n  en = %d",__func__,en);

    if(en)
    {
        sc7a20_500ms_timer_start();
    }
    else
    {
        sc7a20_500ms_timer_stop();
    }
	return 0;
}


unsigned char SL_SC7A20_I2c_Spi_Write(bool sl_spi_iic,unsigned char reg, unsigned char dat)
{
	//printf("%s reg:%d  dat:%d\n",__func__,reg,dat);
	//(void)* sl_spi_iic;
	gravity_sensor_command(SL_SC7A20_IIC_WRITE_ADDRESS, reg, dat);
	//printf("%s==write end\n",__func__);
    return 0;
}
unsigned char SL_SC7A20_I2c_Spi_Read(bool sl_spi_iic,unsigned char reg, unsigned char len, unsigned char *buf)
{
	//printf("%s\n",__func__);

	//(void)* sl_spi_iic;
	 _gravity_sensor_get_ndata(SL_SC7A20_IIC_READ_ADDRESS, reg, buf, len);
	//printf("result = %d %d %d %d %d %d",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
	 return 0;
}


#if SL_Sensor_Algo_Release_Enable==0x00
#include "usart.h"
extern signed short          SL_DEBUG_DATA[10][128];
extern unsigned char         SL_DEBUG_DATA_LEN;
extern unsigned char         sl_sleep_sys_cnt ;
#endif

#if SL_Sensor_Algo_Release_Enable==0x00
extern signed short          SL_ACCEL_DATA_Buf[3][16];
extern unsigned char         SL_FIFO_ACCEL_PEDO_NUM;
signed short                 hp_buf[16];
#endif


#define SL_SC7A20_SPI_IIC_MODE  1
/***SL_SC7A20_SPI_IIC_MODE==0  :SPI*******/
/***SL_SC7A20_SPI_IIC_MODE==1  :IIC*******/

static signed char sl_init_status=0x00;
/**Please modify the following parameters according to the actual situation**/
signed char SL_SC7A20_PEDO_KCAL_WRIST_SLEEP_SWAY_INIT(void)
{
	printf("%s\n",__func__);

#if SL_Sensor_Algo_Release_Enable==0x00
	unsigned char sl_version_value=0x00;
#endif
//	bool  fifo_status       =0;
	unsigned char  sl_person_para[4] ={178,60,26,1};//个人信息参数
	unsigned char  Turn_Wrist_Para[2]={1,3,1};//refer pdf

#if SL_Sensor_Algo_Release_Enable==0x00
	//sl_version_value==0x11  SC7A20
	//sl_version_value==0x26  SC7A20E
	SL_SC7A20_I2c_Spi_Read(1, SL_SC7A20_VERSION_ADDR,1, &sl_version_value);
	printf("Version ID=0x%x!\r\n",sl_version_value);
	if(sl_version_value==SL_SC7A20_VERSION_VALUE)
	printf("SC7A20!\r\n");
	else if(sl_version_value==SL_SC7A20E_VERSION_VALUE)
	printf("SC7A20E!\r\n");
#endif

/*****if use spi mode please config first******/
#if SL_SC7A20_SPI_IIC_MODE == 0//spi
    sl_init_status=SL_SC7A20_Driver_Init(1,0x04);//iic mode pull up config
	//init sensor
	sl_init_status=SL_SC7A20_Driver_Init(0,0x04);//spi or iic select
#else//i2c
 	sl_init_status=SL_SC7A20_Driver_Init(1,0x00);//spi or iic select
	//0x08-->close sdo pull up
#endif
    /**********sc7a20 fifo test***************/
	//fifo_status=SL_SC7A20_FIFO_TEST();

    /**********set pedo sensitivity***********/
    SL_PEDO_TH_SET(28,10,1,30,50);//(32,10,1,30,50)//36 12 0 30 50for motor
	/*which axis used to pedo depend on algo**/
	SL_PEDO_SET_AXIS(3);
    /**********set int para*******************/
    SL_PEDO_INT_SET(2,1,0);//0 or 1 sleep is different
	/**********set motion para****************/
	SL_Pedo_Person_Inf_Init(&sl_person_para[0]);//personal para init
    /**********set turn wrist para************/
    SL_Turn_Wrist_Init(&Turn_Wrist_Para);
    /**********set sleep sensitivity**********/
    SL_Sleep_Para(120,10,1);

    //SL_Pedo_StepCount_Set(SL_INT_TIME_CNT,SL_DISTANCE,SL_DISTANCE);

	return sl_init_status;
}


static unsigned int   SL_STEP                  = 0;
static unsigned int   SL_STEP_TEMP             = 0;
static unsigned char  SL_STEP_MOTION           = 0;
static unsigned int	  SL_DISTANCE              = 0;
static unsigned int	  SL_KCAL                  = 0;
static unsigned int  SL_CADENCE_STEP          = 0;//step per minutes
static unsigned short SL_CADENCE_AMP           = 0;//Equal scale scaling
static unsigned char  SL_CADENCE_DEGREE        = 0;//degree
static signed char    SL_WRIST                 = 0;

static unsigned char  SL_SLEEP_ACTIVE          = 0;
static unsigned char  SL_SLEEP_STATUS          = 0;
static unsigned char  SL_SLEEP_ADOM            = 0;
static unsigned char  SL_SLEEP_CNT             = 0;
static unsigned char  wear_Status             	= 0;

static bool           SL_CLOCK_STATUS          = 0;
static bool           SL_SWAY_STATUS           = 0;
static bool           SL_SWAY_ENABLE           = 0;

static unsigned char  SL_INT_TIME_CNT          = 0;
#define SL_INT_TIME_TH   12  //per lsb is 0.5s
static u32 get_total_step;
unsigned int get_step_count(void)
{
	//printf("SL_CADENCE_STEP/60/2 = %d",SL_CADENCE_STEP/60/2);
	return get_total_step;//SL_CADENCE_STEP/60/2;
}
struct sleep_time{
	u32 sleep_sec;
	u16 sleep_min;
	u8  sleep_hour;
};
u8 get_wrist_detect_result(void)
{
	return SL_CLOCK_STATUS;
}
unsigned char SL_MCU_SLEEP_ALGO_FUNCTION(void);
unsigned int  SL_SC7A20_PEDO_KCAL_WRIST_SLEEP_SWAY_ALGO(void)
{
	//printf("%s\n",__func__);

#if SL_Sensor_Algo_Release_Enable==0x00
	unsigned char         sl_i;
#endif
	signed short          SC7A20_XYZ_Buf[3][14];
	unsigned char         SC7A20_FIFO_NUM;
	bool                  SC7A20_INT_STATUS;

	if((sl_init_status!=SL_SC7A20_VERSION_VALUE)&&(sl_init_status!=SL_SC7A20E_VERSION_VALUE))	return 0;//initial fail

    /*******read int status******/
	SC7A20_INT_STATUS=SL_INT_STATUS_READ();
	//printf("111111SC7A20_INT_STATUS = %d",SC7A20_INT_STATUS);
	if((SC7A20_INT_STATUS==1)||(SL_STEP_TEMP!= SL_STEP))
	{
		//printf("22222SL_STEP = %d",SL_STEP);
		SL_INT_TIME_CNT =0;
		SL_STEP_TEMP    =SL_STEP;
	}
	else
	{
		//printf("333333SL_INT_TIME_CNT = %d",SL_INT_TIME_CNT);
		if(SL_INT_TIME_CNT<200)
		{
			SL_INT_TIME_CNT++;
		}
	}
	    /*******get wrist value*****获得手腕的值*/
	    SL_WRIST= SL_Watch_Wrist_Algo();
		//printf("eeeeeee-SL_WRIST = %d",SL_WRIST);

	    /*******get overturn value*****获取翻转的值*/
	    SL_CLOCK_STATUS=SL_Get_Clock_Status(1);//open overturn monitor
		//printf("SL_CLOCK_STATUS = %d",SL_CLOCK_STATUS);
	    //printf("fffffff-SL_CLOCK_STATUS = %d",SL_CLOCK_STATUS);
	    if(SL_CLOCK_STATUS==1)//overturn success
	    {
	        SL_Get_Clock_Status(0);//close overturn monitor
	    }
	if(SL_INT_TIME_CNT<SL_INT_TIME_TH)//6s
	{
	    /*******get sc7a20 FIFO data*****FIFO数据*/
		SC7A20_FIFO_NUM= SL_SC7A20_Read_FIFO();
        /*******get pedo value***********执行步数*/
	    SL_STEP= SL_Watch_Kcal_Pedo_Algo(0);//不打开马达或音乐时调用该函数
	    //SL_STEP= SL_Watch_Kcal_Pedo_Algo(1);//打开马达或音乐时调用该函数
	    /*********get sleep active degree value*******获得睡眠活跃度值**/
		SL_SLEEP_ACTIVE= SL_Sleep_Active_Degree(0);
	    /*******get accel orginal data and length******获取加速原始数据和长度*/
	    SC7A20_FIFO_NUM      = SL_SC7A20_GET_FIFO_Buf(&SC7A20_XYZ_Buf[0][0],&SC7A20_XYZ_Buf[1][0],&SC7A20_XYZ_Buf[2][0],1);
		//printf("SC7A20_XYZ_Buf[0][0]  SC7A20_XYZ_Buf[1][0] SC7A20_XYZ_Buf[2][0] %d %d %d",SC7A20_XYZ_Buf[0][0],SC7A20_XYZ_Buf[1][0],SC7A20_XYZ_Buf[2][0]);
		/* SL_Pedo_GetMotion_Status计步动作状态 */
		SL_STEP_MOTION       = SL_Pedo_GetMotion_Status();
		/* SL_Pedo_Step_Get_Distance计步距离 */
		SL_DISTANCE          = SL_Pedo_Step_Get_Distance();
		/* SL_Pedo_Step_Get_KCal ==热量（千卡）*/
		SL_KCAL              = SL_Pedo_Step_Get_KCal();
		/*****average step per minutes***每分钟平均多少步*/
		SL_CADENCE_STEP      = SL_Pedo_Step_Get_Step_Per_Min();
		get_total_step += SL_CADENCE_STEP/60/2;
		/*****average amp per step***每阶平均放大器*/
		SL_CADENCE_AMP       = SL_Pedo_Step_Get_Avg_Amp();
		/*****motion degree***运动程度*/
		SL_CADENCE_DEGREE    = SL_Pedo_Step_Get_Motion_Degree();
		// printf("44444SC7A20_FIFO_NUM = %d",SC7A20_FIFO_NUM);
		// printf("55555SSL_STEP = %d",SL_STEP);
		// printf("666666_SL_SLEEP_ACTIVE = %d",SL_SLEEP_ACTIVE);
		// printf("777777-SC7A20_FIFO_NUM = %d",SC7A20_FIFO_NUM);
		// printf("888888-SL_STEP_MOTION = %d",SL_STEP_MOTION);
		// printf("9999999-SL_DISTANCE = %d",SL_DISTANCE);
		// printf("aaaaaa-SL_KCAL = %d",SL_KCAL);
		// printf("bbbbbb-SL_CADENCE_STEP = %d",SL_CADENCE_STEP);
		// printf("cccccc-SL_CADENCE_AMP = %d",SL_CADENCE_AMP);
		// printf("ddddddd-SL_CADENCE_DEGREE = %d",SL_CADENCE_DEGREE);

//	    if(SL_STEP>200)
//	    {
	        /**reset pedo value**/
//	        SL_Pedo_Kcal_ResetStepCount();
//	    }



	    /*******get sway value*****获取摇摆的值*/
	    if(SL_SWAY_ENABLE==1)
	    {
	        /**this function will disable pedo function**/
	        SL_SWAY_STATUS=SL_Get_Phone_Answer_Status(4,5);//get sway value
	        //printf("ggggggggg-SL_SWAY_STATUS = %d",SL_SWAY_STATUS);
	    }
	}
	else
	{
	    //sc7a20 's data for heart rate algo
		//SC7A20_FIFO_NUM= SL_SC7A20_Read_FIFO();
	    //SC7A20_FIFO_NUM= SL_SC7A20_GET_FIFO_Buf(&SC7A20_XYZ_Buf[0][0],&SC7A20_XYZ_Buf[1][0],&SC7A20_XYZ_Buf[2][0]);
		//SL_Turn_Wrist_Reset();
		SC7A20_FIFO_NUM=0;
	}

	/*******get sleep status value******/
	SL_SLEEP_CNT++;
#if SL_Sensor_Algo_Release_Enable==0x00
	if(SL_SLEEP_CNT>39)//1min
#else
	if(SL_SLEEP_CNT>119)//1min
#endif
	{
#if SL_Sensor_Algo_Release_Enable==0
		sl_sleep_sys_cnt++;
		if(sl_sleep_sys_cnt>59)//min
		{
			sl_sleep_sys_cnt=0;
			sl_sleep_sys_hour++;//hour
			if(sl_sleep_sys_hour==24)
				sl_sleep_sys_hour=0;
		}
#endif
		SL_SLEEP_CNT=0;
		//printf("timer = %d sleep=%d",sl_sleep_sys_hour,SL_MCU_SLEEP_ALGO_FUNCTION());
	}

#if SL_Sensor_Algo_Release_Enable==0x00
	SL_DEBUG_DATA_LEN=SC7A20_FIFO_NUM;
	for(sl_i=0;sl_i<SL_DEBUG_DATA_LEN;sl_i++)
	{
		SL_DEBUG_DATA[0][sl_i]=SC7A20_XYZ_Buf[0][sl_i];
		SL_DEBUG_DATA[1][sl_i]=SC7A20_XYZ_Buf[1][sl_i];
		SL_DEBUG_DATA[2][sl_i]=SC7A20_XYZ_Buf[2][sl_i];
		SL_DEBUG_DATA[3][sl_i]=SL_STEP;
		SL_DEBUG_DATA[4][sl_i]=SL_WRIST;
//        SL_DEBUG_DATA[4][sl_i]=SL_CLOCK_STATUS;
//		SL_DEBUG_DATA[5][sl_i]=SL_SLEEP_STATUS;
        SL_DEBUG_DATA[5][sl_i]=SL_SWAY_STATUS;
		SL_DEBUG_DATA[6][sl_i]=SL_STEP_MOTION;
		SL_DEBUG_DATA[7][sl_i]=SL_DISTANCE;
//		SL_DEBUG_DATA[7][sl_i]=hp_buf[sl_i];
		SL_DEBUG_DATA[8][sl_i]=SL_KCAL;
		SL_DEBUG_DATA[9][sl_i]=SL_CADENCE_STEP;
//        SL_DEBUG_DATA[8][sl_i]=SL_CADENCE_AMP;
//        SL_DEBUG_DATA[9][sl_i]=SL_CADENCE_DEGREE;
	}
#endif
//printf("return SL_STEP = %d",SL_STEP);

    return SL_STEP;
}

#if SL_Sensor_Algo_Release_Enable==0x00
unsigned short sl_sleep_counter=0;
#endif

unsigned char get_sleep_status(void)
{
	return SL_SLEEP_STATUS;
}

static u16 slight_sleep_time1;
static u16 slight_deep_sleep_time1;
static u16 slight_no_sleep_time1;
unsigned char slight_sleep_time(void)
{
	return slight_sleep_time1++;
}
unsigned char slight_deep_sleep_time(void)
{
	return slight_deep_sleep_time1++;
}
unsigned char slight_no_sleep_time(void)
{
	return slight_no_sleep_time1++;
}
#define SL_SLEEP_DEEP_TH   6
#define SL_SLEEP_LIGHT_TH  3
/***Call this function regularly for 1 minute***/
unsigned char SL_MCU_SLEEP_ALGO_FUNCTION(void)
{
	//printf("%s\n",__func__);

	/*******get sleep status value******/
	SL_SLEEP_STATUS= SL_Sleep_GetStatus(sl_sleep_sys_hour);//获取睡眠状态结果
	//printf("SL_MCU_SLEEP_ALGO_FUNCTION SL_SLEEP_STATUS = %d",SL_SLEEP_STATUS);
	SL_SLEEP_ACTIVE= SL_Sleep_Get_Active_Degree();//获取睡眠运抵幅度值
	//printf("SL_MCU_SLEEP_ALGO_FUNCTION SL_SLEEP_ACTIVE = %d",SL_SLEEP_ACTIVE);
	SL_SLEEP_ADOM  = SL_Adom_GetStatus();//获取睡眠算法佩戴结果
	printf("SL_SLEEP_STATUS:%d SL_SLEEP_ACTIVE %d SL_SLEEP_ADOM %d",SL_SLEEP_STATUS,SL_SLEEP_ACTIVE,SL_SLEEP_ADOM);

#if SL_Sensor_Algo_Release_Enable==0x00
	sl_sleep_counter++;
	printf("step=%d! sys_time=%d!\r\n",SL_STEP,sl_sleep_sys_hour);
	printf("T=%d,sleep_status:%d,sleep_adom:%d!\r\n",sl_sleep_counter,SL_SLEEP_STATUS,SL_SLEEP_ADOM);
	printf( "SL_SLEEP_ACTIVE:%d!\r\n",SL_SLEEP_ACTIVE);
#endif

	if(SL_SLEEP_STATUS<SL_SLEEP_LIGHT_TH)
	{
		slight_no_sleep_time1++;
		return 0;//0 1 2 3
	}
	else if(SL_SLEEP_STATUS<SL_SLEEP_DEEP_TH)
	{
		slight_sleep_time1++;
		return 1;//4 5 6
	}
	else
	{
		slight_deep_sleep_time1++;
		return 2;//7
	}
}
char sc7A20_driver_init(void)
{
	printf("%s\n",__func__);
	u8 ret = SL_SC7A20_PEDO_KCAL_WRIST_SLEEP_SWAY_INIT();
	printf("ret = %d",ret);
	if(ret==38){
		sc7a20_500ms_timer_cfg(true);
		printf("timer==is===start");
		return 0;
	}else printf("sc7A20_driver_init fail??? ");

}
void gsensor_sc7a20_ctl(u8 cmd, void *arg)
{
	printf("%s %d\n",__func__,cmd);
	char res;
    switch (cmd) {
    case GSENSOR_DISABLE:
        res = sc7a20_500ms_timer_cfg(false);
        memcpy(arg, &res, 1);
        break;
    case GSENSOR_RESET_INT:
        res = sc7A20_driver_init();
        memcpy(arg, &res, 1);
        break;
    case GSENSOR_RESUME_INT:
        break;
    case GSENSOR_INT_DET:
        //SC7A20_int_io_detect(*(u8 *)arg);
        break;
    case READ_GSENSOR_DATA:
		res = sc7a20_500ms_timer_cfg(true);
        //SC7A20_read_data((axis_info_t *)arg);
        break;
    case SEARCH_SENSOR:
        res = sc7A20_driver_init();
        memcpy(arg, &res, 1);
        break;
    default:

        break;
    }



}

static u8 gsensor_sc7a20_idle_query(void)
{
    return gsensor_sc7a20_timer_en;
}

REGISTER_GRAVITY_SENSOR(gSensor) = {
    .logo = "sc7a20",
    .gravity_sensor_init  =sc7A20_driver_init,//SC7A20_Config,
    .gravity_sensor_check = NULL,//SL_Pedo_GetMotion_Status,//NULL,//SC7A20_click_status,
    .gravity_sensor_ctl   = gsensor_sc7a20_ctl,//NULL,//SC7A20_ctl,NULL,//
};

REGISTER_LP_TARGET(sc7a20_lp_target) = {
    .name = "sc7a20",
    .is_idle = gsensor_sc7a20_idle_query,
};


