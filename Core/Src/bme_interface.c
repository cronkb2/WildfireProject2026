//BME688 I2C Device Address
//------------------------------------------------------------------------------
//The sensor uses a 7-bit address of 0x76.
//STM32 HAL requires the address to be left-shifted by 1 bit.
//This file acts as the communication bridge between the Bosch BME688 API and STM32 HAL drivers
//The Bosch API is hardware-independent,
//meaning it does not know which MCU is being used or which I2C peripheral is connected
//------------------------------------------------------------------------------
#include "bme_interface.h"


#define BME688_I2C_ADDR  (0x76 << 1)


int8_t bme68x_i2c_read(uint8_t reg_addr,uint8_t *data,uint32_t len,void *intf_ptr){
	//Read data from the specified sensor register
	HAL_I2C_Mem_Read(&hi2c1,BME688_I2C_ADDR,reg_addr,I2C_MEMADD_SIZE_8BIT,data,len,HAL_MAX_DELAY);
	return 0;
}


int8_t bme68x_i2c_write(uint8_t reg_addr,const uint8_t *data,uint32_t len,void *intf_ptr){
	//Write configuration or command data to sensor register
	HAL_I2C_Mem_Write(&hi2c1,BME688_I2C_ADDR,reg_addr,I2C_MEMADD_SIZE_8BIT,(uint8_t*)data,len,HAL_MAX_DELAY);
	return 0;
}
//Delay required by Bosch API operates in ms
void bme68x_delay_us(uint32_t period, void *intf_ptr){
	HAL_Delay(period / 1000);
}
