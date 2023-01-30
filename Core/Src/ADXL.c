/*
 * ADXL.c
 *
 *  Created on: Dec 6, 2022
 *      Author: Andrei
 */
#include <stdio.h>
#include <stdint.h>
#include <ADXL.h>
#include "spi.h"
#include <stdbool.h>
#include <errno.h>
#include <string.h>

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_rx;

#define ADXL_SPI_READOP             (1 << 7)
#define ADXL_SPI_MB                 (1 << 6)
#define ADXL_INT_ENABLE_REG         0x2E
#define ADXL_INT_MAP_REG            0x2F
#define ADXL_DEVICEID               0x00
#define ADXL_DATAX0                 0x32
#define ADXL_POWER_CTL              0x2D
#define ADXL345_SCALE_FACTOR        39  // if accelerator range is between -2G and 2G we do nothing ,range between -4G and 4G multiply by 2,
                                        //      -8G and 8G multiply by 4,-16G and 16g multiply by 8
#define DATA_SIZE                   7
#define SET_BIT2                    0x4
#define SET_BIT3                    0x8
#define SET_BIT7                    0x80
#define CLEAR_BITS                  0x0
#define ADXL_Devid_Value            0xE5

static int8_t _readData(uint8_t regAddress, uint8_t *registerValue);
static void _set_Measurebit(void);
static void _stop_Measurebit(void);
static void _activate_It(void);
static void _readValuesXYZ(int16_t *x, int16_t *y, int16_t *z);

/*
 * this variable is default on "false" and becomes true when an EXTI interrupt starts for the accelemoter,and after we calculate acc
 * values  becomes false again till another EXTI interrupt starts
 */
bool is_ACC_EXTI_Enabled_g;
/*
 * in this  variable we save the SPI configuration we need to send to the accelometer so that
 * the accelometer will return the accleration raw values into the RxBuffer
 */
static uint8_t TxBuffer_s[DATA_SIZE] = {0};
static uint8_t RxBuffer_s[DATA_SIZE] = {0};

void ADXL_Init(void)
{
    uint8_t devId;
    int16_t x, y, z;

    //we reset the ADXL to make sure is on default configuration
    HAL_GPIO_TogglePin(ADXL_RESET_GPIO_Port, ADXL_RESET_Pin);
    HAL_Delay(100);
    HAL_GPIO_TogglePin(ADXL_RESET_GPIO_Port, ADXL_RESET_Pin);

    // Enter low power mode
    _stop_Measurebit();

    // Check SPI Communication
    _readData(ADXL_DEVICEID, &devId);
    if (devId != ADXL_Devid_Value)
    {
        Error_Handler();
    }
    _activate_It();

    // Enable ADXL measurement
    _set_Measurebit();

    //make dummy read to clear first EXTI interrupt because it starts to early
    _readValuesXYZ(&x, &y, &z);
}

void ADXL_GetValues(int16_t *x, int16_t *y, int16_t *z)
{
    *x = (int16_t)(RxBuffer_s[2] << 8 | RxBuffer_s[1]);
    *y = (int16_t)(RxBuffer_s[4] << 8 | RxBuffer_s[3]);
    *z = (int16_t)(RxBuffer_s[6] << 8 | RxBuffer_s[5]);
    *x = (*x) * ADXL345_SCALE_FACTOR;
    *y = (*y) * ADXL345_SCALE_FACTOR;
    *z = (*z) * ADXL345_SCALE_FACTOR;
}

bool ADXL_StartMeasurement(void)
{
    memset(TxBuffer_s, 0, DATA_SIZE * sizeof(uint8_t));
    TxBuffer_s[0] = ADXL_DATAX0 | ADXL_SPI_MB | ADXL_SPI_READOP;

    if(HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_READY)
    {
        is_ACC_EXTI_Enabled_g = true;
        HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive_DMA(&hspi1, TxBuffer_s, RxBuffer_s, DATA_SIZE);
        return true;
    }
    else
    {
        return false;
    }
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
	uint8_t TxBuffer[2];
	uint8_t RxBuffer[2];

	TxBuffer[0] = regAddress;
	TxBuffer[1] = registerValue;

	HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_RESET);
	if (HAL_SPI_TransmitReceive(&hspi1, TxBuffer, RxBuffer, 2, HAL_MAX_DELAY) != HAL_OK)
	{
		HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_SET);
        return -1;
	}
	HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_SET);

	return 2;
}

/**
 * @brief we use this function to read a certain value from one specific register
 *
 * @param regAddress: the register adress from where we want to read
 * @param registerValue: the value we  want to read at the register adrees
 * @return the number of bytes read, a negative value is an error occurred
 */
static int8_t _readData(uint8_t regAddress, uint8_t *registerValue)
{
	uint8_t TxBuffer[2];
	uint8_t RxBuffer[2];

	TxBuffer[0] = ADXL_SPI_READOP | regAddress;
	TxBuffer[1] = 0;

	HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_RESET);
	if (HAL_SPI_TransmitReceive(&hspi1, TxBuffer, RxBuffer, 2, HAL_MAX_DELAY) != HAL_OK)
	{
		HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_SET);
		return -1;
	}
	HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_SET);
	*registerValue = RxBuffer[1];

	return 2;
}
/**
 * @brief we use the function to read the acc raw values
 *
 * @param x raw value on Xaxis
 * @param y raw value on Yaxis
 * @param z raw value on Zaxis
 */
static void _readValuesXYZ(int16_t *x, int16_t *y, int16_t *z)
{
	uint8_t TxBuffer[DATA_SIZE] = {0};
	uint8_t RxBuffer[DATA_SIZE] = {0};

	TxBuffer[0] = ADXL_DATAX0 | ADXL_SPI_MB | ADXL_SPI_READOP;

	HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi1, TxBuffer, RxBuffer, DATA_SIZE, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_SET);

	*x = (int16_t)((RxBuffer[2] << 8) + RxBuffer[1]);
	*y = (int16_t)((RxBuffer[4] << 8) + RxBuffer[3]);
	*z = (int16_t)((RxBuffer[6] << 8) + RxBuffer[5]);
}
/**
 * @brief:we disable the measurement from accelometer
 *
 */
static void _stop_Measurebit(void)
{
	_writeData(ADXL_POWER_CTL, SET_BIT2);
}

/**
 * @brief we enable the measurement from accelometer
 *
 */
static void _set_Measurebit(void)
{
	_writeData(ADXL_POWER_CTL, CLEAR_BITS);
	_writeData(ADXL_POWER_CTL, SET_BIT3);
}

/**
 * @brief we enable data ready interrupt for accelometer
 *
 */
static void _activate_It(void)
{
	_writeData(ADXL_INT_MAP_REG , SET_BIT7);    //set bit d7 to activate data ready interrupt
	_writeData(ADXL_INT_ENABLE_REG, SET_BIT7);
}


