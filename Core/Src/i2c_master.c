/*
 * i2c_master.c
 *
 *  Created on: Jul 26, 2025
 *      Author: Pc
 */

#include "stm32f1xx_hal.h"

extern TIM_HandleTypeDef htim1;

static inline void clk_delay() {
	__HAL_TIM_SET_COUNTER(&htim1, 0);

	// since each tick in TIM1 is 1 us, this holds SCL for a half period to get 100kbps
	while (__HAL_TIM_GET_COUNTER(&htim1) < 5);
}

static inline void write_1(GPIO_TypeDef* port, uint16_t pin) {
	HAL_GPIO_WritePin(port, SDA_pin, GPIO_PIN_SET);
}
static inline void write_0(GPIO_TypeDef* port, uint16_t pin) {
	HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
}

static inline HAL_StatusTypeDef check_ACK(GPIO_TypeDef* SDA_port, uint16_t SDA_pin,
	GPIO_TypeDef* SCL_port, uint16_t SCL_pin)
{
	write_1(SDA_port, SDA_pin);
	write_1(SCL_port, SCL_pin);
	clk_delay();

	GPIO_PinState pin_status = HAL_GPIO_ReadPin(SDA_port, SDA_pin);

	write_0(SCL_port, SCL_pin);
	clk_delay();

	if (pin_status != GPIO_PIN_SET) {
		return HAL_ERROR;
	}

	return HAL_OK;
}

static inline void send_ACK(GPIO_TypeDef* SDA_port, uint16_t SDA_pin,
	GPIO_TypeDef* SCL_port, uint16_t SCL_pin)
{
	write_0(SDA_port, SDA_pin);
	write_1(SCL_port, SCL_pin);

	clk_delay();

	write_0(SCL_port, SCL_pin);
	write_1(SDA_port, SDA_pin);
	clk_delay();
}

static inline void send_NACK(GPIO_TypeDef* SDA_port, uint16_t SDA_pin,
	GPIO_TypeDef* SCL_port, uint16_t SCL_pin)
{
	write_1(SDA_port, SDA_pin);
	write_1(SCL_port, SCL_pin);

	clk_delay();

	write_0(SCL_port, SCL_pin);
	clk_delay();
}

static inline void write_byte(uint8_t byte, GPIO_TypeDef* SDA_port, uint16_t SDA_pin,
	GPIO_TypeDef* SCL_port, uint16_t SCL_pin)
{
	for (int i = 0; i < 8; ++i) {
		if (byte & 0x80) {
			write_1(SDA_port, SDA_pin);
		}
		else {
			write_0(SDA_port, SDA_pin);
		}

		write_1(SCL_port, SCL_pin);
		clk_delay();
		write_0(SCL_port, SCL_pin);
		clk_delay();

		byte <<= 1;
	}
}

static inline uint8_t read_byte(GPIO_TypeDef* SDA_port, uint16_t SDA_pin,
	GPIO_TypeDef* SCL_port, uint16_t SCL_pin)
{
	uint8_t byte = 0x00;

	write_1(SDA_port, SDA_pin);

	for (int i = 0; i < 8; ++i) {
		byte <<= 1;

		write_1(SCL_port, SCL_pin);
		clk_delay();

		if (HAL_GPIO_ReadPin(SDA_port, SDA_pin) == GPIO_PIN_SET) {
			byte |= 0x01;
		}

		write_0(SCL_port, SCL_pin);
		clk_delay();
	}

	return byte;
}

// addr_and_rw is a byte that holds 7-bit address and read/write bit
static inline void start_i2c(uint8_t addr_and_rw, GPIO_TypeDef* SDA_port, uint16_t SDA_pin,
	GPIO_TypeDef* SCL_port, uint16_t SCL_pin)
{
	//making sure SDA and SCL are high level
	write_1(SDA_port, SDA_pin);
	write_1(SCL_port, SCL_pin);
	clk_delay();

	write_0(SDA_port, SDA_pin);
	clk_delay();
	write_0(SCL_port, SCL_pin);
	clk_delay();

	write_byte(addr_and_rw, SDA_port, SDA_pin, SCL_port, SCL_pin);
}

static inline void end_i2c(GPIO_TypeDef* SDA_port, uint16_t SDA_pin,
		GPIO_TypeDef* SCL_port, uint16_t SCL_pin) {
	write_0(SDA_port, SDA_pin);
	write_0(SCL_port, SCL_pin);

	clk_delay();
	write_1(SCL_port, SCL_pin);
	clk_delay();
	write_1(SDA_port, SDA_pin);

	clk_delay();
}

HAL_StatusTypeDef i2c_write(uint8_t address, const uint8_t* data, size_t length,
	GPIO_TypeDef* SDA_port, uint16_t SDA_pin,
	GPIO_TypeDef* SCL_port, uint16_t SCL_pin)
{
	// contains address and "0" bit for read/write
	address <<= 1;

	start_i2c(address, SDA_port, SDA_pin, SCL_port, SCL_pin);

	if (check_ACK(SDA_port, SDA_pin, SCL_port, SCL_pin) != HAL_OK) {
		end_i2c(SDA_port, SDA_pin, SCL_port, SCL_pin);
		return HAL_ERROR;
	}

	for (int i = 0; i < length; ++i) {
		write_byte(*(data + i), SDA_port, SDA_pin, SCL_port, SCL_pin);
		if (check_ACK(SDA_port, SDA_pin, SCL_port, SCL_pin) != HAL_OK) {
			end_i2c(SDA_port, SDA_pin, SCL_port, SCL_pin);
			return HAL_ERROR;
		}
	}

	end_i2c(SDA_port, SDA_pin, SCL_port, SCL_pin);

	return HAL_OK;
}

HAL_StatusTypeDef i2c_read(uint8_t address, uint8_t* data_buff, size_t length,
	GPIO_TypeDef* SDA_port, uint16_t SDA_pin,
	GPIO_TypeDef* SCL_port, uint16_t SCL_pin)
{
	address <<= 1;
	// contains address and "1" bit for read/write
	address |= 0x01;

	start_i2c(address, SDA_port, SDA_pin, SCL_port, SCL_pin);

	if (check_ACK(SDA_port, SDA_pin, SCL_port, SCL_pin) != HAL_OK) {
		end_i2c(SDA_port, SDA_pin, SCL_port, SCL_pin);
		return HAL_ERROR;
	}

	for (int i = 0; i < length; ++i) {
		*(data_buff + i) = read_byte(SDA_port, SDA_pin, SCL_port, SCL_pin);
		if (i == length - 1) {
			send_NACK(SDA_port, SDA_pin, SCL_port, SCL_pin);
			break;
		}
		send_ACK(SDA_port, SDA_pin, SCL_port, SCL_pin);
	}

	end_i2c(SDA_port, SDA_pin, SCL_port, SCL_pin);

	return HAL_OK;
}
