/*
 * DS18B20.c
 *
 * Created: 10.10.2019 13:41:54
 *  Author: d.vysokih
 */ 
//#include <avr/io.h>


//#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
//#include <stdio.h>
#include "DS18B20.h"
#include "main.h"
#include <util/delay.h>

/* ������� ��� "�������� ����" � ��������� ������ �����/������ */
#define THERM_INPUT_MODE() THERM_DDR&=~(1<<THERM_DQ)
#define THERM_OUTPUT_MODE() THERM_DDR|=(1<<THERM_DQ)
#define THERM_LOW() THERM_PORT&=~(1<<THERM_DQ)
#define THERM_HIGH() THERM_PORT|=(1<<THERM_DQ)

// ����� �������
static uint8_t therm_reset(){
	uint8_t i;
	// �������� ���� ���� �� 480uS
	THERM_LOW();
	THERM_OUTPUT_MODE();
	_delay_us(500);
	// �������� ����� �� 60uS
	THERM_INPUT_MODE();
	_delay_us(60);
	// �������� �������� �� ����� � ������ 480uS
	i=(THERM_PIN & (1<<THERM_DQ));
	_delay_us(420);
	// ���������� �������� (0=OK, 1=������ �� ������)
	return i;
}


// ������� �������� ����
static void therm_write_bit(uint8_t bit){
	// �������� �� 1uS
	THERM_LOW();
	THERM_OUTPUT_MODE();
	_delay_us(2);
	// ���� ����� ��������� 1, ��������� ����� (���� ���, ��������� ��� ����)
	if(bit) THERM_INPUT_MODE();
	// ���� 60uS � ��������� �����
	_delay_us(60);
	THERM_INPUT_MODE();
}

// ������ ����
static uint8_t therm_read_bit(void){
	uint8_t bit=0;
	// �������� �� 1uS
	THERM_LOW();
	THERM_OUTPUT_MODE();
	_delay_us(2);
	// ��������� �� 14uS
	THERM_INPUT_MODE();
	_delay_us(15);
	// ������ ���������
	if(THERM_PIN&(1<<THERM_DQ)) bit=1;
	// ���� 45 ��� � ���������� ��������
	_delay_us(60);
	return bit;
}

// ������ ����
static uint8_t therm_read_byte(void){
	uint8_t i=8, n=0;
	cli();
	while(i--){
		// �������� � ����� �� 1 � ��������� ��������� ��������
		n>>=1;
		n|=(therm_read_bit()<<7);
	}
	sei();
	return n;
}

// ���������� ����
static void therm_write_byte(uint8_t byte){
	uint8_t i=8;
	cli();
	while(i--){
		// ���������� ��� � �������� ������ �� 1
		therm_write_bit(byte&1);
		byte>>=1;
	}
	sei();
}


// ������� ���������� ��������
#define THERM_CMD_CONVERTTEMP 0x44		
#define THERM_CMD_RSCRATCHPAD 0xbe
#define THERM_CMD_WSCRATCHPAD 0x4e
#define THERM_CMD_CPYSCRATCHPAD 0x48
#define THERM_CMD_RECEEPROM 0xb8
#define THERM_CMD_RPWRSUPPLY 0xb4
#define THERM_CMD_SEARCHROM 0xf0
#define THERM_CMD_READROM 0x33
#define THERM_CMD_MATCHROM 0x55
#define THERM_CMD_SKIPROM 0xcc
#define THERM_CMD_ALARMSEARCH 0xec

#define THERM_DECIMAL_STEPS_12BIT 625 //.0625

uint8_t state = 0;
int16_t tempHex;
ds18b20 status;

// ������ ����������� � �������
void Ds18b20_ConvertTemp()
{
	if(timer[TEMP_CONVERT] != 0) return;
	status = DS18B20_BUSY;
		
	switch(state)
	{
	case 0:
		//����� � �������� ������� ������� �� ����
		if(therm_reset())
		{
			status = DS18B20_NA;
			break;
		}
		
		//therm_reset();
		therm_write_byte(THERM_CMD_SKIPROM);
		therm_write_byte(THERM_CMD_CONVERTTEMP);
		state++;
		TaskStart(TEMP_CONVERT, 1000);
		break;
	
	case 1:
		therm_reset();
		therm_write_byte(THERM_CMD_SKIPROM);
		therm_write_byte(THERM_CMD_RSCRATCHPAD);
		uint16_t a = therm_read_byte();
		uint16_t b = therm_read_byte();
				
		tempHex = (b << 8) | a;
		//*(&tempHex + 0) = 0x12;//therm_read_byte();
		//(&tempHex)[1] = 0xab;//therm_read_byte();
		//tempHex = 0xabcd;
		therm_reset();
		
		//digit=temperature[0]>>4;
		//digit|=(temperature[1]&0x7)<<4;
		//
		//decimal=temperature[0]&0xf;
		//decimal*=THERM_DECIMAL_STEPS_12BIT;
		state = 0;
		status = DS18B20_DATAREADY;
		TaskStop(TEMP_CONVERT);
		break;
	}
}

int16_t Ds1820_ReadTempBCD(int8_t compens)
{
	int16_t temp;
	int16_t tempBcd = 0;
	//if(status != DS18B20_DATAREADY)
	//{
		//return 0xffff;
	//}
	if(tempHex >= 0)
	{	
		temp = (tempHex >> 4) + compens;
	}
	else temp = -1 * (tempHex >> 4) + compens;
	//������� ����� �� HEX � BCD
	tempBcd = temp % 10;
	temp /= 10;
	tempBcd |= (temp % 10) << 4;
	//���� ����� ���� �������������, ����� 1 � ������� ������
	if(tempHex < 0)
	{
		tempBcd |= (1 << 15);
	}		
	
	return tempBcd;
	//return tempHex;
}

ds18b20 Ds18b20_GetStatus()
{
	return status;
}

