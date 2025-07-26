/*
 * i2c_master.h
 *
 *  Created on: Jul 26, 2025
 *      Author: Pc
 */

#ifndef INC_I2C_MASTER_H_
#define INC_I2C_MASTER_H_

HAL_StatusTypeDef i2c_write(uint8_t address, const uint8_t* data, size_t length,
	GPIO_TypeDef* SDA_port, uint16_t SDA_pin,
	GPIO_TypeDef* SCL_port, uint16_t SCL_pin);

HAL_StatusTypeDef i2c_read(uint8_t address, uint8_t* data_buff, size_t length,
	GPIO_TypeDef* SDA_port, uint16_t SDA_pin,
	GPIO_TypeDef* SCL_port, uint16_t SCL_pin);

#endif /* INC_I2C_MASTER_H_ */
