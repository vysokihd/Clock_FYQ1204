/*
 * Buttons.h
 *
 * Created: 02.10.2019 16:36:27
 *  Author: d.vysokih
 */ 


#ifndef BUTTONS_H_
#define BUTTONS_H_

#define BUT_GETSTATE_TIME	10 //мс, период сканирования кнопок
#define BUT_RESET_TIME		10 //мс*10, время сброса необработанного состояния кнопок

#define BUT_LONG_PRES	150	//мс*10, длинное нажатие
#define BUT_SHORT_PRES	2	//мс*10, короткое нажатие

//#define BUT_NUMBER		3U		//количество кнопок

//-------- define PORT ------------
#define BUT_PORT		(void*)&PIND

//-------- define PIN -------------
#define BUT_PIN_INC		7
#define BUT_PIN_DEC		6
#define BUT_PIN_MODE	5

//Лист инициализации кнопок
#define BUT_LIST_INIT	{.port = BUT_PORT, .pin = BUT_PIN_MODE, .timer = 0, .pres = 0, .shortPres = 0, .longPres = 0, .prevState = 0},\
						{.port = BUT_PORT, .pin = BUT_PIN_DEC, .timer = 0, .pres = 0, .shortPres = 0, .longPres = 0, .prevState = 0},\
						{.port = BUT_PORT, .pin = BUT_PIN_INC, .timer = 0, .pres = 0, .shortPres = 0, .longPres = 0, .prevState = 0}

//Проверяет и запоминает состояние всех кнопок при каждом запуске
void Button_GetState(void);
//Определяет бы ло ли долгое нажатие, после считывания состояние обнуляется
bool Button_LongPress(uint8_t butPin, uint8_t* butPort);
//Определяет было ли короткое нажатие, после считывания состояние обнуляется
bool Button_ShortPress(uint8_t butPin, uint8_t* butPort);
//Определяет текущее состояние кнопки
bool Button_CurrentPress(uint8_t butPin, uint8_t* butPort);




#endif /* BUTTONS_H_ */