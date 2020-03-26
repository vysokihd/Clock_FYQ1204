#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include "main.h"
#include "ds1307.h"
#include "i2c.h"
//#include <avr/delay.h>

#define CH_BIT		7
#define RAM_START	0x08
#define RAM_END		0x3F

 //кол-во дней в месяце не высокосного года
static const uint8_t dayInMonth[] = {0, 0x31, 0x28, 0x31, 0x30, 0x31, 0x30, 0x31, 0x31, 0x30, 0x31, 0x30, 0x31};
	 
//определение высокосного года
static inline bool leapYear()
{
	if((tm.tm_year % 4) == 0) return true;
	else return false; 
}

//инкремент двоично-десятичного числа
static uint8_t IncrBCD(uint8_t num)
{
	num++;
	if(num > 0x99) return 0;
	if((num & 0xf) > 9)
	{
		num += 6; //дополняем на 6 если переполнение было
	}
	return num;
}

//декремент двоично-десятичного числа
static uint8_t DecrBCD(uint8_t num)
{
	num--;
	if(num > 0x99) return 99;
	if((num & 0xf) > 9) //проверяем младшую тетраду
	{
		num -= 0x6; //отнимаем 6 если запрещенное значение (A..F)
	}
	return num;
}

bool DS1307_Read()
{
	//if(timer[DS1307_READ] != 0) return true;
	//uint8_t tim[sizeof(tm)];
	I2C_ReadByAdr(0, (uint8_t*)&tm, sizeof(tm));
	
	//tm.tm_sec = tim[0];
	//tm.tm_min = tim[1];
	//tm.tm_hour = tim[2];
	//tm.tm_wday = tim[3];
	//tm.tm_date = tim[4];
	//tm.tm_mon = tim[5];
	//tm.tm_year = tim[6];
	//TaskStart(DS1307_READ, DS1307_READTIME);
	return true;
}

bool DS1307_Save()
{
	tm.tm_sec = 0;
	I2C_WriteByAdr(0, (uint8_t*)&tm, sizeof(tm));
	return true;
}

bool DS1307_Config(uint8_t config)
{
	I2C_WriteByAdr(0x07, &config, 1);
	
	return true;
}

bool DS1307_Status()
{
	uint8_t data;
	I2C_ReadByAdr(0, &data, 1);
	if((data & (1 << CH_BIT)) != 0) return false;
	return true;	
}


bool DS1307_ReadRam(uint8_t adr, uint8_t* data, uint8_t size)
{
	if(adr + size > DS1307_RAMSIZE) return false;
	I2C_ReadByAdr(adr + RAM_START, data, size);
	return true;
}

bool DS1307_WriteRam(uint8_t adr, uint8_t* data, uint8_t size)
{
	if(adr + size > DS1307_RAMSIZE) return false;
	I2C_WriteByAdr(adr + RAM_START, data, size);
	return true;
}

void DS1307_incrMin()
{
	if(tm.tm_min == 0x59) tm.tm_min = 0;
	else tm.tm_min = IncrBCD(tm.tm_min);
}

void DS1307_decrMin()
{
	if(tm.tm_min == 0) tm.tm_min = 0x59;
	else tm.tm_min = DecrBCD(tm.tm_min);
}

void DS1307_incrHour()
{
	if(tm.tm_hour == 0x23) tm.tm_hour = 0;
	else tm.tm_hour = IncrBCD(tm.tm_hour);
}

void DS1307_decrHour()
{
	if(tm.tm_hour == 0) tm.tm_hour = 0x23;
	else tm.tm_hour = DecrBCD(tm.tm_hour);
}

void DS1307_incrDay()
{
	if(tm.tm_wday == 7) tm.tm_wday = 1;
	else tm.tm_wday++;
}

void DS1307_decrDay()
{
	if(tm.tm_wday == 1) tm.tm_wday = 7;
	else tm.tm_wday--;
}

void DS1307_incrDate()
{
	if(tm.tm_date == 0x31) tm.tm_date = 1;
	else tm.tm_date = IncrBCD(tm.tm_date);
}

void DS1307_decrDate()
{
	if(tm.tm_date == 1) tm.tm_date = 0x31;
	else tm.tm_date = DecrBCD(tm.tm_date);
}

void DS1307_incrMonth()
{
	if(tm.tm_mon == 0x12) tm.tm_mon = 1;
	else tm.tm_mon = IncrBCD(tm.tm_mon);
}

void DS1307_decrMonth()
{
	if(tm.tm_mon == 1) tm.tm_mon = 0x12;
	else tm.tm_mon = DecrBCD(tm.tm_mon);
}

void DS1307_incrYear()
{
	tm.tm_year = IncrBCD(tm.tm_year);
}

void DS1307_decrYear()
{
	tm.tm_year = DecrBCD(tm.tm_year);
}

void DS1307_Correction()
{
	//Перевод месяца из BDC в HEX
	uint8_t mon = (tm.tm_mon > 9) ? (tm.tm_mon - 6) : (tm.tm_mon);
	//Колличество дней в заданном месяце
	volatile uint8_t days = dayInMonth[mon];
	//Если февраль и высокосный год прибавляем ещё один день
	if((tm.tm_mon == 2) && (leapYear(tm.tm_year))) days++;
	//Скорректировать число до максимального если установлено больше
	if(tm.tm_date > days) tm.tm_date = days;	
}


