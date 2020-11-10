#ifndef DIMMER_H_
#define DIMMER_H_

#include <avr/io.h>

#define PWM_PORT		OCR1A
#define ADC_PORT		ADCH

extern const uint8_t dimmVal[];

#define DIMM_LEVELS		0, 15, 50, 100, 175, 255
#define DIMM_COUNT		5
#define DIMM_MIN_VAL	10



/*	Автоматическое диммирование в зависимости от датчика света
	Функция должна быть помещена в while()*/
void Dimm();

/*	Установка яркости дисплея
	set = 0 - автоматический режим, в зависимости от датчика света
	set = 1..255 - ручной режим
*/
void DimmSet(uint8_t set);


#endif