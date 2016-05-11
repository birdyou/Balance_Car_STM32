
#include "motion.h"
#include "cmsis_os.h"
#include "function.h"

extern int16_t steer_out[2][5];
Steer_Motion_TypeDef Steer_Motion;
/*
	osThreadDef(MotionARMRightThread, MotionARMRightThread, osPriorityNormal, 0,configMINIMAL_STACK_SIZE);//���ֱ۵Ŀ������������������������ȼ���ң�ظ�
		
			osThreadDef(MotionHeadThread, MotionHeadThread, osPriorityNormal, 0,configMINIMAL_STACK_SIZE);//ͷ�Ŀ������������������������ȼ���ң�ظ�
		
			osThreadDef(MotionSpeedThread, MotionSpeedThread, osPriorityNormal, 0,configMINIMAL_STACK_SIZE);//�ٶȵĿ������������������������ȼ���ң�ظ�
	
			osThreadDef(MotionTurnThread, MotionTurnThread, osPriorityNormal, 0,configMINIMAL_STACK_SIZE);//ת��Ŀ������������������������ȼ���ң�ظ�
	
*/

/**************************************************************************
�������ܣ����ֱ۵Ŀ���
**************************************************************************/
  void MotionARMLeftThread(void const *argument)
{ 
	int16_t Steer_Now,Steer_E;
	Steer_Now=steer_out[0][1];
	if(steer_out[1][1]!=0)
	Steer_Now=steer_out[1][1];	
  Steer_E=myabs(Steer_Motion.Steer_Left_Hand1-Steer_Now);
	steer_out[1][1]=(Steer_Motion.Steer_Left_Hand1);
	osDelay(Steer_E*2);
	Steer_Motion.Steer_Left_Hand1_Flag=1;
	//osDelay(2000);
	vTaskDelete(NULL);
	
}

/**************************************************************************
�������ܣ����ֱ۵Ŀ���
**************************************************************************/
  void MotionARMRightThread(void const *argument)
{  
  
}

/**************************************************************************
�������ܣ�ͷ�Ŀ���
**************************************************************************/
  void MotionHeadThread(void const *argument)
{  
  
}
/**************************************************************************
�������ܣ��ٶȵĿ���
**************************************************************************/
  void MotionSpeedThread(void const *argument)
{  
  
}
/**************************************************************************
�������ܣ�ת��Ŀ���
**************************************************************************/
  void MotionTurnThread(void const *argument)
{  
  
}

/**************************************************************************
�������ܣ�ת��Ŀ���
**************************************************************************/
  osThreadState  MotionARMLeftThreadState;
//extern osThreadDef_t os_thread_def_MotionARMLeftThread;
osThreadId MotionARMLeftThreadID=0;
  void MotionThread(void const *argument)
{
  for(;;){
		/*
		if(MotionARMLeftThreadID==0||(MotionARMLeftThreadState=osThreadGetState(MotionARMLeftThreadID)==1)){
//	MotionARMLeftThreadState=osThreadGetState(os_thread_def_MotionARMLeftThreadID);
	//	if(MotionARMLeftThreadState==1){
			Steer_Motion.Steer_Head=100;
			osThreadDef(MotionARMLeftThread, MotionARMLeftThread, osPriorityNormal, 0,configMINIMAL_STACK_SIZE);//���ֱ۵Ŀ������������������������ȼ���ң�ظ�
		
				MotionARMLeftThreadID = osThreadCreate(osThread(MotionARMLeftThread), NULL);
		}
		*/
		if(Steer_Motion.Steer_Left_Hand1_Flag==1){
			Steer_Motion.Steer_Left_Hand1_Flag=0;
			Steer_Motion.Steer_Left_Hand1=100;
			osThreadDef(MotionARMLeftThread, MotionARMLeftThread, osPriorityNormal, 0,configMINIMAL_STACK_SIZE);//���ֱ۵Ŀ������������������������ȼ���ң�ظ�
			MotionARMLeftThreadID = osThreadCreate(osThread(MotionARMLeftThread), NULL);
		}
		
		osDelay(50);
	}
}
