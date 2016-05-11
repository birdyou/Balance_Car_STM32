
#include "app.h"
#include "cmsis_os.h"
#include "stm32_bluenrg_ble.h"
#include "motion.h"
/*start adv*/

#if 0 //NO_PRINTF
#define printf(...)
#endif

/*
ң����������
0     ID    01
1     X��
2     Y��
3     ģʽ������1,�������ͨ
4     ģʽ������2����̬��ҡ��
5			�ֻ������8�ֽ�
6     �ֻ������8�ֽ�
7			���1
8			���2
9			���3
10		���4
11		���5
12		CheckSum
*/
#define REMOTE_CONTROL_ID 					0x01
#define REMOTE_CONTROL_LONG 				12

/*
�޸Ĳ���
0     ID    09
1~16  ����1~8��8λ���8λ
17		CheckSum
*/
#define PARAMETER_MODIFY_ID 				0x09
#define PARAMETER_MODIFY_LONG 			17

/*
��ȡԭʼ����
0     ID    08
1			CheckSum
*/
#define PARAMETER_GET_ORIGIN_ID   	0x08
#define PARAMETER_GET_ORIGIN_LONG 	1

/*
����ԭʼ����
0     ID    07
1~16  ����1~8��8λ���8λ
17			CheckSum
*/
#define PARAMETER_PUT_ORIGIN_ID   	0x07
#define PARAMETER_PUT_ORIGIN_LONG 	17

/*
���ֲ���ģʽ
0     ID    02
1			ģʽ
2			������Ŀ
3			CheckSum
*/
#define MUSIC_PLAYER_MODE_ID     0x02
#define MUSIC_PLAYER_MODE_LONG   3
/*
������С
0     ID    03
1			������С
2			CheckSum
*/
#define MUSIC_PLAYER_VOLUME_ID     0x03
#define MUSIC_PLAYER_VOLUME_LONG   2
/*
��ѹ��С
0     ID    04
1~2		��ѹ��8λ���8λ
3			CheckSum
*/
#define VOLTAGE_ID     0x04
#define VOLTAGE_LONG   3


#define ADC_to_VOLTAGE   0.004462f//ADCת��Ϊ��ѹֵ

static void adv_name_generate(uint8_t* uni_name);

extern uint8_t Volume;
uint8_t Volume_Pre;
uint8_t Music_Mode;
uint8_t Music_Mode_List_Ptr;

AxesRaw_TypeDef XData,GData;
#ifdef CANNON_V2
char name[20] = "CannonRobot";
#endif
extern float speed_A;//�ٶȺ�
extern int32_t speed_L;//��ߵ���ٶ�
extern int32_t speed_R;//�ұߵ���ٶ�
uint32_t adc_value;//ADCֵ�������ѹ
extern imu_sensor_raw_data_t sensor_saw_data;//IMU�ʹ�����ԭʼֵ
extern imu_sensor_data_t sensor_data;//У׼ת�����ֵ��Offset��MyOffset����
extern imu_euler_data_t sensor_euler_angle;//ŷ����
extern uint16_t MData[3];
extern float r_pitch;//pitch�ķ�תֵ
extern int16_t motor1_output;//���1�����ֵ��-1000~1000
extern int16_t motor2_output;//���2�����ֵ��-1000~1000
extern float Speed_Kp,Speed_Ki;	
extern float Angle_Kp,Angle_Kd;	
extern float Turn_Kp;							//ת�����P
extern float Car_Angle_Center;			//ƽ���Ƕ�
extern int8_t remote_control_X,remote_control_Y;
extern int16_t motor_output_Speed;
extern int16_t motor_output_temp;
extern int16_t motor_output_Angle;
extern  int8_t Flag_Fall;
extern float speed_target;
extern float 	turn_target_speed;
extern float 	turn_target_orientaion;
extern int8_t	trun_mode;
extern int16_t motor_output_Turn;	
extern int fifo_length;
	
extern   TIM_HandleTypeDef        TimHandleT3;//���
extern   TIM_HandleTypeDef        TimHandleT9;//���
extern __IO uint32_t PauseResumeStatus;
extern char USBDISKPath[4];  
extern FATFS USBDISKFatFs;   
extern  MSC_ApplicationTypeDef AppliState;
extern __IO uint32_t RepeatState;
extern __IO uint32_t AudioPlayStart ;
extern 	float heading;
extern IMU_Offset MyOffset;
extern int16_t steer_out[2][5];
extern AUDIO_PLAYBACK_StateTypeDef AudioState;
extern Music_List MusicList;//�̶������б�
uint8_t Dance_Mode_Flag=0;
uint8_t Dance_Mode_Flag_Pre;
uint8_t Read_Temp_Flag=0;
extern int16_t Temperature;
int temp=0;//�������ʹ�õ�ȫ�ֱ���
int temp2=0;
 xQueueHandle  TXQueue;				//�����������͵��ж�
  osMutexId osMutexSPI; 		//����SPI�������뺯���Ļ�����
  osMutexId osMutexIIC; 					//����IIC�������뺯���Ļ�����
  osSemaphoreId osSemaphore_MWMS_EXTI;		//�����жϵ��ź���
  osSemaphoreId osSemaphore_SPI;		//�����жϵ��ź���
	
extern	int steer[3];
typedef struct 
 {
	uint8_t type;
	uint8_t length;
	uint8_t value[18];
 }BLEMessage;
 
   static void InitThread(void const *argument)
{    
		uint8_t tx_power_level = 7;
    uint16_t adv_interval = 100;
    uint8_t bdAddr[6];
  
		 /* Initialize the BlueNRG SPI driver */
    BNRG_SPI_Init();
    /* Initialize the BlueNRG HCI */
    HCI_Init();
    /* Reset BlueNRG hardware */
    BlueNRG_RST();
    /*Gatt And Gap Init*/
    ble_init_bluenrg();
    HCI_get_bdAddr(bdAddr);
    adv_name_generate(bdAddr+4);

    ble_set_adv_param(name, bdAddr, tx_power_level, adv_interval);
    ble_device_start_advertising();
		vTaskDelete(NULL);
}
 
 
  static void SPIISRhread(void const *argument)
{  
  for(;;)
  {
		if(xSemaphoreTake( osSemaphore_SPI, portMAX_DELAY )==pdTRUE)
      {
				 if(osMutexWait(osMutexSPI, osWaitForever) == osOK)
				   HCI_Isr();
				 osMutexRelease(osMutexSPI);
      }
  }
}
/*
 static void CarControlhread(void const *argument)
{ 
  for(;;)
  {
		if(xSemaphoreTake( osSemaphore_MWMS_EXTI, portMAX_DELAY )==pdTRUE)
      {
				 if(osMutexWait(osMutexIIC, osWaitForever) == osOK)
				imu_sensor_read_data_from_fifo_DMA();
				   osMutexRelease(osMutexIIC);
      }
  }
}
 */
static void ToggleLEDsThread(void const *argument)
{
  for(;;)
  {
    BSP_LED_Toggle(LED0);
    osDelay(300);
  }
}


static void HeartBeatthread(void const *argument)
{
	BLEMessage RXMessage;
	int8_t send_temp[4]={0};//��������
	int16_t p16Data[1] = {0};//���ͻ���յ�temp
	for(;;){
		 RXMessage.type=(uint8_t)0x01;
			RXMessage.length=VOLTAGE_LONG+1;
		send_temp[0]=VOLTAGE_ID;
		p16Data[0]=(int16_t)adc_value*ADC_to_VOLTAGE*100.0f;
		temp2=p16Data[0];
			for(int i=0;i<2;i++){
       send_temp[i+1]=(int8_t)(p16Data[i/2]>>(8-(i%2)*8));
       }
			send_temp[3]=send_temp[0]+send_temp[1]+send_temp[2];
			memcpy(RXMessage.value, (uint8_t*)send_temp, RXMessage.length);
			if( TXQueue != 0 &&Ble_conn_state==BLE_NOCONNECTABLE)
			{
			xQueueSend( TXQueue, ( void* )&RXMessage, 0 );  
			}
		 	osDelay(2200);	
	}
}
static void BLEThread(void const *argument)
{
	for(;;){
		 if(osMutexWait(osMutexSPI, osWaitForever) == osOK){
			HCI_Process();
			}
		  osMutexRelease(osMutexSPI);
		if(Ble_conn_state) {
		Ble_conn_state = BLE_NOCONNECTABLE;
     }
	}
}

uint16_t	nowTick=0;
	uint16_t preTick=0;
	uint16_t cha;
extern  int my_cnt;
static void mainThread(const void *argument){
		TickType_t xLastWakeTime;
		const TickType_t xFrequency = 5;
    xLastWakeTime = xTaskGetTickCount ();
		for( ;; )
		{
			vTaskDelayUntil( &xLastWakeTime, xFrequency );
			Get_Adc(&adc_value);	
			nowTick = HAL_GetTick();
			cha=nowTick-preTick;	
			if (cha>100)cha=5;
			getYaw(cha);
		//	get_heading();
			preTick=nowTick; 
			
			if(Dance_Mode_Flag){
				steer_out[1][0]=sensor_euler_angle.yaw/1.8f;
				steer_out[1][1]=speed_A;
				steer_out[1][2]=motor_output_Turn;
				
			}
			else if((Dance_Mode_Flag==0)&&(Dance_Mode_Flag_Pre==1)){
				
				steer_out[1][0]=0;
				steer_out[1][1]=0;
				steer_out[1][2]=0;
			}
			Dance_Mode_Flag_Pre=Dance_Mode_Flag;
			Steer_Control(steer_out);
				
OutData[0] = sensor_saw_data.gyro[0];
OutData[1] = sensor_saw_data.gyro[1];
OutData[2] = sensor_saw_data.gyro[2];	

//OutData[2] =MyOffset.G_X;
//OutData[3] = MyOffset.G_Y;

//OutData[0]=MData[0];
//OutData[1]=MData[1];
//OutData[2]=MData[2];				
//OutData[0]=steer_out[0][0];
//OutData[1]=steer_out[0][1];
//OutData[2]=steer_out[0][2];

//OutData[2] = fifo_length;
//OutData[3] = cha;	
//OutData[0] = sensor_euler_angle.pitch;
//OutData[0] =sensor_euler_angle.pitch*100;
		//	OutData[1] =my_cnt;
	//	OutData[1] =	sensor_saw_data.gyro[0];
		//OutData[3] =	sensor_data.gyro[0]*100;
		//		OutData[2] =	sensor_saw_data.acc[1];
		//	OutData[3] =	sensor_saw_data.acc[0];
//OutData[0] =motor_output_temp;
//OutData[1] = turn_target_speed;
//OutData[2] =motor_output_Speed;
//OutData[1] = adc_value;
//OutData[2] = speed_R;
//OutData[3] = speed_target;
//OutData[3] = cha;
//	OutPut_Data();		
	//	BSP_IMU_6AXES_X_GetAxesRaw(&XData);
//	BSP_IMU_6AXES_G_GetAxesRaw(&GData);
 //LSM303AGR_MAG_Get_Raw_Magnetic((u8_t*)MData);

//OutData[0] = sensor_saw_data.gyro[0];
//OutData[1] = sensor_saw_data.gyro[1];
//OutData[2] = sensor_saw_data.gyro[2];
//OutData[3] = sensor_saw_data.gyro[1];	
//OutData[0]=motor1_output;
//OutData[1]=motor_output_Angle;
OutData[2]=sensor_euler_angle.yaw*100;
OutData[3]=sensor_euler_angle.pitch*100;
//OutData[3]=motor_output_Speed;

//OutData[0]=speed_target;
//OutData[1]=turn_target_speed;
//OutData[2]=sensor_data.mag[0];
//OutData[0]=my_raw;
//OutData[1]=sensor_data.mag[1];
//OutData[2]=adc_value;

//OutData[0]=my_cnt;

//OutData[2]= heading*10;
//OutData[3]= sensor_euler_angle.pitch*10;
	OutPut_Data();
		}
}
static void BLEMessageQueueConsumer (const void *argument)
{
  BLEMessage pxMessage;
  for(;;)
  {	
   if( TXQueue != 0 )
		{
		if( xQueueReceive( TXQueue,  ( void* )&pxMessage, 10 ) )
			{
				 if(osMutexWait(osMutexSPI, osWaitForever) == osOK)
				ble_device_send(pxMessage.type,pxMessage.length,pxMessage.value);				 
				 osMutexRelease(osMutexSPI);
			}
		}
		osDelay(50);	
  }
}

static void MusicPlayThread (const void *argument)
{
  for(;;)
  {	
		AUDIO_PLAYER_Process();
	}
}


static void MusicControlThread (const void *argument)
{
  for(;;)
  {	
		if(Volume!=Volume_Pre){
			BSP_AUDIO_OUT_SetVolume(SOUNDTERMINAL_DEV1,  STA350BW_CHANNEL_MASTER ,Volume);
			Volume_Pre=Volume;
		}
		if(Music_Mode!=0){
			switch (Music_Mode){
			case 1:   //���Ű�ť
					if(AudioState ==AUDIO_STATE_WAIT){
					
					AudioState = AUDIO_STATE_RESUME;
				}		
					break;
			case 2:
				if(AudioState == AUDIO_STATE_PLAY)
				{
					AudioState = AUDIO_STATE_PAUSE;
				}		
					break;
			case 4:
				WavePlayerStop();
				if(Music_Mode_List_Ptr!=4){
				MusicList.Music_List_All=1;
			  MusicList.Music_List_Ptr=0;
				MusicList.list[MusicList.Music_List_Ptr]=Music_Mode_List_Ptr;//
			  AUDIO_PLAYER_Start(MusicList.list[MusicList.Music_List_Ptr]);
			
				}
				else{
					if(Dance_Mode_Flag==0){
						Dance_Mode_Flag=1;
							MusicList.Music_List_All=1;
							MusicList.Music_List_Ptr=0;
							MusicList.list[MusicList.Music_List_Ptr]=Music_Mode_List_Ptr;//
							AUDIO_PLAYER_Start(MusicList.list[MusicList.Music_List_Ptr]);
					}
					else {
						Dance_Mode_Flag=0;
							AudioState = AUDIO_STATE_STOP;			
					}
				}
			
				break;
			case 5:  //����ť
				if(AudioState == AUDIO_STATE_PLAY){
				AudioState = AUDIO_STATE_STOP;//ֹͣ
				}
				else{
					//��һ��
				}	
					break;
			case 6:  //���Ұ�ť
				//��һ��
				break;
			case 7:  //ѭ��ģʽ��ť
				
				break;
			}	
		Music_Mode=0;;
		}
		osDelay(50);	
	}
}
void on_ready(void)
{
    uint32_t data_rate = 400;
	
//HAL_Delay(100);
	/* Initialize STA350BW */
 // Init_AudioOut_Device();
  
  /* Start Audio Streaming*/
 // Start_AudioOut_Device();  
	FATFS_LinkDriver(&SD_Driver, USBDISKPath);
	 /* FatFs initialization fails */
  if(f_mount(&USBDISKFatFs, (TCHAR const*)USBDISKPath, 1 ) == FR_OK )  {
       AppliState = APPLICATION_START;
			AUDIO_PLAYER_Start(0);//���Ż�ӭ����
	}
    imu_sensor_select_features(ALL_ENABLE);
	  jsensor_app_set_sensor(JSENSOR_TYPE_HUMITY_TEMP);

   imu_sensor_reset();

    imu_sensor_set_data_rate(&data_rate, LSM6DS3_XG_FIFO_MODE_CONTINUOUS_OVERWRITE);

	 // imu_sensor_filter();
	
    imu_sensor_start();

		Motor_Pwm_Init();
		Encoder_Init();         
		Steer_Pwm_Init();
		Adc_Init();
	
		osMutexDef(osMutexSPI);//����SPI�Ƿǿ����뺯����������Ҫʹ�û����ź�
		osMutexSPI = osMutexCreate(osMutex(osMutexSPI));
	
	  osMutexDef(osMutexIIC);//����IIC�Ƿǿ����뺯����������Ҫʹ�û����ź�
		osMutexIIC = osMutexCreate(osMutex(osMutexIIC));
	
		TXQueue=xQueueCreate( 5 , 20 );//���������жӣ�����ͳһ����

		osSemaphoreDef(SEM);
		osSemaphore_MWMS_EXTI = osSemaphoreCreate(osSemaphore(SEM) , 1);
		osSemaphoreDef(SEM1);
		osSemaphore_SPI = osSemaphoreCreate(osSemaphore(SEM1) , 1);
	
		osThreadDef(BLE, BLEThread, osPriorityIdle, 0, 2*configMINIMAL_STACK_SIZE);//�������գ���͵����ȼ�
		osThreadCreate(osThread(BLE), NULL);
	
		osThreadDef(QCons, BLEMessageQueueConsumer, osPriorityAboveNormal, 0, 3*configMINIMAL_STACK_SIZE);//�������ͣ�����ж�
		osThreadCreate(osThread(QCons), NULL);
	
		if( AppliState == APPLICATION_START){
			osThreadDef(MusicPlayThread, MusicPlayThread, osPriorityIdle, 0, 3*configMINIMAL_STACK_SIZE);//���ֲ����������߳�
			osThreadCreate(osThread(MusicPlayThread), NULL);
	  
			osThreadDef(MusicControlThread, MusicControlThread, osPriorityAboveNormal, 0, 2*configMINIMAL_STACK_SIZE);//���ֲ����������߳�,�Ȳ����߳����ȼ���
			osThreadCreate(osThread(MusicControlThread), NULL);
		}
		else{//�������ʹ�������ֲ�����ô�Ͳ��ܿ�LED�ˣ�PB3������IIC2�еĵ�CLK��
			osThreadDef(uLEDThread, ToggleLEDsThread, osPriorityNormal, 0,configMINIMAL_STACK_SIZE);//ָʾ�ƣ����ڵ���˸
			osThreadCreate(osThread(uLEDThread), NULL);
		}
			osThreadDef(HeartBeatThread, HeartBeatthread, osPriorityLow, 0,configMINIMAL_STACK_SIZE);//�������񣬵�Ƶ�ʵķ��������Ϣ���ֻ�
			osThreadCreate(osThread(HeartBeatThread), NULL);
	
			osThreadDef(mainThread, mainThread, osPriorityHigh, 0,2*configMINIMAL_STACK_SIZE);//��������ʱ�������ȼ�
			osThreadCreate(osThread(mainThread), NULL);
	
			//osThreadDef(CarControlhread, CarControlhread, osPriorityRealtime, 0,3*configMINIMAL_STACK_SIZE);//MEMS�жϣ�������ȼ�
			//osThreadCreate(osThread(CarControlhread), NULL);
	
			osThreadDef(SPIISRhread, SPIISRhread, osPriorityRealtime,0,3*configMINIMAL_STACK_SIZE);//SPI�жϣ�������ȼ�
			osThreadCreate(osThread(SPIISRhread), NULL);
	
			osThreadDef(InitThread, InitThread, osPriorityHigh, 0,3*configMINIMAL_STACK_SIZE);//BLE��ʼ������,Ҫ��SPI�жϵͣ�Ҫ�ȷ��ͺͽ�����������ߣ���������ɾ���Լ�
			osThreadCreate(osThread(InitThread), NULL);
		
		//	osThreadDef(MotionThread, MotionThread, osPriorityAboveNormal, 0,configMINIMAL_STACK_SIZE);//�˶���������
		//	osThreadCreate(osThread(MotionThread), NULL);
	
			osKernelStart();
}

static void adv_name_generate(uint8_t* uni_name) {
    char temp[3] = "_";
    /*adv name aplice*/
    sprintf(temp+1,"%01d%01d",*uni_name,*(uni_name+1));
    strcat(name, temp);
}
/* Device On Message */
/*
У��sum
length:��ҪУ���ֽڵĳ���
* value����ҪУ��������ָ��
return:0�ɹ���1����
*/
uint8_t Checksum(uint8_t length, uint8_t* value){
	int  check=0;
	for(int i=0;i<length;i++)	check+=*(value+i);
			if(*(value+length)==(uint8_t)check)return 0;
			else return 1;	
}
void ble_device_on_message(uint8_t type, uint16_t length, uint8_t* value)
{
	BLEMessage RXMessage;
	
	int16_t p16Data[8] = {0};//���ͻ���յ�temp
	int8_t send_temp[18]={0};//��������
	int8_t p8Data[13]={0};//��������13���ֽڵ�ң������
	if(type!=1)return;
	
		switch (*value){
		case REMOTE_CONTROL_ID://ң��ID
			if(Checksum(REMOTE_CONTROL_LONG,value))return;
			p8Data[0]=*(value+1);//X��
			p8Data[1]=*(value+2);//Y��
			p8Data[2]=*(value+3);//ģʽ������1,�������ͨ
			p8Data[3]=*(value+4);//ģʽ������2����̬��ҡ��
		  p16Data[0]=(((int16_t)*(value +5)) << 8) + (int16_t)*(value+6);//�ֻ�����
			p8Data[7]=*(value+7);//���1
		  p8Data[8]=*(value+8);//���2
		  p8Data[9]=*(value+9);//���3
		  p8Data[10]=*(value+10);//���4
		  p8Data[11]=*(value+11);//���5
		
			if(p8Data[2]==0&&p8Data[3]==0){
				speed_target=p8Data[0];
				turn_target_speed=p8Data[1];
				trun_mode=0;
			}
			else if(p8Data[2]==1&&p8Data[3]==0){		
				speed_target=-sqrt(p8Data[0]*p8Data[0]+p8Data[1]*p8Data[1]);
				if(p8Data[0]&&p8Data[1])
				turn_target_orientaion=-atan2((double)p8Data[0],(double)p8Data[1])*57.295646f;
				//else
				//	turn_target_orientaion_offset=
				trun_mode=1;
			}
			else if(p8Data[2]==0&&p8Data[3]==1){
				speed_target=p8Data[0];
				turn_target_speed=p16Data[0];
				trun_mode=0;
			}
			else if(p8Data[2]==1&&p8Data[3]==1){
				speed_target=-sqrt(p8Data[0]*p8Data[0]+p8Data[1]*p8Data[1]);
				turn_target_orientaion=-p16Data[0];
				trun_mode=1;
			}
			steer_out[0][0]=(p8Data[7]);
			steer_out[0][1]=-(p8Data[8]);
			steer_out[0][2]=(p8Data[9]);
			Stand_Up( sensor_euler_angle.pitch, Car_Angle_Center, Flag_Fall);
			
			break;
		case PARAMETER_MODIFY_ID://�޸Ĳ���ID
			if(Checksum(PARAMETER_MODIFY_LONG,value))return;
			for (int i = 0; i < 8; i++) {
				p16Data[i] = (((int16_t)*(value +i* 2 + 1)) << 8) + (int16_t)*(value+i*2+2);
			}
			Angle_Kp=					(float)p16Data[0];
			Angle_Kd=					(float)p16Data[1];
			Speed_Kp=					(float)p16Data[2];
			Speed_Ki=					(float)p16Data[3];
			Turn_Kp= 					(float)p16Data[4];
			Car_Angle_Center=	(float)p16Data[5]-20;
			//ble_device_send((uint8_t)0x01,1,(uint8_t*)"Y");
			RXMessage.type=0x01;
			RXMessage.length=2;
			memcpy(RXMessage.value, (uint8_t*)"YY", RXMessage.length);
			if( TXQueue != 0 )
			{
			xQueueSend( TXQueue, ( void* )&RXMessage, 0 );  
			}
			
			break;
			
		case PARAMETER_GET_ORIGIN_ID://����ԭʼ���ݣ�����ÿһ��������޸Ĳ�����һһ��Ӧ,���в�������ȫ��Ϊ������
			p16Data[0]=Angle_Kp;
			p16Data[1]=Angle_Kd;
			p16Data[2]=Speed_Kp;
			p16Data[3]=Speed_Ki;
			p16Data[4]=Turn_Kp;
			p16Data[5]=Car_Angle_Center+20;
			p16Data[6]=sensor_euler_angle.pitch+20;//��������������޸ģ����״����п���App�鿴ƽ��㴦��ֵȻ���޸�Car_Angle_Center
			p16Data[7]=0;
			for(int i=0;i<16;i++){
       send_temp[i+1]=(int8_t)(p16Data[i/2]>>(8-(i%2)*8));
				send_temp[1+16]+=send_temp[i+1];
       }
			send_temp[1+16]+=send_temp[0];
			send_temp[0]=PARAMETER_PUT_ORIGIN_ID;//APP����ԭʼ���ݵ�ID
			//ble_device_send((uint8_t)0x01,18,(uint8_t*)send_temp);

			 RXMessage.type=(uint8_t)0x01;
			RXMessage.length=(uint8_t)PARAMETER_PUT_ORIGIN_LONG+1;
			memcpy(RXMessage.value, (uint8_t*)send_temp, RXMessage.length);
			if( TXQueue != 0 )
			{
			xQueueSend( TXQueue, ( void* )&RXMessage, 0 );  
			}
			break;
		case MUSIC_PLAYER_MODE_ID:
		if(Checksum(MUSIC_PLAYER_MODE_LONG,value))return;
		Music_Mode=  *(value+1);
		Music_Mode_List_Ptr= *(value+2);
		break;
		case MUSIC_PLAYER_VOLUME_ID:
				if(Checksum(MUSIC_PLAYER_VOLUME_LONG,value))return;
		Volume=*(value+1);
		break;
		}
}
/* Device on connect */
void ble_device_on_connect(void)
{
}
/* Device on disconnect */
void ble_device_on_disconnect(uint8_t reason)
{
    /* Make the device connectable again. */
    Ble_conn_state = BLE_CONNECTABLE;
    ble_device_start_advertising();
}
