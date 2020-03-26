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


//Структура хранящая состояние каждой кнопки
static struct butState
{
	uint8_t* port;			//порт кнопки
	uint16_t counter;		//счётчик для реализации задержки
	uint8_t pin;			//пин кнопки
	uint8_t pres:1;			//1 - кнопка сейчас нажата, 0 - кнопка сейчас отпущена
	uint8_t bloked:1;		//1 - блокирована кнопка, 0 - не блокирована
	uint8_t shortPres:1;	//1 - зафиксировано короткое нажатие
	uint8_t longPres:1;		//1 - зафиксировано длинное нажатие
	uint8_t prevState:1;	//предыдущее состояние кнопки
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
	//Проход по всем кнопкам и считывание их состояние
	for(uint8_t i = 0, btst; i < BUT_NUMBER; i++)
	{
		//получаем текущее состояние кнопки из порта
		btst = (((1 << but[i].pin) & (*(but[i].port))) == 0)? 1:0;
		//если состояние не менялось и кнопка не нажата
		if((btst == 0) && (but[i].prevState == 0)) continue;
		
		//если состояние поменялось на нажатое
		if((btst == 1) && (but[i].prevState == 0))
		{
			but[i].prevState = btst;	//запоминаем состояние кнопки
			but[i].counter = 0;			//обнуляем счетчик
			but[i].shortPres = 0;		//обнуляем состояние кнопки shortPres
			but[i].longPres = 0;		//обнуляем сотояние кнопки	longPres
			but[i].bloked = 0;			//разрешаем работу кнопки
			continue;
		}

		//если состояние не менялось и кнопка нажата
		if((btst == 1) && (but[i].prevState == 1))
		{
			but[i].counter++;
		}
		
		//если кнопка нажата и вышло время BUT_SHORT_PRES
		if((btst == 1) && (but[i].counter > BUT_SHORT_PRES))
		{
			but[i].pres = 1;
		}
		
		//если кнопка нажата, вышло время BUT_LONG_PRES и состояние longPres не срабатывало
		if((btst == 1) && (but[i].counter > BUT_LONG_PRES) && (but[i].bloked == 0))
		{
			but[i].longPres = 1;
			but[i].shortPres = 0;
			but[i].bloked = 1;
		}
		
		//если состояние поменялось на отпущеное
		if((btst == 0) && (but[i].prevState == 1))
		{
			but[i].prevState = btst;	//запоминаем состояние кнопки
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