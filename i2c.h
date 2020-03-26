/*
 * i2c.h
 *
 * Created: 21.09.2019 23:40:04
 *  Author: Дмитрий
 */ 

#ifndef I2C_H_
#define I2C_H_

enum
{
	I2C_ERR_NO = 0,		//нет ошибок
	I2C_ERR_NA,			//нет ответа от подчиненного
	I2C_ERR_NK,			//получен NACK
	I2C_ERR_BF,			//ошибка шины i2c
	I2C_ERR_UN,			//неисзветная ошибка
	I2C_ERR_DEF = 0xff	//статус неизвестен 
};

void I2C_TargetSet(uint8_t target);
uint8_t I2C_WriteByAdr(uint8_t adr, uint8_t* data, uint8_t size);
uint8_t I2C_ReadByAdr(uint8_t adr, uint8_t* data, uint8_t size);
uint8_t I2C_ReadCurrent(uint8_t* data, uint8_t size);
uint8_t I2C_WriteCurrent(uint8_t* data, uint8_t size);

#endif /* I2C_H_ */