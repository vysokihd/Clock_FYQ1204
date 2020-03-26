#include <stdint.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "dimmer.h"
#include "main.h"

static bool modeAuto = false;
const uint8_t dimmVal[]=
{
	L_00,
	L_01,
	L_02,
	L_03,
	L_04,
	L_05,
};

void Dimm()
{
	if(timer[DIMMER] != 0) return;
	
	if (PWM_PORT < (ADC_PORT))
	{
		PWM_PORT++;
	}
	else if(PWM_PORT > (ADC_PORT))
	{
		if(PWM_PORT > DIMM_MIN_VAL)
		{
			PWM_PORT--;
		}
	}
	
	TaskStart(DIMMER, 20);
}

//��������� ��������� �������
void DimmSet(uint8_t set)
{
	//�������������� ����� ��� ������
	if(set == 0 && modeAuto) return;
	
	//������ �������������� ����� ������
	if(set == 0)
	{
		TaskStart(DIMMER, 0);
		modeAuto = true;
		//setPwm = PWM_PORT;
	}
	//������ ������ �����
	else
	{
		PWM_PORT = set;
		TaskStop(DIMMER);
		modeAuto = false;
	}	
	return;
}