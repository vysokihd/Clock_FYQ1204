/*
 * main.h
 *
 * Created: 14.09.2019 20:32:38
 *  Author: Dmitry
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#define F_CPU 8000000UL	//частота MCU
#define F_SCL 100000UL	//частота TWI

//Список функций, выполняемых через программную очередь
typedef enum
{
	DISP_UPDATE,
	DISP_BLINK,
	DISP_ANIMATE,
	DIMMER,
	MODE_CLOCK,
	BUT_GETSTATE,
	TEMP_CONVERT,
	DOTS_BLINK,
	NIGHT_MODE,
	PROCESS_COUNT,
}Tasks;

//Список режимов работы часов
typedef enum
{
	MODE_DEMO,
	MODE_NORM_TIME,
	MODE_NORM_DATE,
	MODE_NORM_TEMP,
	MODE_SET_MINUT,
	MODE_SET_HOUR,
	MODE_SET_DATE,
	MODE_SET_MON,
	MODE_SET_YEAR,
	MODE_SET_DAY,
	MODE_SET_TEMP,
	MODE_MAN_TEMP,
	MODE_MAN_DATE,
	MODE_MAN_SEC,
	MODE_TEST_1,
	MODE_TEST_2,
	MODE_SET_DIMM,
	MODE_SET_NIGHT,
}MODE;

volatile uint16_t timer[PROCESS_COUNT];	//Массив программных таймеров

//Возвращает время в милисекундах
uint32_t _time_ms();

//Запуск задачи через заданное время
inline void TaskStart(Tasks task, uint16_t tim)
{
	cli();
	//Запускать задачу только если она выполнилась 
//	if(timer[task] == 0xFFFF)
	{
		timer[task] = tim;
	}
	sei();
}

//Остановка задачи
inline void TaskStop(Tasks task)
{
	cli();
	timer[task] = 0xFFFF;
	sei();
}

#endif /* MAIN_H_ */