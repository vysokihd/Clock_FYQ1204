#ifndef DIMMER_H_
#define DIMMER_H_

#include <avr/io.h>

#define PWM_PORT		OCR1A
#define ADC_PORT		ADCH

#define L_00	0
#define L_01	15
#define L_02	50
#define L_03	100
#define L_04	175
#define L_05	255
#define L_CNT	5
#define DIMM_MIN_VAL	L_01

extern const uint8_t dimmVal[];

/*	�������������� ������������ � ����������� �� ������� �����
	������� ������ ���� �������� � while()*/
void Dimm();

/*	��������� ������� �������
	set = 0 - �������������� �����, � ����������� �� ������� �����
	set = 1..255 - ������ �����
*/
void DimmSet(uint8_t set);


#endif