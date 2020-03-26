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
disp_sym sym[] = {CHAR_8, CHAR_8, CHAR_8, CHAR_8};
MODE mode = MODE_DEMO;
static int8_t tempComp = 0;			//��������� �����������
static uint8_t dimmMode = 0;		//����� ������������
static bool nightMode = false;		//������ ����� (���. ����)
static bool isNight = false;		//������ ����

static void McuInit();				//������������� �����������
static void DS1307_init();			//������������� RTC
static void Mode_Clock();			//���������� ������� ������ �����
static void NightMode();			//�������������� ��������� ������� ������ �� �������



int main(void)
{
	McuInit();					//������������� �����������
	Button_Init();				//������������� ������
	sei();						//��������� ��������� ����������
	DS1307_init();				//������������� RTC
	DimmSet(dimmVal[dimmMode]);	//��������� ������� �������
	
	//��������� � ������� �� ���������� ����. �������
	TaskStart(BUT_GETSTATE, 0);		//Button_GetState()
	TaskStart(MODE_CLOCK, 0);		//Mode_Clock()
	
	
    while (1) 
    {
		Button_GetState();
		Mode_Clock();		
		DisplayAnimate();
		Ds18b20_ConvertTemp();
		DotsBlink();
		NightMode();
		Dimm();		
	}
}

	//������������� ����������� � ���������
void McuInit()
{
	//-------- ������������� ���������� ����������� ------------
	for (uint8_t i=0; i < PROCESS_COUNT; i++)
	{
		TaskStop(i);
	}
	timerTick = 0;
	rtcTick = 0;
	
	
	//------------ ������������� ������ �����-������ ------------
	//������
	DDRB |= SEG_MASK;				//���� ��������� - �����
	DDRC |= COMMON_MASK;			//���� ���� - �����
	DDRD |= (1 << DOTS_PIN);		//���� ����� - �����
	DDRB |= (1 << 1);
	//PORTB |= (1 << 1);
	//�����
	PORTD |= (1 << BUT_PIN_INC) | (1 << BUT_PIN_DEC) | (1 << BUT_PIN_MODE);	//���� ������ (���.��������)
	
	//-------------- ������������� ������� �1 -------------------
	//��������� ������� �� FastPWM
	TCCR1A |= (1 << COM1A0) | (1 << COM1A1);			//��������� ������ ��� ������ ���
	TCCR1A |= (0 << WGM13) | (1 << WGM12) | (0 << WGM11) | (1 << WGM10);
	TCCR1B |= (0 << CS02) | (1 << CS01) | (0 << CS00);	//����� �������� 8 � ������ �������
	OCR1A = 0xff;
	
	//-------------- ������������� ������� �2 -------------------
	//��������� ������� �� ��������� ���������� ��� � 1�� ��� 8���
	OCR2 = 250;											//��� ���������� ������� �������� ������� �� 250
	TIMSK |= (1 << OCIE2);								//���������� ���������� ��� ���������� � ��������� ��������� (OCR2)
	TCCR2 |= (1 << WGM21) | (0 << WGM20);				//����� ������: "����� ��� ����������"
	TCCR2 |= (0 << CS02) | (1 << CS01) | (1 << CS00);	//����� �������� 32 (� ������ �������)

	//-------------- ������������� TWI --------------------------
	TWBR = (F_CPU/F_SCL - 16) / 2;						//����� ������� ������ ���� 100���
	I2C_TargetSet(DS1307_ADR);							//��������� ������ RTC �� ���� I2C
	
	//-------------- ������������� ���������� INT1 ---------------
	MCUCR = (1 << ISC11) | ( 0 << ISC10);
	GIMSK = (1 << INT1);
	
	//------------------ ������������� ��� ----------------------
	ADMUX = (1 << REFS1) | (1 << REFS0) | (1 << ADLAR) | (0b111 << MUX0); //���������� ������� 2.54� | ������������ ����� | �����. � ADC7
	ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADFR) | (0 << ADIE) | (0b111 << ADPS0); //��� ��� | ����� | ����������� | ���������� | ��������
}
	//������������� DS1307
void DS1307_init()
{
	//���� DS1307 �� ���� ����������������
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
		DS1307_WriteRam(0, (uint8_t*)&tempComp, 1);
		DS1307_WriteRam(1, (uint8_t*)&nightMode, 1);
		DS1307_WriteRam(2, (uint8_t*)&dimmMode, 1);
	}
	else
	{
		DS1307_ReadRam(0, (uint8_t*)&tempComp, 1);
		DS1307_ReadRam(1, (uint8_t*)&nightMode, 1);	
		DS1307_ReadRam(2, (uint8_t*)&dimmMode, 1);	
	}
		DS1307_Config(1 << SQWE_BIT);		//��������� ������ 1��
}


/********************************************************************/
//                    ������ ������ �����
/********************************************************************/
	//������� �������� � ���������� ����� ������ � ����. �������� (��� Mode_Clock)
static void Back_Normal_Mode()
{
	mode = MODE_NORM_TIME;
	DotsBlinkOn();
	DisplayBlinkOff();
	DS1307_Correction();
	DS1307_Save();
	rtcTick = 0;
}
	//����� �� ������� ������� ����������� (��� Mode_Clock)
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
	//����� �� ������� �������� ��� ������ (��� Mode_Clock)
static void WDay_Display()
{
	DisplayIntToChar((tm.tm_wday << 8), sym);
	DisplayDayToChar(tm.tm_wday, sym, 1);
	sym[0] = CHAR_PROCHERK;
	sym[3] = CHAR_PROCHERK;
	DisplaySet_Char(sym, false);
}
	//���������� ������� ������ �����
void Mode_Clock()
{
	if(timer[MODE_CLOCK] != 0) return;
	switch(mode)
	{
		//------------- ����� ����� ��� ��������� ---------------
		case MODE_DEMO:
			DotsOn();
			DisplaySet_Char(sym, false);
			if(rtcTick == 2)
			{
				mode = MODE_NORM_TIME;
				DotsBlinkOn();
				rtcTick = 0;
			}
		break;
		//++++++++++++++ ���������� ����� ������ ++++++++++++++++
		//-------- ����� ��������� ������� -----------
		case MODE_NORM_TIME:
			if(Ds1307_ready)
			{
				DS1307_Read();
				Ds1307_ready = false;
			}
			/*���� �������� ������ ����� � ������ ���� �������� rtcTick
			��� ���� ��� ��  ����� ����������� ������� �� �������
			*/
			if(nightMode && isNight) rtcTick = 0;
			
			if((tm.tm_hour & (0xf0)) == 0)
			{
				DisplaySet_Int(((tm.tm_hour << 8) | (tm.tm_min)), 0b0111 , true);
			}
			else DisplaySet_Int(((tm.tm_hour << 8) | (tm.tm_min)), 0b1111 , true);
			
			if(rtcTick == 15)
			{
				//������ ��������� �����������
				TaskStart(TEMP_CONVERT, 0);
				mode = MODE_NORM_DATE;
				rtcTick = 0;
			}
			//�������� ������� ��������� ������ �����
			if(Button_LongPress(BUT_PIN_MODE, BUT_PORT))
			{
				mode = MODE_SET_HOUR;
				rtcTick = 0;
			}
			//���� � ����� ��������� �����������
			if(Button_LongPress(BUT_PIN_INC, BUT_PORT))
			{
				mode = MODE_SET_TEMP;
				rtcTick = 0;
			}
			//���� � ��������� ������� ������ �������
			if(Button_LongPress(BUT_PIN_DEC, BUT_PORT))
			{
				mode = MODE_SET_DIMM;
			}
			//���� � ����� ����������� �����������
			if(Button_ShortPress(BUT_PIN_INC, BUT_PORT))
			{
				TaskStart(TEMP_CONVERT, 0);
				mode = MODE_MAN_TEMP;
				rtcTick = 0;
			}
			//���� � ����� ��������� ����
			if(Button_ShortPress(BUT_PIN_DEC, BUT_PORT))
			{
				mode = MODE_MAN_DATE;
				rtcTick = 0;
			}
		break;
		
		//-------- ����� ��������� ��� ������ � ���� -----------
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
		
		//------------- ����� ��������� ����������� -------------
		case MODE_NORM_TEMP:
			if(rtcTick == 0) Temp_Display();
			if(rtcTick == 2)
			{
				mode = MODE_NORM_TIME;
				DotsBlinkOn();
				rtcTick = 0;
			}
		break;
		
		//------------- ����� ������ ��������� ����������� -----------
		case MODE_MAN_TEMP:
			DotsOff();		
			Temp_Display();		
			if(rtcTick == 4)
			{
				mode = MODE_NORM_TIME;
				DotsBlinkOn();
				rtcTick = 0;
			}
		break;
		
		//------------- ����� ������ ��������� ���� -------------
		case MODE_MAN_DATE:
			//���������� ����� � �����
			if(rtcTick == 1)
			{
				DotsOn();
				DisplaySet_Int(((tm.tm_date << 8) | (tm.tm_mon)), 0b1111, true);
			}
			//���������� ���� ������
			if(rtcTick == 4)
			{
				 DotsOff();
				 WDay_Display();
			}
			if(rtcTick == 6)
			{
				mode = MODE_NORM_TIME;
				DotsBlinkOn();
				rtcTick = 0;
			}
		break;
		
		//++++++++++++++ ����� ��������� ���� � ������� ++++++++++++++++
		//----------------- ��������� ����� -------------------
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
		
		//-------- ��������� ����� -----------
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
		
		//--------- ��������� ���� ------------
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
		
		//--------- ��������� ������ ---------
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
		//--------- ��������� ���� ----------
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
		//--------- ��������� ��� ������ ----------
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
		//------------------ ��������� ����������� -----------------
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
				DotsBlinkOn();
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
		
		//++++++++++++ ��������� ������� ������� +++++++++++++++++
		//------- ��������� ������� ������� ---------
		case MODE_SET_DIMM:
		DotsOff();
		if(Button_ShortPress(BUT_PIN_INC, BUT_PORT))
		{
			dimmMode < L_CNT ? (dimmMode++) : (dimmMode = 0);
		}
		if(Button_ShortPress(BUT_PIN_DEC, BUT_PORT))
		{
			dimmMode == 0 ? (dimmMode = (L_CNT)) : dimmMode--;
		}
		if(Button_ShortPress(BUT_PIN_MODE, BUT_PORT))
		{
			mode = MODE_SET_NIGHT;
			DS1307_WriteRam(0x02, (uint8_t*)&dimmMode, 1);
		}
		if(Button_LongPress(BUT_PIN_MODE, BUT_PORT))
		{
			mode = MODE_TEST;
		}
		
		DisplayIntToChar(dimmMode, sym);
		sym[0] = CHAR_L;
		sym[1] = CHAR_PROCHERK;
		DisplaySet_Char(sym, false);
		
		//��������� �������� PWM
		DimmSet(dimmVal[dimmMode]);
		
		break;
		//-------- ���������-���������� ������� ������ ----------
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
				DotsBlinkOn();
				DS1307_WriteRam(0x01, (uint8_t*)&nightMode, 1);
			}
		break;
		
		//---------------------- �������� ����� --------------------------
		case MODE_TEST:
		DotsOff();
		DisplaySet_Int(ADCH, 0xff, false);
		if(Button_ShortPress(BUT_PIN_MODE,BUT_PORT))
		{
			mode = MODE_NORM_TIME;
			DotsBlinkOn();
			rtcTick = 0;
		}
		break;
	}
	TaskStart(MODE_CLOCK, 100);
}
	//�������������� ��������� ������� ������ �� �������
static void NightMode()
{
	uint8_t morning = 6;
	uint8_t evening = 0x23;
		
	//�������� ����-����
	if((tm.tm_hour >= morning) && (tm.tm_hour < evening))
	{
		isNight = false;
	}
	else isNight = true;
}

/********************************************************************/
//                    ��������� ����������
/********************************************************************/
	//���������� �� �������
ISR (TIMER2_COMP_vect)
{
	//�������� ��������� �� �������
	DisplayUpdate();
	
	//��������� ������� ��������
	for(uint8_t i = 0; i < PROCESS_COUNT; i++)
	{
		if(timer[i] != 0 && timer[i] != 0xFFFF)
		{
			timer[i]--;
		}
	}
}
	//���������� �� ���� �����������
ISR (INT1_vect)
{
	Ds1307_ready = true;
	NightMode();
	rtcTick++;
}