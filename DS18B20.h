/*
 * DS18B20.h
 *
 * Created: 10.10.2019 13:41:26
 *  Author: d.vysokih
 */ 


#ifndef DS18B20_H_
#define DS18B20_H_

/* Определяем куда подключен датчик */
#define THERM_PORT PORTD
#define THERM_DDR DDRD
#define THERM_PIN PIND
#define THERM_DQ PD2

extern int16_t tempRaw;
extern int16_t tempHex;
extern int16_t tempBcd;

typedef enum
{
	DS18B20_STANDBY = 0,		//Режим ожидания
	DS18B20_DATAREADY,			//Данные готовы, можно читать
	DS18B20_BUSY,				//Идёт обмен данными
	DS18B20_NA,					//Датчик не отвечает
}ds18b20;

//Старт замера температуры
void Ds18b20_ConvertTemp();
//Чтение температуры
//int16_t Ds1820_ReadTempBCD();
//Получение сотояния
ds18b20 Ds18b20_GetStatus(); 


#endif /* DS18B20_H_ */