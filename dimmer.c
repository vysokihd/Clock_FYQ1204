#include <stdint.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "dimmer.h"
#include "main.h"

static bool modeAuto = false;
const uint8_t dimmVal[]= {DIMM_LEVELS};

void Dimm()
{
	if(timer[DIMMER] != 0) return;
	if(PWM_PORT < (ADC_PORT))
	{
		PWM_PORT++;
	}
	else if((PWM_PORT > ADC_PORT) && (PWM_PORT > DIMM_MIN_VAL))
	{
		PWM_PORT--;
	}
	
	if(PWM_PORT < 20)
	{
		TaskStart(DIMMER, 100);
	}
	else if(PWM_PORT < 40)
	{
		TaskStart(DIMMER, 50);
	}
	else TaskStart(DIMMER, 20);
}

//Установка состояния диммера
void DimmSet(uint8_t set)
{
	//Автоматический режим уже выбран
	if(set == 0 && modeAuto) return;
	
	//Выбран автоматический режим работы
	if(set == 0)
	{
		TaskStart(DIMMER, 0);
		modeAuto = true;
		//setPwm = PWM_PORT;
	}
	//Выбран ручной режим
	else
	{
		PWM_PORT = set;
		TaskStop(DIMMER);
		modeAuto = false;
	}	
	return;
}