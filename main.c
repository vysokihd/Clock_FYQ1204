/*
 * Clock_FYQ1204.c
 *
 * Created: 12.09.2019 0:04:54
 * Author : Dmitry
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>
#include "main.h"
#include "display.h"
#include "i2c.h"
#include "ds1307.h"
#include "DS18B20.h"
#include "Buttons.h"
#include "dimmer.h"

volatile bool Ds1307_ready = false;
disp_sym sym[] = {CHAR_H, CHAR_E, CHAR_ll, CHAR_0};
MODE mode = MODE_DEMO;
static int8_t tempComp = 0;				//Коррекция температуры
static uint8_t dimmLevel = 0;			//Режим диммирования
static bool nightMode = false;			//Ночной режим (вкл. выкл)
static bool isNight = false;			//Сейчас ночь

static volatile uint32_t timerTick = 0;			//Таймер - 1мс
static volatile uint8_t  rtcTick = 0;			//Таймер - 1с

static void McuInit();				//Инициализация контроллера
static void DS1307_init();			//Инициализация RTC
static void Mode_Clock();			//Реализация режимов работы часов
static void NightMode();			//Автоматическая активация ночного режима по времени

int main(void)
{
	McuInit();				//Инициализация контроллера
	sei();					//Глобально разрешить прерывания
	DS1307_init();				//Инициализация RTC
	DimmSet(dimmVal[dimmLevel]);		//Установка яркости дисплея
	
	//Поставить в очередь на выполнение след. функция
	TaskStart(BUT_GETSTATE, 0);		//Button_GetState()
	TaskStart(MODE_CLOCK, 0);		//Mode_Clock()
	
	
    while (1) 
    {
		Button_GetState();
		Mode_Clock();		
		DisplayAnimate();
		Ds18b20_ConvertTemp();
		NightMode();
		Dimm();		
	}
}

	//Инициализация контроллера и переферии
void McuInit()
{
	//-------- Инициализация глобальных переменнных ------------
	for (uint8_t i=0; i < PROCESS_COUNT; i++)
	{
		TaskStop(i);
	}

	//------------ Инициализация портов ввода-вывода ------------
	//ВЫХОДЫ
	DDRB = SEG_MASK |			//Порт сегментов
			(1 << 1);			//PWM порт
	DDRC = COMMON_MASK;			//Порт цифр
	DDRD = (1 << DOTS_PIN);		//Порт точек
	
	//ВХОДЫ
	PORTD = (1 << BUT_PIN_INC) | (1 << BUT_PIN_DEC) | (1 << BUT_PIN_MODE) |		//Порт кнопок (вкл.подтяжку)
			(1 << 3);				//Подтяжка INT1 для работы внешнего прерывания от DS1307
	PORTC = (1 << 4) | (1 << 5);	//Подтяжка I2C SDA, SCL  
	
	//-------------- Инициализация таймера Т1 -------------------
	//настройка таймера на FastPWM
#if defined(COMMON_ANODE)
	TCCR1A = (1 << COM1A1) | (1 << COM1A0) |		//Поведение вывода при работе ШИМ
			 (0 << WGM11) | (1 << WGM10);			//Выбор режима 8bit fastPWM
#elif defined(COMMON_CATODE)
	TCCR1A = (1 << COM1A1) | (0 << COM1A0) |		//Поведение вывода при работе ШИМ
			 (0 << WGM11) | (1 << WGM10);			//Выбор режима 8bit fastPWM
#endif
	TCCR1B = (0 << WGM13) | (1 << WGM12) |			//Выбор режима 8bit fastPWM
			 (0 << CS02) | (0 << CS01) | (1 << CS00);	//выбор делителя 1 и запуск таймера
	OCR1A = 0xff;
	
	//-------------- Инициализация таймера Т2 -------------------
	//настройка таймера на генерацию прерывания раз в 1мс при 8Мгц
	OCR2 = 250;							//для достижения большей точности считаем до 250
	TIMSK = (1 << OCIE2);						//разрешение прерывания при совпадении с регистром сравнения (OCR2)
	TCCR2 = (1 << WGM21) | (0 << WGM20) |				//Выбор режима: "сброс при совпадении"
			(0 << CS02) | (1 << CS01) | (1 << CS00);		//выбор делителя 32 (и запуск таймера)

	//-------------- Инициализация TWI --------------------------
	TWBR = (F_CPU/F_SCL - 16) / 2;					//Выбор частоты работы шины 100кГц
	I2C_TargetSet(DS1307_ADR);					//Установка адреса RTC на шине I2C
	
	//-------------- Инициализация прерывания INT1 ---------------
	MCUCR = (1 << ISC11) | ( 0 << ISC10);
	GICR = (1 << INT1);
	
	//------------------ Инициализация АЦП ----------------------
	ADMUX = (1 << REFS1) | (1 << REFS0) | (1 << ADLAR) | (0b111 << MUX0); //Внутреннее опорное 2.54В | выравнивание влево | подкл. в ADC7
	ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADFR) | (0 << ADIE) | (0b111 << ADPS0); //АЦП вкл | старт | циклическое | прерывание | делитель
}
	//Инициализация DS1307
void DS1307_init()
{
	//Если DS1307 не были инициализированы
	if(DS1307_Status() == false)
	{
		tm.tm_sec = 0x00;
		tm.tm_min = 0x00;
		tm.tm_hour = 0x00;
		tm.tm_wday = 0x02;
		tm.tm_date = 0x24;
		tm.tm_mon = 0x09;
		tm.tm_year = 0x19;
		DS1307_Save();
		DS1307_WriteRam(0, &tempComp, 1);
		DS1307_WriteRam(1, &nightMode, 1);
		DS1307_WriteRam(2, &dimmLevel, 1);
	}
	else
	{
		DS1307_ReadRam(0, &tempComp, 1);
		DS1307_ReadRam(1, &nightMode, 1);	
		DS1307_ReadRam(2, &dimmLevel, 1);	
	}
		DS1307_Config(1 << SQWE_BIT);		//настройка выхода 1Гц
}


/********************************************************************/
//                    Режимы работы часов
/********************************************************************/
	//Функция возврата в нормальный режим работы с сохр. настроек (для Mode_Clock)
static void Back_Normal_Mode()
{
	mode = MODE_NORM_TIME;
	//DotsBlinkOn();
	DisplayBlinkOff();
	DS1307_Correction();
	DS1307_Save();
	rtcTick = 0;
}
	//Вывод на дисплей текущей температуры (для Mode_Clock)
static void Temp_Display()
{
	int16_t tempr;
	ds18b20 sts = Ds18b20_GetStatus();
	if(sts == DS18B20_NA)
	{
		sym[0] = CHAR_PROCHERK;
		sym[1] = CHAR_PROCHERK;
		sym[2] = CHAR_PROCHERK;
		sym[3] = CHAR_PROCHERK;
		DisplaySet_Char(sym, true);
	}
	else if(sts == DS18B20_DATAREADY)
	{
		tempr = Ds1820_ReadTempBCD(tempComp);
		//DotsOff();
		if(tempr >= 0)
		{
			DisplayIntToChar((tempr << 8), sym);
			sym[2] = CHAR_GRAD;
			sym[3] = CHAR_C;
		}
		else
		{
			DisplayIntToChar((tempr << 4), sym);
			sym[0] = CHAR_PROCHERK;
			sym[3] = CHAR_GRAD;
		}
		DisplaySet_Char(sym, true);
	}
}
	//Вывод на дисплей текущего дня недели (для Mode_Clock)
static void WDay_Display()
{
	DisplayIntToChar((tm.tm_wday << 8), sym);
	DisplayDayToChar(tm.tm_wday, sym, 1);
	sym[0] = CHAR_PROCHERK;
	sym[3] = CHAR_PROCHERK;
	DisplaySet_Char(sym, false);
}
	//Реализация режимов работы часов
void Mode_Clock()
{
	if(timer[MODE_CLOCK] != 0) return;
	bool animate = false;
	
	switch(mode)
	{
		//------------- Режим демки при включении ---------------
		case MODE_DEMO:
			DotsOff();
			DisplaySet_Char(sym, false);
			if(rtcTick == 2)
			{
				mode = MODE_NORM_TIME;
				//DotsBlinkOn();
				rtcTick = 0;
			}
		break;
		//++++++++++++++ Нормальный режим работы ++++++++++++++++
		//-------- Режим индикации времени -----------
		case MODE_NORM_TIME:
			//Чтение времение из I2C RTC модуля
			if(Ds1307_ready)
			{
				DS1307_Read();
				Ds1307_ready = false;
			}
			
			/*Если разрешён ночной режим и сейчас ночь обнуляем rtcTick
			для того что бы режим отображения времени не менялся
			*/
			if(nightMode && isNight)
			{
				 rtcTick = 0;
				 animate = false;
				 DotsOn();
			}
			else
			{
				 animate = true;
				 DotsBlinkOn();
			}
			
			if((tm.tm_hour & (0xf0)) == 0)
			{
				DisplaySet_Int(((tm.tm_hour << 8) | (tm.tm_min)), 0b0111 , animate);
			}
			else DisplaySet_Int(((tm.tm_hour << 8) | (tm.tm_min)), 0b1111 , animate);
			
			if(rtcTick == 55)
			{
				//запуск измерения температуры
				TaskStart(TEMP_CONVERT, 0);
				mode = MODE_NORM_DATE;
				rtcTick = 0;
			}
			//Проверка долгого удержания кнопки режим
			if(Button_LongPress(BUT_PIN_MODE, BUT_PORT))
			{
				mode = MODE_SET_HOUR;
				rtcTick = 0;
			}
			//Вход в режим коррекции температуры
			if(Button_LongPress(BUT_PIN_INC, BUT_PORT))
			{
				mode = MODE_SET_TEMP;
				rtcTick = 0;
			}
			//Вход в коррекцию режимов работы дисплея
			if(Button_LongPress(BUT_PIN_DEC, BUT_PORT))
			{
				mode = MODE_SET_DIMM;
			}
			//Вход в режим отображения температуры
			if(Button_ShortPress(BUT_PIN_INC, BUT_PORT))
			{
				TaskStart(TEMP_CONVERT, 0);
				mode = MODE_MAN_TEMP;
				rtcTick = 0;
			}
			//Вход в режим индикации даты
			if(Button_ShortPress(BUT_PIN_DEC, BUT_PORT))
			{
				mode = MODE_MAN_DATE;
				rtcTick = 0;
			}
			//Вход в режим индикации минут-секунд
			if(Button_ShortPress(BUT_PIN_MODE, BUT_PORT))
			{
				mode = MODE_MAN_SEC;
				rtcTick = 0;
				DotsOn();
			}			
		break;
		
		//-------- Режим индикации дня недели и даты -----------
		case MODE_NORM_DATE:
			DotsOff();
			DisplayIntToChar(((tm.tm_wday << 8) | (tm.tm_date)), sym);
			DisplayDayToChar(tm.tm_wday, sym, 0);
			DisplaySet_Char(sym, true);
			if(rtcTick == 2)
			{
				mode = MODE_NORM_TEMP;
				rtcTick = 0;
			}
		break;
		
		//------------- Режим индикации температуры -------------
		case MODE_NORM_TEMP:
			if(rtcTick == 0) Temp_Display();
			if(rtcTick == 2)
			{
				mode = MODE_NORM_TIME;
				//DotsBlinkOn();
				rtcTick = 0;
			}
		break;
		
		//------------- Режим ручной индикации темпаратуры -----------
		case MODE_MAN_TEMP:
			DotsOff();		
			Temp_Display();		
			if(rtcTick == 4)
			{
				mode = MODE_NORM_TIME;
				//DotsBlinkOn();
				rtcTick = 0;
			}
		break;
		
		//------------- Режим ручной индикации даты -------------
		case MODE_MAN_DATE:
			//показываем число и месяц
			if(rtcTick == 1)
			{
				DotsOn();
				DisplaySet_Int(((tm.tm_date << 8) | (tm.tm_mon)), 0b1111, true);
			}
			//отобразить день недели
			if(rtcTick == 4)
			{
				 DotsOff();
				 WDay_Display();
			}
			if(rtcTick == 6)
			{
				mode = MODE_NORM_TIME;
				//DotsBlinkOn();
				rtcTick = 0;
			}
		break;
		
		//------------ Режим индикации минут - секунд
		case MODE_MAN_SEC:
			//Чтение времение из I2C RTC модуля
			if(Ds1307_ready)
			{
				DS1307_Read();
				Ds1307_ready = false;
			}
			DisplaySet_Int(((tm.tm_min << 8) | (tm.tm_sec)), 0b1111 , false);
			if(Button_ShortPress(BUT_PIN_MODE, BUT_PORT))
			{
				//DotsBlinkOn();
				mode = MODE_NORM_TIME;
			}
			if(Button_LongPress(BUT_PIN_MODE, BUT_PORT))
			{
				if(tm.tm_sec > 0x30)
				{
					DS1307_incrMin();
				}
				tm.tm_sec = 0;
				DS1307_Save();
			}
		break;	
		
		//++++++++++++++ Режим настройки даты и времени ++++++++++++++++
		//----------------- Установка часов -------------------
		case MODE_SET_HOUR:
			DotsOn();
			DisplaySet_Int(((tm.tm_hour << 8) | (tm.tm_min)), 0b1111 , false);
			DisplayBlinkOn(0b1100, 4);
			if(Button_ShortPress(BUT_PIN_DEC, BUT_PORT))
			{
				DS1307_decrHour();
			}
			if(Button_ShortPress(BUT_PIN_INC, BUT_PORT))
			{
				DS1307_incrHour();
			}
			if(Button_ShortPress(BUT_PIN_MODE, BUT_PORT))
			{
				mode = MODE_SET_MINUT;
				DisplayBlinkOn(0b0011, 4);
			}
			if(Button_LongPress(BUT_PIN_MODE, BUT_PORT))
			{
				Back_Normal_Mode();
			}
		break;
		
		//-------- Установка минут -----------
		case MODE_SET_MINUT:
			DisplaySet_Int(((tm.tm_hour << 8) | (tm.tm_min)), 0b1111 , false);
			if(Button_ShortPress(BUT_PIN_DEC, BUT_PORT))
			{
				DS1307_decrMin();
			}
			if(Button_ShortPress(BUT_PIN_INC, BUT_PORT))
			{
				DS1307_incrMin();
			}
			if(Button_ShortPress(BUT_PIN_MODE, BUT_PORT))
			{
				mode = MODE_SET_DATE;
				DisplayBlinkOn(0b1100, 4);
			}
			if(Button_LongPress(BUT_PIN_MODE, BUT_PORT))
			{
				Back_Normal_Mode();
			}
		break;
		
		//--------- Установка даты ------------
		case MODE_SET_DATE:
			DisplaySet_Int(((tm.tm_date << 8) | (tm.tm_mon)), 0b1111 , false);
			if(Button_ShortPress(BUT_PIN_DEC, BUT_PORT))
			{
				DS1307_decrDate();
			}
			if(Button_ShortPress(BUT_PIN_INC, BUT_PORT))
			{
				DS1307_incrDate();
			}
			if(Button_ShortPress(BUT_PIN_MODE, BUT_PORT))
			{
				mode = MODE_SET_MON;
				DisplayBlinkOn(0b0011, 4);
			}
			if(Button_LongPress(BUT_PIN_MODE, BUT_PORT))
			{
				Back_Normal_Mode();
			}
		break;
		
		//--------- Установка месяца ---------
		case MODE_SET_MON:
			DisplaySet_Int(((tm.tm_date << 8) | (tm.tm_mon)), 0b1111 , false);
			if(Button_ShortPress(BUT_PIN_DEC, BUT_PORT))
			{
				DS1307_decrMonth();
			}
			if(Button_ShortPress(BUT_PIN_INC, BUT_PORT))
			{
				DS1307_incrMonth();
			}
			if(Button_ShortPress(BUT_PIN_MODE, BUT_PORT))
			{
				mode = MODE_SET_YEAR;
				//DisplayBlinkOn(0b1111, 4);
				DisplayBlinkOff();
				//DotsOff();
			}
			if(Button_LongPress(BUT_PIN_MODE, BUT_PORT))
			{
				Back_Normal_Mode();
			}
		break;
		//--------- Установка года ----------
		case MODE_SET_YEAR:
			DotsOff();
			DisplaySet_Int(((0x20 << 8) | (tm.tm_year)), 0b1111 , false);
			if(Button_ShortPress(BUT_PIN_DEC, BUT_PORT))
			{
				DS1307_decrYear();
			}
			if(Button_ShortPress(BUT_PIN_INC, BUT_PORT))
			{
				DS1307_incrYear();
			}
			if(Button_ShortPress(BUT_PIN_MODE, BUT_PORT))
			{
				mode = MODE_SET_DAY;
			}
			if(Button_LongPress(BUT_PIN_MODE, BUT_PORT))
			{
				Back_Normal_Mode();
			}
		break;
		//--------- Установка дня недели ----------
		case MODE_SET_DAY:
			WDay_Display();
			if(Button_ShortPress(BUT_PIN_DEC, BUT_PORT))
			{
				DS1307_decrDay();
			}
			if(Button_ShortPress(BUT_PIN_INC, BUT_PORT))
			{
				DS1307_incrDay();
			}
			if(Button_ShortPress(BUT_PIN_MODE, BUT_PORT))
			{
				mode = MODE_SET_HOUR;
				DS1307_Correction();
				DisplayBlinkOn(0b1100, 4);
			}
			if(Button_LongPress(BUT_PIN_MODE, BUT_PORT))
			{
				Back_Normal_Mode();
			}
		break;
		//------------------ Коррекция температуры -----------------
		case MODE_SET_TEMP:
			DotsOff();
			if(tempComp < 0)
			{
				DisplayIntToChar((tempComp << 8) * (-1), sym);
				sym[0] = CHAR_PROCHERK;
			}
			else
			{
				DisplayIntToChar((tempComp << 8), sym);
				sym[0] = CHAR_NOTHIN;
			}
			sym[2] = CHAR_GRAD;
			sym[3] = CHAR_C;
			DisplaySet_Char(sym, false);
			if(Button_ShortPress(BUT_PIN_MODE, BUT_PORT))
			{
				mode = MODE_NORM_TIME;
				DS1307_WriteRam(0, (uint8_t*)&tempComp, 1);
				rtcTick = 0;
				//DotsBlinkOn();
			}
			if(Button_ShortPress(BUT_PIN_DEC, BUT_PORT))
			{
				tempComp--;
				if(tempComp < -5 ) tempComp = 5;
			}
			if(Button_ShortPress(BUT_PIN_INC, BUT_PORT))
			{
				tempComp++;
				if(tempComp > 5 ) tempComp = -5;
			}
		break;
		
		//++++++++++++ Установка режимов дисплея +++++++++++++++++
		//------- Установка яркости дисплея ---------
		case MODE_SET_DIMM:
		DotsOff();
		if(Button_ShortPress(BUT_PIN_INC, BUT_PORT))
		{
			dimmLevel < DIMM_LEV_SIZE - 1 ? (dimmLevel++) : (dimmLevel = 0);
		}
		if(Button_ShortPress(BUT_PIN_DEC, BUT_PORT))
		{
			dimmLevel == 0 ? (dimmLevel = DIMM_LEV_SIZE - 1) : dimmLevel--;
		}
		if(Button_ShortPress(BUT_PIN_MODE, BUT_PORT))
		{
			mode = MODE_SET_NIGHT;
			DS1307_WriteRam(0x02, (uint8_t*)&dimmLevel, 1);
		}
		if(Button_LongPress(BUT_PIN_MODE, BUT_PORT))
		{
			mode = MODE_TEST_1;
		}
		
		DisplayIntToChar(dimmLevel, sym);
		sym[0] = CHAR_L;
		sym[1] = CHAR_PROCHERK;
		DisplaySet_Char(sym, false);
		
		//Установка значения PWM
		DimmSet(dimmVal[dimmLevel]);
		
		break;
		//-------- Включение-отключение ночного режима ----------
		case MODE_SET_NIGHT:
			if(Button_ShortPress(BUT_PIN_INC, BUT_PORT))
			{
				nightMode = true;
			}
			if(Button_ShortPress(BUT_PIN_DEC, BUT_PORT))
			{
				nightMode = false;
			}
			
			if(nightMode)
			{
				sym[0] = CHAR_H;
				sym[1] = CHAR_PROCHERK;
				sym[2] = CHAR_0;
				sym[3] = CHAR_H;
			}
			else
			{
				sym[0] = CHAR_H;
				sym[1] = CHAR_PROCHERK;
				sym[2] = CHAR_0;
				sym[3] = CHAR_F;
			}
			DisplaySet_Char(sym, false);
			if(Button_ShortPress(BUT_PIN_MODE, BUT_PORT))
			{
				mode = MODE_NORM_TIME;
				rtcTick = 0;
				//DotsBlinkOn();
				DS1307_WriteRam(0x01, (uint8_t*)&nightMode, 1);
			}
		break;
		
		//---------------------- Тестовый режим --------------------------
		case MODE_TEST_1:
		DotsOff();
		DisplaySet_Int(ADCH, 0b0011, false);
		if(Button_ShortPress(BUT_PIN_MODE,BUT_PORT))
		{
			mode = MODE_TEST_2;
		}
		break;
		case MODE_TEST_2:
		DisplaySet_Int(tempBcd, 0b1111, false);
		if(Button_ShortPress(BUT_PIN_MODE,BUT_PORT))
		{
			mode = MODE_NORM_TIME;
			//DotsBlinkOn();
			rtcTick = 0;
		}
		break;
		
	}
	TaskStart(MODE_CLOCK, 100);
}
/*
	! NightMode - автоматическая активация ночного режима по уровню шиммирования,
	который связан с датчиком освещённости
*/
static void NightMode()
{
	if(ADC_PORT > 10)
	{
		isNight = false;
	}
	else isNight = true;
}

//Возвращает время в милисекундах
uint32_t _time_ms()
{
	cli();
	uint32_t tm = timerTick;
	sei();
	return tm;
}

/********************************************************************/
//                    Системные прерывания
/********************************************************************/
	//Прерывание от таймера
ISR (TIMER2_COMP_vect)
{
	timerTick++;
	//Обновить индикацию на дисплее
	DisplayUpdate();
	//Декремент массива таймеров
	for(uint8_t i = 0; i < PROCESS_COUNT; i++)
	{
		if(timer[i] != 0 && timer[i] != 0xFFFF)
		{
			timer[i]--;
		}
	}
}
	//Внешнее прерывание от DS1307 период сигнала равен 1с
ISR (INT1_vect)
{
	Ds1307_ready = true;
	NightMode();
	DotsBlink();
	
	if(mode == MODE_TEST_2 && Ds18b20_GetStatus() != DS18B20_BUSY)
	{
		TaskStart(TEMP_CONVERT, 0);
	}
	
	if((MCUCR & (1 << ISC10)) != 0) rtcTick++;
	MCUCR ^= ( 1 << ISC10);
	/* MCUCR ^= ( 1 << ISC10)
	 меняет поочерёдно условие генерации внешнего прерывания фронт-спад
	 для того, что бы прерывание от DS1307 возникало 2 раза в секунду
	 и точки мигали равномерно с периодом 1 раз в секунду
	*/
	//if(isNight && nightMode) MCUCR |= (1 << ISC10);
	//else MCUCR ^= (1 << ISC10);
	//rtcTick++;
}
