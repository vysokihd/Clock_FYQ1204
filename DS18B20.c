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

/* Макросы для "дергания ноги" и изменения режима ввода/вывода */
#define THERM_INPUT_MODE() THERM_DDR&=~(1<<THERM_DQ)
#define THERM_OUTPUT_MODE() THERM_DDR|=(1<<THERM_DQ)
#define THERM_LOW() THERM_PORT&=~(1<<THERM_DQ)
#define THERM_HIGH() THERM_PORT|=(1<<THERM_DQ)

// сброс датчика
static uint8_t therm_reset(){
	uint8_t i;
	// опускаем ногу вниз на 480uS
	THERM_LOW();
	THERM_OUTPUT_MODE();
	_delay_us(500);
	// подымаем линию на 60uS
	THERM_INPUT_MODE();
	_delay_us(60);
	// получаем значение на линии в период 480uS
	i=(THERM_PIN & (1<<THERM_DQ));
	_delay_us(420);
	// возвращаем значение (0=OK, 1=датчик не найден)
	return i;
}


// функция отправки бита
static void therm_write_bit(uint8_t bit){
	// опускаем на 1uS
	THERM_LOW();
	THERM_OUTPUT_MODE();
	_delay_us(2);
	// если хотим отправить 1, поднимаем линию (если нет, оставляем как есть)
	if(bit) THERM_INPUT_MODE();
	// ждем 60uS и поднимаем линию
	_delay_us(60);
	THERM_INPUT_MODE();
}

// чтение бита
static uint8_t therm_read_bit(void){
	uint8_t bit=0;
	// опускаем на 1uS
	THERM_LOW();
	THERM_OUTPUT_MODE();
	_delay_us(2);
	// поднимаем на 14uS
	THERM_INPUT_MODE();
	_delay_us(15);
	// читаем состояние
	if(THERM_PIN&(1<<THERM_DQ)) bit=1;
	// ждем 45 мкс и возвращаем значение
	_delay_us(60);
	return bit;
}

// читаем байт
static uint8_t therm_read_byte(void){
	uint8_t i=8, n=0;
	cli();
	while(i--){
		// сдвигаем в право на 1 и сохраняем следующее значение
		n>>=1;
		n|=(therm_read_bit()<<7);
	}
	sei();
	return n;
}

// отправляем байт
static void therm_write_byte(uint8_t byte){
	uint8_t i=8;
	cli();
	while(i--){
		// отправляем бит и сдвигаем вправо на 1
		therm_write_bit(byte&1);
		byte>>=1;
	}
	sei();
}


// команды управления датчиком
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
int16_t tempRaw;
int16_t tempHex;
int16_t tempBcd;
ds18b20 status;

// читаем температуру с датчика
void Ds18b20_ConvertTemp()
{
	if(timer[TEMP_CONVERT] != 0) return;
	status = DS18B20_BUSY;
		
	switch(state)
	{
	case 0:
		//Сброс и проверка наличия датчика на шине
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
		uint8_t a = therm_read_byte();
		uint8_t b = therm_read_byte();
				
		//Температура в сыром виде с датчика
		tempRaw = (b << 8) | a;
		//Температура в нормальном виде в hex-формате (пример: 25.15гр, читается как 2515)
		tempHex = (int32_t)tempRaw * THERM_DECIMAL_STEPS_12BIT / 100;
		//Перевод числа из HEX в BCD
		int16_t temp = tempHex;
		tempBcd = temp % 10;
		temp /= 10;
		tempBcd |= (temp % 10) << 4;
		temp /= 10;
		tempBcd |= (temp % 10) << 8;
		temp /= 10;
		tempBcd |= temp << 12;

		therm_reset();
		state = 0;
		status = DS18B20_DATAREADY;
		TaskStop(TEMP_CONVERT);
		break;
	}
}

int16_t Ds1820_ReadTempBCD(int8_t compens)
{
	int16_t temp;
	int16_t Bcd = 0;
	//if(status != DS18B20_DATAREADY)
	//{
		//return 0xffff;
	//}
		if(tempRaw >= 0)
		{	
			temp = (tempRaw >> 4) + compens;
		}
		else temp = -1 * (tempRaw >> 4) + compens;
	//Перевод числа из HEX в BCD
	Bcd = temp % 10;
	temp /= 10;
	Bcd |= temp << 4;
		
	//Если число было отрицательным, пишем 1 в старший разряд
	if(tempRaw < 0)
	{
		Bcd |= (1 << 15);
	}		
	
	return Bcd;
	//return tempHex;
}

ds18b20 Ds18b20_GetStatus()
{
	return status;
}

