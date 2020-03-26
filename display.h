/*
 * display.h
 *
 * Created: 14.09.2019 21:10:10
 *  Author: Dmitry
 */ 


#ifndef DISPLAY_H_
#define DISPLAY_H_

#define COMMON_ANODE		//Индикатор с общим анодом
//#define COMMON_CATODE		//Индикатор с общим катодом

//digits[]={0 ,ch1... ch0xf}

#define DISPLAY_DIGITS		4U
#define UPDATE_TIME			2U
#define ANIMATE_TIME		80U
#define DOTS_BLINK_TIME		510U

#define DOTS_PORT			PORTD
#define DOTS_PIN			4

#define COMMON_PORT			PORTC
#define COMMON_0			0
#define COMMON_1			1
#define COMMON_2			2
#define COMMON_3			3
#define COMMON_MASK			((1 << COMMON_0) | (1 << COMMON_1) | (1 << COMMON_2) | (1 << COMMON_3))

#define SEG_PORT			PORTB
#define SEG_A				0
#define SEG_B				2
#define SEG_C				3
#define SEG_D				4
#define SEG_E				5
#define SEG_F				6
#define SEG_G				7
#define SEG_MASK			((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_C) | (1 << SEG_D) | (1 << SEG_E) | (1 << SEG_F) | (1 << SEG_G))

#define PWM_PORT			OCR1A
#define LIGHT_SENS			ADCH

typedef enum
{
	CHAR_0 = 		((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_C) | (1 << SEG_D) | (1 << SEG_E) | (1 << SEG_F)),	//0
	CHAR_1 = 		((1 << SEG_B) | (1 << SEG_C)), //1
	CHAR_2 = 		((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_D) | (1 << SEG_E) | (1 << SEG_G)), //2
	CHAR_3 = 		((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_C) | (1 << SEG_D) | (1 << SEG_G)), //3
	CHAR_4 = 		((1 << SEG_B) | (1 << SEG_C) | (1 << SEG_F) | (1 << SEG_G)), //4
	CHAR_5 = 		((1 << SEG_A) | (1 << SEG_C) | (1 << SEG_D) | (1 << SEG_F) | (1 << SEG_G)), //5
	CHAR_6 = 		((1 << SEG_A) | (1 << SEG_C) | (1 << SEG_D) | (1 << SEG_E) | (1 << SEG_F) | (1 << SEG_G)), //6
	CHAR_7 = 		((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_C)), //7
	CHAR_8 = 		((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_C) | (1 << SEG_D) | (1 << SEG_E) | (1 << SEG_F) | (1 << SEG_G)), //8
	CHAR_9 = 		((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_C) | (1 << SEG_D) | (1 << SEG_F) | (1 << SEG_G)), //9
	CHAR_A = 		((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_C) | (1 << SEG_E) | (1 << SEG_F) | (1 << SEG_G)), //A
	CHAR_b = 		((1 << SEG_C) | (1 << SEG_D) | (1 << SEG_E) | (1 << SEG_F) | (1 << SEG_G)), //b
	CHAR_C = 		((1 << SEG_A) | (1 << SEG_D) | (1 << SEG_E) | (1 << SEG_F)) , //C
	CHAR_d = 		((1 << SEG_B) | (1 << SEG_C) | (1 << SEG_D) | (1 << SEG_E) | (1 << SEG_G)), //d
	CHAR_E = 		((1 << SEG_A) | (1 << SEG_D) | (1 << SEG_E) | (1 << SEG_F) | (1 << SEG_G)), //E
	CHAR_F = 		((1 << SEG_A) | (1 << SEG_E) | (1 << SEG_F) | (1 << SEG_G)), //F
	CHAR_ll = 		((1 << SEG_B) | (1 << SEG_C) | (1 << SEG_E) | (1 << SEG_F)), //ll
	CHAR_L	=		((1 << SEG_D) | (1 << SEG_E) | (1 << SEG_F)), //L
	CHAR_RU_P =		((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_C) | (1 << SEG_E) | (1 << SEG_F)), //П
	CHAR_RU_G =		((1 << SEG_A) | (1 << SEG_E) | (1 << SEG_F)), //Г
	CHAR_RU_B =		((1 << SEG_A) | (1 << SEG_C) | (1 << SEG_D) | (1 << SEG_E) | (1 << SEG_F) | (1 << SEG_G)), //8
	CHAR_P = 		((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_E) | (1 << SEG_F) | (1 << SEG_G)), //Р
	CHAR_t =		((1 << SEG_D) | (1 << SEG_E) | (1 << SEG_F) | (1 << SEG_G)), //8
	CHAR_H =		((1 << SEG_B) | (1 << SEG_C) | (1 << SEG_E) | (1 << SEG_F) | (1 << SEG_G)), //H
	CHAR_GRAD =		((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_F) | (1 << SEG_G)), //кружок значка градуса
	CHAR_PROCHERK = (1 << SEG_G),	//прочерк
	CHAR_NOTHIN =	0,
	CHAR_SKOBKAZ = ((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_C) | (1 << SEG_D)), //)
}disp_sym;

static const disp_sym digits[] =
{
	CHAR_0,
	CHAR_1,
	CHAR_2,
	CHAR_3,
	CHAR_4,
	CHAR_5,
	CHAR_6,
	CHAR_7,
	CHAR_8,
	CHAR_9,
	CHAR_A,
	CHAR_b,
	CHAR_C,
	CHAR_d,
	CHAR_E,
	CHAR_F,
	CHAR_ll,
	CHAR_L,
	CHAR_RU_P,
	CHAR_RU_G,
	CHAR_RU_B,
	CHAR_P,
	CHAR_t,
	CHAR_H,
	CHAR_GRAD,
	CHAR_PROCHERK,
	CHAR_NOTHIN,
	CHAR_SKOBKAZ,
};


//---------- Отобразить содержимое буфера (при каждом вызове последовательно отображаются цыфры) --------------
void DisplayUpdate();
//---------- Запись числа в буфер на отображение ------------------
void DisplaySet_Int(uint16_t set, uint8_t bitmask, bool animate);
//-------------- Запись букв и цифр в буфер -------------------
void DisplaySet_Char(disp_sym* sym, bool animate);
//---------- Отключить индикацию ------------------
void DisplayOff(void);
//---------- Включить индикацию ---------------
void DisplayOn(void);
//---------- Функция анимирования смены цыфр ------------
void DisplayAnimate(void);
//---------- Мигание выбранной цифрой ---------------
void DisplayBlinkOn(uint8_t bitmask, uint8_t period);
//---------- Остановить мигание -------------------
void DisplayBlinkOff(void);
//----------- Конвертация INT16 to disp_sym --------------
void DisplayIntToChar(uint16_t in, disp_sym* out);
//----------- Конвертация дня недели INT8 to disp_sym ---------
void DisplayDayToChar(uint8_t in, disp_sym* out, uint8_t offset);


//Включить точки
void DotsOn();
//Выключить точки
void DotsOff();
//Функция включения мигания точек
void DotsBlinkOn();
//Мигание точек, функция должна быть помещена в бесконечный while
void DotsBlink();



#endif /* DISPLAY_H_ */