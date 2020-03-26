/*
 * Buttons.c
 *
 * Created: 02.10.2019 16:47:54
 *  Author: d.vysokih
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "main.h"
#include "Buttons.h"


//��������� �������� ��������� ������ ������
static struct butState
{
	uint8_t* port;			//���� ������
	uint16_t counter;		//������� ��� ���������� ��������
	uint8_t pin;			//��� ������
	uint8_t pres:1;			//1 - ������ ������ ������, 0 - ������ ������ ��������
	uint8_t bloked:1;		//1 - ����������� ������, 0 - �� �����������
	uint8_t shortPres:1;	//1 - ������������� �������� �������
	uint8_t longPres:1;		//1 - ������������� ������� �������
	uint8_t prevState:1;	//���������� ��������� ������
}but[BUT_NUMBER] ;


void Button_Init()
{
	
	but[0].port = BUT_PORT;
	but[1].port = BUT_PORT;
	but[2].port = BUT_PORT;
	but[0].pin = BUT_PIN_MODE;
	but[1].pin = BUT_PIN_DEC;
	but[2].pin = BUT_PIN_INC;
}

bool Button_LongPress(uint8_t pin, uint8_t* port)
{
	bool state = false;
	for(uint8_t i = 0; i < BUT_NUMBER; i++)
	{
		if((pin == but[i].pin) && (port == but[i].port))
		{
			state = but[i].longPres;
			but[i].longPres = 0;
		}
	}
	return state;
}

bool Button_ShortPress(uint8_t pin, uint8_t* port)
{
	bool state = false;
	for(uint8_t i = 0; i < BUT_NUMBER; i++)
	{
		if((pin == but[i].pin) && (port == but[i].port))
		{
			state = but[i].shortPres;
			but[i].shortPres = 0;
		}
	}
	return state;
}

bool Button_CurrentPress(uint8_t pin, uint8_t* port)
{
	bool state = false;
	for(uint8_t i = 0; i < BUT_NUMBER; i++)
	{
		if((pin == but[i].pin) && (port == but[i].port))
		{
			state = but[i].pres;
		}
	}
	return state;
}

void Button_GetState()
{
	if(timer[BUT_GETSTATE] != 0) return;
	//������ �� ���� ������� � ���������� �� ���������
	for(uint8_t i = 0, btst; i < BUT_NUMBER; i++)
	{
		//�������� ������� ��������� ������ �� �����
		btst = (((1 << but[i].pin) & (*(but[i].port))) == 0)? 1:0;
		//���� ��������� �� �������� � ������ �� ������
		if((btst == 0) && (but[i].prevState == 0)) continue;
		
		//���� ��������� ���������� �� �������
		if((btst == 1) && (but[i].prevState == 0))
		{
			but[i].prevState = btst;	//���������� ��������� ������
			but[i].counter = 0;			//�������� �������
			but[i].shortPres = 0;		//�������� ��������� ������ shortPres
			but[i].longPres = 0;		//�������� �������� ������	longPres
			but[i].bloked = 0;			//��������� ������ ������
			continue;
		}

		//���� ��������� �� �������� � ������ ������
		if((btst == 1) && (but[i].prevState == 1))
		{
			but[i].counter++;
		}
		
		//���� ������ ������ � ����� ����� BUT_SHORT_PRES
		if((btst == 1) && (but[i].counter > BUT_SHORT_PRES))
		{
			but[i].pres = 1;
		}
		
		//���� ������ ������, ����� ����� BUT_LONG_PRES � ��������� longPres �� �����������
		if((btst == 1) && (but[i].counter > BUT_LONG_PRES) && (but[i].bloked == 0))
		{
			but[i].longPres = 1;
			but[i].shortPres = 0;
			but[i].bloked = 1;
		}
		
		//���� ��������� ���������� �� ���������
		if((btst == 0) && (but[i].prevState == 1))
		{
			but[i].prevState = btst;	//���������� ��������� ������
			but[i].pres = 0;
			
			if((but[i].counter > BUT_SHORT_PRES) && (but[i].counter < (BUT_LONG_PRES / 2)))
			{
				but[i].shortPres = 1;
				but[i].longPres = 0;
			}
		}
	}
	TaskStart(BUT_GETSTATE, BUT_GETSTATE_TIME);
}