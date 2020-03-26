/*
 * Buttons.h
 *
 * Created: 02.10.2019 16:36:27
 *  Author: d.vysokih
 */ 


#ifndef BUTTONS_H_
#define BUTTONS_H_

#define BUT_GETSTATE_TIME	10U //��

#define BUT_LONG_PRES	150U	//������� �������, �� * 10
#define BUT_SHORT_PRES	1U		//�� * 10
#define BUT_NUMBER		3U		//���������� ������

//-------- define PORT ------------
#define BUT_PORT		(void*)&PIND

//-------- define PIN -------------
#define BUT_PIN_INC		7
#define BUT_PIN_DEC		6
#define BUT_PIN_MODE	5

//������ ��������� ������ ������������ ������
void Button_Init();
//��������� � ���������� ��������� ���� ������ ��� ������ �������
void Button_GetState(void);
//���������� �� �� �� ������ �������, ����� ���������� ��������� ����������
bool Button_LongPress(uint8_t butPin, uint8_t* butPort);
//���������� ���� �� �������� �������, ����� ���������� ��������� ����������
bool Button_ShortPress(uint8_t butPin, uint8_t* butPort);
//���������� ������� ��������� ������
bool Button_CurrentPress(uint8_t butPin, uint8_t* butPort);




#endif /* BUTTONS_H_ */