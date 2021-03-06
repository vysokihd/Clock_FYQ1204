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

typedef struct
{
	uint8_t* port;			//порт кнопки
	uint8_t pin;			//пин кнопки
	uint16_t timer;			//таймер для реализации задержки
	uint8_t pres:1;			//1 - кнопка сейчас нажата, 0 - кнопка сейчас отпущена
	uint8_t bloked:1;		//1 - блокирована кнопка, 0 - не блокирована
	uint8_t shortPres:1;	//1 - зафиксировано короткое нажатие
	uint8_t longPres:1;		//1 - зафиксировано длинное нажатие
	uint8_t prevState:1;	//предыдущее состояние кнопки
}butState;

butState but[] = {BUT_LIST_INIT};
#define BUT_NUMBER sizeof(but)/sizeof(butState)

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
	//Проход по всем кнопкам и считывание их состояние
	for(uint8_t i = 0, btst; i < BUT_NUMBER; i++)
	{
		//получаем текущее состояние кнопки из порта
		btst = (((1 << but[i].pin) & (*(but[i].port))) == 0)? 0 : 1;

		//кнопка не нажата и состояние не менялось
		if(btst == 1 && (but[i].prevState == 1))
		{
			//сброс состояния кнопки через некоторое время
			if(but[i].timer > BUT_RESET_TIME)
			{
				but[i].shortPres = 0;
				but[i].longPres = 0;
			}
			else but[i].timer++;
			continue;
		}
		
		//кнопка поменяла состояние на нажатое
		if(btst == 0 && (but[i].prevState == 1))
		{
			but[i].prevState = btst;	//запоминаем состояние кнопки
			but[i].timer = 0;			//обнуляем счетчик
			but[i].shortPres = 0;		//обнуляем состояние кнопки shortPres
			but[i].longPres = 0;		//обнуляем состояние кнопки	longPres
			but[i].bloked = 0;			//разрешаем работу кнопки
		}
		//кнопка нажата и состояние не менялось
		else if((btst == 0) && (but[i].prevState == 0) && !but[i].bloked)
		{
			if(but[i].timer > BUT_LONG_PRES)
			{
				but[i].longPres = 1;
				but[i].bloked = 1;
			}
			else but[i].timer++;
		}
		//кнопка поменяла состояние на отпущеное
		else if(btst == 1 && but[i].prevState == 0)
		{
			if(but[i].timer > BUT_SHORT_PRES && but[i].timer < BUT_LONG_PRES/2)
			{
				but[i].shortPres = 1;
			}
			but[i].prevState = btst;
			but[i].timer = 0;
		}
	}
	TaskStart(BUT_GETSTATE, BUT_GETSTATE_TIME);
}