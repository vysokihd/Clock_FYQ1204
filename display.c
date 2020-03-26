/*
* display.c
*
* Created: 14.09.2019 21:10:34
*  Author: Dmitry
*/
#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include "display.h"
#include "main.h"

//Выбор типа дисплея
#if defined(COMMON_ANODE)
	#define SETPORT(por, mask)	por &= ~(mask)	
	#define CLRPORT(por, mask)	por |= (mask)
#elif defined(COMMON_CATODE)
	#define SETPORT(por, mask)	por |= (mask)
	#define CLRPORT(por, mask)	por &= ~(mask)
#else 
	#error "Display Type Not Defined"
#endif	


static uint8_t dispBufer [DISPLAY_DIGITS];
static uint8_t animBufer [DISPLAY_DIGITS];
static uint8_t activeDig;
static uint8_t animState;			//стадия анимации цифр
static bool	animBusy = false;		//true - начата анимация, fasle - анимация не активна
static uint8_t blinkBitmask;		//маска цифр, которые должны мигать
static uint8_t blinkTime;			//время мигания (1 = 100мс)
static bool blinkDigits = false;	//true - зажигать, false - не зажигать цифру
static bool dispOn = true;			//true - разрешить, false - запретить индикацию


static void SetClearSegment(uint8_t set, uint8_t clear);

bool dotsStatus = false;

void DotsOn()
{
	TaskStop(DOTS_BLINK);
	dotsStatus = true;
	SETPORT(DOTS_PORT, (1 << DOTS_PIN));
}

void DotsOff()
{
	TaskStop(DOTS_BLINK);
	dotsStatus = false;
	CLRPORT(DOTS_PORT, (1 << DOTS_PIN));
}

void DotsBlinkOn()
{
	TaskStart(DOTS_BLINK,DOTS_BLINK_TIME);
}

void DotsBlink()
{
	//проверка времени
	if(timer[DOTS_BLINK] != 0) return;
	
	if(dotsStatus == true)
	{
		dotsStatus = false;
		CLRPORT(DOTS_PORT, (1 << DOTS_PIN));
	}
	else
	{
		dotsStatus = true;
		SETPORT(DOTS_PORT, (1 << DOTS_PIN));
	}
	
	TaskStart(DOTS_BLINK, DOTS_BLINK_TIME);	
}


void DisplayUpdate()
{
	//выключае все общие выводы индикаторов
	CLRPORT(COMMON_PORT, COMMON_MASK);
	
	//if(dimm == true)
	//{
		//dispOn? (dispOn = false) : (dispOn = true);
		//if(dotsStatus == true)
		//{
			//SETPORT(DOTS_PORT, (DOTS_PIN));
		//}
	//}
	//else dispOn = true;
	//
	//if(!dispOn) return;
	
	//Через заданное время инвертируем переменую отвечающую за мигание цифр
	if(timer[DISP_BLINK] == 0)
	{
		blinkDigits = !blinkDigits;
		TaskStart(DISP_BLINK, (blinkTime * 100));
	}
	//Проверяем по маске цифру которую хотим мигать
	if(((blinkBitmask & (1 << activeDig)) != 0) && blinkDigits)
	{
		activeDig++;
		if(activeDig == DISPLAY_DIGITS) activeDig = 0;
		TaskStart(DISP_UPDATE,UPDATE_TIME);
		return;
	}
	//Записываем цифру в порт сегментов
	CLRPORT(SEG_PORT, SEG_MASK);
	SETPORT(SEG_PORT, (dispBufer[activeDig] & SEG_MASK));
	
	//Поджигаем цифру запиткой общего вывода
	switch(activeDig)
	{
		case 0:
		SETPORT(COMMON_PORT, (1 <<COMMON_0));
		activeDig++;
		break;

		case 1:
		SETPORT(COMMON_PORT, (1 <<COMMON_1));
		activeDig++;
		break;

		case 2:
		SETPORT(COMMON_PORT, (1 <<COMMON_2));
		activeDig++;
		break;

		case 3:
		SETPORT(COMMON_PORT, (1 <<COMMON_3));
		activeDig = 0;
		break;
	}
	//Заводим таймер для последующей отрисовки цифры
	//TaskStart(DISP_UPDATE,UPDATE_TIME);
}

void DisplaySet_Int(uint16_t set, uint8_t bitmask, bool animate)
{
	//Не обновлять буфер дисплея, если цыфры не поменялись или идёт анимация
	if(animBusy == true) return;
	for(uint8_t i = 0, num; i < DISPLAY_DIGITS; i++)
	{
		if((bitmask & (1 << i)) == 0)
		{
			animate? (animBufer[i] = 0) : (dispBufer[i] = 0);
		}
		else
		{
			num = (uint8_t)(set & 0xF);
			animate? (animBufer[i] = digits[num]) : (dispBufer[i] = digits[num]);
			set = (set >> 4);
		}
	}	
	if(animate)
	{
		TaskStart(DISP_ANIMATE,0);
	}
}


void DisplaySet_Char(disp_sym* sym, bool animate)
{
	//Не обновлять буфер дисплея пока идёт анимация
	if(animBusy == true) return;
	for(uint8_t i = 0; i <DISPLAY_DIGITS; i++)
	{
		if(animate)
		{
			animBufer[i] = sym[DISPLAY_DIGITS - 1 - i];
		}
		else
		{
			dispBufer[i] = sym[DISPLAY_DIGITS - 1 - i];
		}
	}
	if(animate)
	{
		TaskStart(DISP_ANIMATE,0);
	}
}


void DisplayOff()
{
	CLRPORT(COMMON_PORT, COMMON_MASK);
	dispOn = false;
	//TaskStop(DISP_UPDATE);
}

void DisplayOn()
{
	dispOn = true;
	//TaskStart(DISP_UPDATE, 0);
}

void DisplayAnimate()
{
	if(timer[DISP_ANIMATE] != 0) return;
	
	if(animState == 0)
	{
		animBusy = true;
		for (uint8_t i = 0; i < DISPLAY_DIGITS; i++)
		{
			if(dispBufer[i] != animBufer[i])
			{
				dispBufer[i] = 0;
			}
		}
	}
	switch(animState)
	{
		case 0:
		SetClearSegment((1 << SEG_F), 0);
		TaskStart(DISP_ANIMATE,ANIMATE_TIME);
		animState++;
		break;
		case 1:
		SetClearSegment((1 << SEG_A), (1 << SEG_F));
		TaskStart(DISP_ANIMATE,ANIMATE_TIME);
		animState++;
		break;
		case 2:
		SetClearSegment((1 << SEG_B), (1 << SEG_A));
		TaskStart(DISP_ANIMATE,ANIMATE_TIME);
		animState++;
		break;
		case 3:
		SetClearSegment((1 << SEG_C), (1 << SEG_B));
		TaskStart(DISP_ANIMATE,ANIMATE_TIME);
		animState++;
		break;
		case 4:
		SetClearSegment((1 << SEG_D), (1 << SEG_C));
		TaskStart(DISP_ANIMATE,ANIMATE_TIME);
		animState++;
		break;
		case 5:
		SetClearSegment((1 << SEG_E), (1 << SEG_D));
		TaskStart(DISP_ANIMATE,ANIMATE_TIME);
		animState++;
		break;
		case 6:
		SetClearSegment((1 << SEG_G), (1 << SEG_E));
		TaskStart(DISP_ANIMATE,ANIMATE_TIME);
		animState++;
		break;
		case 7:
		SetClearSegment(0, (1 << SEG_G));
		TaskStop(DISP_ANIMATE);
		animState = 0;
		animBusy = false;
	}
}

void DisplayBlinkOn(uint8_t bitmask, uint8_t period)
{
	if((bitmask == blinkBitmask) && (period == blinkTime)) return;
	blinkBitmask = bitmask;
	blinkTime = period;
	blinkDigits = true;
	TaskStart(DISP_BLINK, (blinkTime * 100));
}

void DisplayBlinkOff()
{
	blinkDigits = false;
	blinkBitmask = 0;
	TaskStop(DISP_BLINK);
}

void DisplayIntToChar(uint16_t set, disp_sym* out)
{
	uint8_t size = (sizeof(uint16_t) * 2);
	uint8_t num;
	
	for(int8_t i = (size - 1); i >= 0; i--)
	{
		num = (uint8_t)(set & 0xF);
		out[i] = digits[num];
		set = (set >> 4);
	}
}

void DisplayDayToChar(uint8_t in, disp_sym* out, uint8_t offset)
{
	switch(in)
	{
		case 1:	
			out[0 + offset] = CHAR_RU_P;
			out[1 + offset] = CHAR_H;
		break;	  
		case 2:	  
			out[0 + offset] = CHAR_8;
			out[1 + offset] = CHAR_H;
		break;	  
		case 3:	  
			out[0 + offset] = CHAR_C;
			out[1 + offset] = CHAR_P;
		break;	  
		case 4:	  
			out[0 + offset] = CHAR_4;
			out[1 + offset] = CHAR_P;
		break;	  
		case 5:	  
			out[0 + offset] = CHAR_RU_P;
			out[1 + offset] = CHAR_t;
		break;	  
		case 6:	  
			out[0 + offset] = CHAR_C;
			out[1 + offset] = CHAR_RU_B;
		break;	  
		case 7:	  
			out[0 + offset] = CHAR_8;
			out[1 + offset] = CHAR_C;
		break;
		default:
			out[0 + offset] = CHAR_PROCHERK;
			out[1 + offset] = CHAR_PROCHERK;		
	}
}

static void SetClearSegment(uint8_t set, uint8_t clear)
{
	for(volatile int i = 0; i < 4; i++)
	{
		if(dispBufer[i] != animBufer[i])
		{
			if(set != 0)
			{
				dispBufer[i] |= set;
			}
			
			if(clear == 0) continue;
			if((animBufer[i] & clear) == 0)
			{
				dispBufer[i] &= ~(clear);
			}
		}
	}
	
}
