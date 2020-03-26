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
	uint8_t		tm_sec;			//������� [0..59]
	uint8_t		tm_min;			//������ - [0..59]
	uint8_t		tm_hour;		//���� - [0..23]
	uint8_t		tm_wday;		//���� ������ - [1-�����������, 7-�����������]
	uint8_t		tm_date;		//���� - [1 to 31]
	uint8_t		tm_mon;			//����� - [1 to 12]
	uint8_t		tm_year;		//��� - [0 - 2000���, 99 - 2099���]
}tm;

//---------- ������� ������ � �������� ��������� tm ---------------
bool DS1307_Read();
//---------- ��������� ��������� � ������ ds1307 ------------------
bool DS1307_Save();
//---------- �������� ������ � ���������������� �������� ----------
bool DS1307_Config(uint8_t config);
//---------- ��������� ������ ds1307 ------------------------------
bool DS1307_Status();
		//true - ���� ��������
		//false - ���� �����������

//---------- ��������� �� ����������� RAM -----------------
bool DS1307_ReadRam(uint8_t adr, uint8_t* data, uint8_t size);

//---------- �������� �� ���������� RAM ---------------
bool DS1307_WriteRam(uint8_t adr, uint8_t* data, uint8_t size);

//-------------- ������� ��������� ���� ������� -------------------
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
//-------------- ������� ������������� ���� ------------------------
void DS1307_Correction();
		//������������� ���� ������ �� ������ � ����

#endif /* TIME_H_ */