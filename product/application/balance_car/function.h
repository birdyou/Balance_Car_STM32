#ifndef _function_H
#define _function_H
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"

#include "stm32f4xx_hal_sd.h"
#include "stm32469i_discovery_sd.h"

/* FatFs includes component */
#include "ff_gen_drv.h"
#include "sd_diskio.h"



void Encoder_Init(void);
void Get_Speed(int32_t *SpeedL,int32_t *SpeedR,float *SpeedA);
void Steer_Pwm_Init(void);
void Motor_Pwm_Init(void);
void	SD_Init(void);
void	Adc_Init(void);
	void Motor_Control_1(int16_t Pulse);
		void Motor_Control_2(int16_t Pulse);
	void Get_Adc(uint32_t *Adc);
	uint8_t Fall_Detect(float Angle,float Target);
HAL_StatusTypeDef HAL_TIM_PWM_Pulse(TIM_HandleTypeDef *htim,  uint32_t Channel,uint32_t Pulse);
void Steer_Control(int16_t steer_out[2][5]);
int Pick_Up_Detect(float Angle,float Car_Angle_Center,int encoder_left,int encoder_right);
void Stand_Up(float Angle,float Car_Angle_Center,int8_t Flag_Fall);
int myabs(int a);
#endif
