

#include "control.h"
#include "math.h"
#define SPEED_INTEGRAL_MAX  200
#define SPEED_INTEGRAL_MIN  -200

#define TURN_CONTROL_OUT_MAX  500
#define TURN_CONTROL_OUT_MIN  -500

#define SPEED_CONTROL_PERIOD 20

float speed_target;
float 	turn_target_speed;
float 	turn_target_orientaion;
int8_t	trun_mode;
int8_t Flag_PickUp=0;
int8_t Flag_Fall=0;
float Turn_Kp=3,Turn_Kp2=3;	//ת�����P
/*������ʱ
float Speed_Kp=12,Speed_Ki=2;	//�ٶȿ���PI						
float Angle_Kp=170,Angle_Kd=17;	//�Ƕȿ���PD
 float Car_Angle_Center=-5;			//ƽ���Ƕ�
 */
 float Speed_Kp=11,Speed_Ki=2;	//�ٶȿ���PI
float Angle_Kp=125,Angle_Kd=12;	//�Ƕȿ���PD
 float Car_Angle_Center=0;			//ƽ���Ƕ�
 
 extern IMU_Offset MyOffset;
imu_sensor_raw_data_t sensor_saw_data;//IMU�ʹ�����ԭʼֵ
imu_sensor_data_t sensor_data;//У׼ת�����ֵ��Offset��MyOffset����
imu_euler_data_t sensor_euler_angle;//ŷ����
uint16_t MData[3];
int16_t motor1_output;//���1�����ֵ��-1000~1000
int16_t motor2_output;//���2�����ֵ��-1000~1000



int16_t motor_output_temp;
int16_t motor_output_Angle;
int16_t motor_output_Speed;
int16_t motor_output_Turn;
int16_t steer_out[2][5]={0,0,0,0,0,
												 0,0,0,0,0};//��������ά���飬����Խ�����ȼ�Խ��
 float speed_A;//�ٶȺ�
 int32_t speed_L;//��ߵ���ٶ�
 int32_t speed_R;//�ұߵ���ٶ�
 int8_t Speed_ControlPeriod =0;
 float Speed_ControlOutOld;
float SpeedControlOutNew=0;
float SpeedControlOutValue;
float Encoder_Integral=0;												 
 int my_cnt=0;
int MAG_Cnt=0;
extern uint8_t Read_Temp_Flag;									
	 int16_t Temperature;									 
/**************************************************************************
�������ܣ�����PI������
��ڲ���������������ֵ��Ŀ���ٶ�
����  ֵ�����PWM
��������ʽ��ɢPID��ʽ 
pwm+=Kp[e��k��-e(k-1)]+Ki*e(k)+Kd[e(k)-2e(k-1)+e(k-2)]
e(k)������ƫ�� 
e(k-1)������һ�ε�ƫ��  �Դ����� 
pwm�����������
�����ǵ��ٶȿ��Ʊջ�ϵͳ���棬ֻʹ��PI����
pwm+=Kp[e��k��-e(k-1)]+Ki*e(k)
**************************************************************************/
int Speed_Incremental_PI (int Encoder,int Target)
{ 	
	 static int Bias,Pwm,Last_bias;
	 Bias=Encoder-Target;                //����ƫ��
	 Pwm+=Speed_Kp*(Bias-Last_bias)+Speed_Ki*Bias;   //����ʽPI������
	if(Pwm>500)Pwm=500;
	else if(Pwm<-500)Pwm=-500;
	 Last_bias=Bias;	                   //������һ��ƫ�� 
	 return Pwm;                         //�������
}


int Speed_PI (float Encoder,int Movement)
{
	
	
	Encoder_Integral +=Encoder;                                     
		Encoder_Integral=Encoder_Integral+Movement;    
	if(Encoder_Integral>SPEED_INTEGRAL_MAX)  	Encoder_Integral=SPEED_INTEGRAL_MAX;           
		if(Encoder_Integral<SPEED_INTEGRAL_MIN)	Encoder_Integral=SPEED_INTEGRAL_MIN;
	
	return Encoder*Speed_Kp+Encoder_Integral*Speed_Ki;  

}

int My_Speed_PI(float Encoder,int Movement)
{
	static float fP;
	fP=Encoder+Movement;
	Encoder_Integral+=fP;
	if(Encoder_Integral>SPEED_INTEGRAL_MAX)  	Encoder_Integral=SPEED_INTEGRAL_MAX;           
		if(Encoder_Integral<SPEED_INTEGRAL_MIN)	Encoder_Integral=SPEED_INTEGRAL_MIN;
		return fP*Speed_Kp+Encoder_Integral*Speed_Ki;  
}


//------------------------------------------------------------------------------


int16_t Speed_Control(float Speed_Target) 
{      int16_t motor_output_Speed_temp;
 Speed_ControlPeriod++;
	 motor_output_Speed_temp = SpeedControlOutValue * ((float)Speed_ControlPeriod)  / SPEED_CONTROL_PERIOD+ Speed_ControlOutOld;
		if(Speed_ControlPeriod>=SPEED_CONTROL_PERIOD)
		{	
			Speed_ControlPeriod=0;
			Get_Speed(&speed_L,&speed_R,&speed_A);
			Speed_ControlOutOld = SpeedControlOutNew;
			SpeedControlOutNew=My_Speed_PI(speed_A,Speed_Target);
			SpeedControlOutValue=SpeedControlOutNew-Speed_ControlOutOld;	
		}	
//	return motor_output_Speed_temp;//�����Ҫƽ���������ʹ�����
	return	SpeedControlOutNew;
}


int Angle_Control_PD(float Angle,float Target,float gyro){
	
	return Angle_Kp*(Angle-Target)+Angle_Kd*gyro/10.0f;
}
int Turn_Control(int8_t Mode,int16_t Turn_Speed,float Now_Orientation,float Target_Orientation){
	float Diff_Orientation;
	
	
	if(Mode==0){//��ͨģʽ
		motor_output_Turn=Turn_Kp2*Turn_Speed;
	}
	else if(Mode==1){//����ģʽ
		Diff_Orientation=Now_Orientation-Target_Orientation;
		if(Diff_Orientation>=-360&&Diff_Orientation<-180)
			motor_output_Turn=(Diff_Orientation+360)*Turn_Kp;
		
		else if(Diff_Orientation>=-180&&Diff_Orientation<180)
				motor_output_Turn=(Diff_Orientation)*Turn_Kp;
		
		else if(Diff_Orientation>=180&&Diff_Orientation<=360)
			motor_output_Turn=(Diff_Orientation-360)*Turn_Kp;
		
	}
	if(motor_output_Turn>TURN_CONTROL_OUT_MAX)motor_output_Turn=TURN_CONTROL_OUT_MAX;
	else if(motor_output_Turn<TURN_CONTROL_OUT_MIN)motor_output_Turn=TURN_CONTROL_OUT_MIN;
	
	return motor_output_Turn;
}

void IIC_Operation(void){
//	 int16_t temperature;
 //JSensor_HUM_TEMP_Typedef tdef;		
	
	MAG_Cnt++;
	if(MAG_Cnt>=40){
		MAG_Cnt=0;
		LSM303AGR_MAG_Get_Raw_Magnetic((u8_t*)MData);
	}
	//if(Read_Temp_Flag==1){
  //  tdef.temperature = &temperature;
	//	 if (JSENSOR_OK == jsensor_app_read_sensor(JSENSOR_TYPE_HUMITY_TEMP, (void *)&tdef)) {Ϊ�ζ�ȡ�¶��Ǵ�ģ�
	//		 Temperature=(temperature-32)/0.18f;
			 
			  //Temperature=temperature;
 //   }
	//}
}
void Car_Control(void){

	//imu_sensor_read_data_from_fifo(&sensor_saw_data,&sensor_data,&sensor_euler_angle);
	//my_imu_sensor_read_data_from_fifo(&sensor_saw_data,&sensor_data,&sensor_euler_angle);
	motor_output_Angle =  Angle_Control_PD(sensor_euler_angle.pitch,Car_Angle_Center,sensor_data.gyro[0]);
	motor_output_Turn  =  Turn_Control(trun_mode,turn_target_speed,sensor_euler_angle.yaw,turn_target_orientaion);
	motor_output_Speed =  Speed_Control(speed_target);
	
	motor1_output=motor_output_Angle-motor_output_Speed+motor_output_Turn;
	motor2_output=motor_output_Angle-motor_output_Speed-motor_output_Turn;
	
	if(Flag_Fall||Flag_PickUp){
		motor1_output=0;
		motor2_output=0;
	}
		Motor_Control_1(motor1_output);
		Motor_Control_2(-motor2_output);	
	
		Flag_Fall=Fall_Detect(sensor_euler_angle.pitch,Car_Angle_Center);
	
		//Flag_PickUp=Pick_Up_Detect( sensor_euler_angle.pitch,Car_Angle_Center, speed_L, speed_R);

	IIC_Operation();//IICΪ�����غ������жϻᵼ��������������ｫ���������ݶ�ȡ�ŵ�����
	my_cnt++;
}

