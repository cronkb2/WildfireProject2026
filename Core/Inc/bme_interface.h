#ifndef BME_INTERFACE_H
#define BME_INTERFACE_H
//bme_interface.h
//Purpose:
//Interface between STM32 HAL drivers and the Bosch BME688 API.
//This file declares the low-level I2C communication functions
//required by the Bosch sensor library.
//Communication Protocol:
//I2C
//STM32 Nucleo-F030R8
#include "main.h"
#include "bme68x.h"

extern I2C_HandleTypeDef hi2c1;

//Function: bme68x_i2c_read
//Purpose: Reads data from a BME688 sensor register using I2C
//Inputs:
//reg_addr-> Register address to read from
//data-> Pointer to data buffer
//len-> Number of bytes to read
int8_t bme68x_i2c_read(uint8_t reg_addr,uint8_t *data,uint32_t len,void *intf_ptr);

//Function:bme68x_i2c_write
//Purpose:Writes data to a BME688 sensor register using I2C
//Inputs:
//reg_addr->Register address to write to
//data->Pointer to transmit buffer
//len->Number of bytes to transmit
int8_t bme68x_i2c_write(uint8_t reg_addr,const uint8_t *data,uint32_t len,void *intf_ptr);

//Function:bme68x-delay_us
//Purpose:Introduce delay when called
//Inputs:
//period-> Delay duration in microseconds
void bme68x_delay_us(uint32_t period, void *intf_ptr);

#endif
