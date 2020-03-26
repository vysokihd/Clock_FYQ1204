/*
 * DS18B20.h
 *
 * Created: 10.10.2019 13:41:26
 *  Author: d.vysokih
 */ 


#ifndef DS18B20_H_
#define DS18B20_H_

/* ���������� ���� ��������� ������ */
#define THERM_PORT PORTD
#define THERM_DDR DDRD
#define THERM_PIN PIND
#define THERM_DQ PD2

extern int16_t tempHex;

typedef enum
{
	DS18B20_STANDBY = 0,		//����� ��������
	DS18B20_DATAREADY,			//������ ������, ����� ������
	DS18B20_BUSY,				//��� ����� �������
	DS18B20_NA,					//������ �� ��������
}ds18b20;

//����� ������ �����������
void Ds18b20_ConvertTemp();
//������ �����������
int16_t Ds1820_ReadTempBCD(int8_t compens);
//��������� ��������
ds18b20 Ds18b20_GetStatus(); 


#endif /* DS18B20_H_ */