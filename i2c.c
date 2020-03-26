/*
 * i2c.c
 *
 * Created: 21.09.2019 23:40:26
 *  Author: Дмитрий
 */ 
 #include <avr/io.h>
 #include <avr/interrupt.h>
 #include <stdbool.h>
 #include "i2c.h"
 #include "display.h"
 
 enum
 {
	 I2C_BUS_FAIL = 0x00,			//ошибка на шине
	 I2C_START = 0x08,				//сформирован старт
	 I2C_RESTART = 0x10,			//сформировани повт.старт
	 I2C_SLA_W_ACK = 0x18,			//отправлен адрес с битом записи и получили ответ
	 I2C_SLA_W_NACK = 0x20,			//отправили адрес с битом записи и не получили ответ
	 I2C_BYTE_ACK = 0x28,			//послан байт - ведомый подтвердил
	 I2C_BYTE_NACK = 0x30,			//послан байт - ведомый не подтвердил
	 I2C_COLLISION = 0x38,			//обнаружен второй ведомый
	 I2C_SLA_R_ACK = 0x40,			//отправлен адрес с битом чтения и получили ответ
	 I2C_SLA_R_NACK = 0x48,			//отпраивли адрес с битом записи и не получили ответ
	 I2C_RECEIVE_BYTE = 0x50,		//принят байт от ведомого
	 I2C_RECEIVE_BYTE_NACK = 0x58,	//принят байт от ведомого, мастер ответил NACK
 };

static volatile uint8_t target;	//адрес подчиненного
static volatile uint8_t adres;	//адрес в подчиненом устройстве
static volatile uint8_t* data;	//массив данных для записи-чтения
static volatile uint8_t size;	//размер массива данных для чтения-записи
static volatile uint8_t	i_byte;	//счетчик
static volatile bool read;		//режим true - чтение, false - запись
static volatile bool busy;
static volatile uint8_t error;	//код ошибки


 
static inline void I2C_Start()
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);
}
static inline void I2C_Stop()
{
	//TWCR &= ~(1 << TWIE);
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}
static inline void I2C_Continue()
{
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
}
static inline void I2C_Ack()
{
	TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWIE) | (1<< TWEN);
}

static inline uint8_t I2C_GetStatus()
{
	return 	(TWSR & (0x1f << 3));
}

void I2C_TargetSet(uint8_t tg)
{
	target = tg & (~(1 << 0));
}

uint8_t I2C_WriteByAdr(uint8_t adr, uint8_t* dt, uint8_t sz)
{
	error = I2C_ERR_DEF;
	i_byte = 0;
	adres = adr;
	data = dt;
	size = sz;
	read = false;
	busy = true;
	I2C_Start();
	while(busy);
	return error;
}

uint8_t I2C_ReadByAdr(uint8_t adr, uint8_t* dt, uint8_t sz)
{
	error = I2C_ERR_DEF;
	i_byte = 0;
	adres = adr;
	data = dt;
	size = sz;
	read = true;
	busy = true;
	I2C_Start();
	while(busy);
	return error;	
}

ISR(TWI_vect)
{
	uint8_t status = I2C_GetStatus();
	//DisplaySet_Int(TWSR, 0xff , false);
	switch(status)
	{
		case I2C_BUS_FAIL:
			error = I2C_ERR_BF;
			break;
		case I2C_START:		//сформировани старт, пишем адрес устройства на шине I2C
			
			TWDR = target;	//установка target+W
			I2C_Continue();
			break;
		
		case I2C_RESTART:
			TWDR = target | ((1 << 0));	//установка target+R
			 I2C_Continue();
			break;
		
		case I2C_SLA_W_ACK:
			TWDR = adres;		//таргет ответил, пишем адрес с которым хотим работать
			I2C_Continue();
			break;
		
		case I2C_SLA_W_NACK:	//отпраивли адрес с битом записи и не получили ответ
			I2C_Stop();
			error = I2C_ERR_NA;	
			busy = false;
			break;
		
		case I2C_BYTE_ACK:		//шлем байт, таргет подтвердил
			if(read)			//если режим чтения, формируем повт.старт
			{
			 I2C_Start();
			}
			else if(i_byte < size)
			{
				TWDR = data[i_byte++];
				I2C_Continue();
			}
			else
			{
				 I2C_Stop();
				 busy = false;
				 error = I2C_ERR_NO;
			}
			break;
		
		case I2C_BYTE_NACK:			//нет подтверждения посланного байта
			I2C_Stop();
			error = I2C_ERR_NK;
			busy = false;
			break;
		
		case I2C_COLLISION:
			I2C_Stop();
			error = I2C_ERR_UN;
			busy = false;
			break;
		
		case I2C_SLA_R_ACK:
			if(size > 1)
			{
				I2C_Ack();
			}
			else I2C_Continue();
			break;
		
		case I2C_SLA_R_NACK:
			I2C_Stop();
			error = I2C_ERR_NK;
			busy = false;
			break;
		
		case I2C_RECEIVE_BYTE:
			data[i_byte++] = TWDR;
			if(i_byte < (size - 1))
			{
				I2C_Ack();
			}
			else I2C_Continue();
			break;
		
		case I2C_RECEIVE_BYTE_NACK:
			data[i_byte++] = TWDR;
			I2C_Stop();
			error = I2C_ERR_NO;
			busy = false;
			break;
		
		default:
			error = I2C_ERR_UN;
			busy = false;
	}
}
