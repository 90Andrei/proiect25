/*
 * GYRO.c
 *
 *  Created on: Dec 8, 2022
 *      Author: Andrei
 */
#include <stdio.h>
#include <stdint.h>
#include "spi.h"
#include <stdbool.h>
#include <string.h>

static int8_t _writeData(uint8_t regAddress, uint8_t registerValue);
static int8_t _readData(uint8_t regAddress, uint8_t *registerValue);
static void _readValuesXYZ(int16_t *x, int16_t *y, int16_t *z);

extern SPI_HandleTypeDef hspi1;
/*
 * this variable is default on "false" and becomes true when an EXTI interrupt starts for the gyroscope,and after we calculate gyro
 * values  becomes false again till another EXTI interrupt starts
 */
bool is_GYRO_EXTI_Enabled_g;

#define GYRO_DEVID               0xF
#define GYRO_SPI_ReadOp          (1 << 7)
#define GYRO_SPI_MB              (1 << 6)
#define GYRO_DATAX0              0x28
#define GYRO_CTRL_REG1           0x20
#define GYRO_POWERMODENORMAL     0xF
#define GYRO345_SCALE_FACTOR     9   //scale factor is 0,009 but we multiply with 1000 to save the value into an integer type , and process it after
#define GYRO_CTRLREG3            0x22
#define DATA_SIZE                7
#define GYRO_DEVID_VALUE         0xD3
#define BOOTUP_TIME              100
#define POWER_ENABLE_TIME        100
#define POWER_NORMAL_MODE_TIME   250
#define SET_BIT3                 0x8
/*
 * in this  variable we save the SPI configuration we need to send to the gyroscope so that
 * the gyroscope will return the gyro raw values into the RxBuffer
 */
static uint8_t TxBuffer_s[DATA_SIZE] = {0};
static uint8_t RxBuffer_s[DATA_SIZE] = {0};

void GYRO_Init()
{
    uint8_t devId;
    int16_t x, y, z;

    //reset GYRO to make sure it is on default configuration
    HAL_GPIO_TogglePin(ADXL_RESET_GPIO_Port, ADXL_RESET_Pin);
    HAL_Delay(BOOTUP_TIME);
    HAL_GPIO_TogglePin(ADXL_RESET_GPIO_Port, ADXL_RESET_Pin);

   //stop GYRO power mode
    _writeData(GYRO_CTRL_REG1, SET_BIT3);
    HAL_Delay(POWER_ENABLE_TIME);

    //Check SPI communication for gyro
    _readData(GYRO_DEVID, &devId);
    if(devId != GYRO_DEVID_VALUE)
    {
        Error_Handler();
    }

    //enable pin2 DRDY interrupt
    _writeData(GYRO_CTRLREG3, SET_BIT3);   //set bit D3(I2_DRDY) to activate data ready interrupt

   //start GYRO power mode-normal
    _writeData(GYRO_CTRL_REG1, GYRO_POWERMODENORMAL);
    HAL_Delay(POWER_NORMAL_MODE_TIME);

   // make one dummy readvalueszyz to clear first exti interrupt because  it starts too early
    _readValuesXYZ(&x, &y, &z);

}

bool GYRO_StartMeasurement()
{
    memset(TxBuffer_s, 0, DATA_SIZE * sizeof(uint8_t));
    TxBuffer_s[0] = GYRO_DATAX0 | GYRO_SPI_MB | GYRO_SPI_ReadOp;
    if(HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_READY)
    {
        is_GYRO_EXTI_Enabled_g = true;
        HAL_GPIO_WritePin(SPI_CS_GYRO_GPIO_Port, SPI_CS_GYRO_Pin, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive_DMA(&hspi1, TxBuffer_s, RxBuffer_s, DATA_SIZE);
        return true;
    }
    return false;
}

void GYRO_GetValues(int16_t *x, int16_t *y, int16_t *z)
{
    *x = (int16_t)(RxBuffer_s[2] << 8 | RxBuffer_s[1]);
    *y = (int16_t)(RxBuffer_s[4] << 8 | RxBuffer_s[3]);
    *z = (int16_t)(RxBuffer_s[6] << 8 | RxBuffer_s[5]);
    *x = (*x) * GYRO345_SCALE_FACTOR;
    *y = (*y) * GYRO345_SCALE_FACTOR;
    *z = (*z) * GYRO345_SCALE_FACTOR;
}

/**
 * @brief we use this function to write a certain value into one specific register
 *
 * @param regAddress: the register adress where we wanna write
 * @param registerValue: the value we wanna write at the register adrees
 * @return the number of bytes written , a negative value is an error occurred
 */
static int8_t _writeData(uint8_t regAddress, uint8_t registerValue)
{
	bool rc = 2;
	uint8_t TxBuffer[2];

	TxBuffer[0] = regAddress;
	TxBuffer[1] = registerValue;

	HAL_GPIO_WritePin(SPI_CS_GYRO_GPIO_Port, SPI_CS_GYRO_Pin, GPIO_PIN_RESET);
	if (HAL_SPI_Transmit(&hspi1, TxBuffer, 2, HAL_MAX_DELAY) != HAL_OK)
	{
		rc = -1;
	}
	HAL_GPIO_WritePin(SPI_CS_GYRO_GPIO_Port, SPI_CS_GYRO_Pin, GPIO_PIN_SET);

	return rc;
}

/**
 * @brief we use this function to read a certain value from one specific register
 *
 * @param regAddress: the register adress from where we want to read
 * @param registerValue: the value we  want to read at the register adrees
 * @return the number of bytes read,a negative value is an error occurred
 */
static int8_t _readData(uint8_t regAddress, uint8_t *registerValue)
{
	uint8_t TxBuffer[2];
	uint8_t RxBuffer[2];

    TxBuffer[0] = GYRO_SPI_ReadOp | regAddress;
	TxBuffer[1] = 0xFF;

	HAL_GPIO_WritePin(SPI_CS_GYRO_GPIO_Port, SPI_CS_GYRO_Pin, GPIO_PIN_RESET);
	if (HAL_SPI_TransmitReceive(&hspi1, TxBuffer, RxBuffer, 2, HAL_MAX_DELAY) != HAL_OK)
	{
		HAL_GPIO_WritePin(SPI_CS_GYRO_GPIO_Port, SPI_CS_GYRO_Pin, GPIO_PIN_SET);
		return -1;
    }
	HAL_GPIO_WritePin(SPI_CS_GYRO_GPIO_Port, SPI_CS_GYRO_Pin, GPIO_PIN_SET);
	*registerValue = RxBuffer[1];

	return 2;
}

/**
 * @brief we use the function to read the gyro raw values
 *
 * @param x raw value on Xaxis
 * @param y raw value on Yaxis
 * @param z raw value on Zaxis
 */
static void _readValuesXYZ(int16_t *x, int16_t *y, int16_t *z)
{
	uint8_t TxBuffer[7] = {0};
	uint8_t RxBuffer[7]= {0};

	TxBuffer[0] = GYRO_DATAX0 | GYRO_SPI_MB | GYRO_SPI_ReadOp;
	HAL_GPIO_WritePin(SPI_CS_GYRO_GPIO_Port, SPI_CS_GYRO_Pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi1, TxBuffer, RxBuffer, DATA_SIZE, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(SPI_CS_GYRO_GPIO_Port, SPI_CS_GYRO_Pin, GPIO_PIN_SET);

	*x = (int16_t)(RxBuffer[2] << 8 | RxBuffer[1]);
	*y = (int16_t)(RxBuffer[4] << 8 | RxBuffer[3]);
	*z = (int16_t)(RxBuffer[6] << 8 | RxBuffer[5]);
}



