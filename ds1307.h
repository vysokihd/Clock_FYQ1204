/*
 * time.h
 *
 * Created: 28.09.2019 19:31:15
 *  Author: Dmitry
 */ 

#ifndef DS1307_H_
#define DS1307_H_

//#define DS1307_READTIME 200

#define DS1307_ADR	0xD0
#define RS0_BIT	0
#define RS1_BIT 1
#define SQWE_BIT 4
#define OUT_BIT 7

#define DS1307_RAMSIZE 56

volatile struct{
	uint8_t		tm_sec;			//секунды [0..59]
	uint8_t		tm_min;			//минуты - [0..59]
	uint8_t		tm_hour;		//часы - [0..23]
	uint8_t		tm_wday;		//день недели - [1-понедельник, 7-воскресенье]
	uint8_t		tm_date;		//дата - [1 to 31]
	uint8_t		tm_mon;			//месяц - [1 to 12]
	uint8_t		tm_year;		//год - [0 - 2000год, 99 - 2099год]
}tm;

//---------- считать данный и обновить структуру tm ---------------
bool DS1307_Read();
//---------- сохранить структуру в память ds1307 ------------------
bool DS1307_Save();
//---------- изменить данные в конфигурационном регистре ----------
bool DS1307_Config(uint8_t config);
//---------- прочитать статус ds1307 ------------------------------
bool DS1307_Status();
		//true - часы запущены
		//false - часы остановлены

//---------- прочитать из внутреннего RAM -----------------
bool DS1307_ReadRam(uint8_t adr, uint8_t* data, uint8_t size);

//---------- записать во внутренний RAM ---------------
bool DS1307_WriteRam(uint8_t adr, uint8_t* data, uint8_t size);

//-------------- Функции установки даты времени -------------------
void DS1307_incrMin();
void DS1307_decrMin();
void DS1307_incrHour();
void DS1307_decrHour();
void DS1307_incrDay();
void DS1307_decrDay();
void DS1307_incrDate();
void DS1307_decrDate();
void DS1307_incrMonth();
void DS1307_decrMonth();
void DS1307_incrYear();
void DS1307_decrYear(); 
//-------------- Функция корректировки даты ------------------------
void DS1307_Correction();
		//корректировка даты исходя из месяца и года

#endif /* TIME_H_ */