/*
 * ADXL.c
 *
 *  Created on: Dec 6, 2022
 *      Author: Andrei
 */
#include <stdio.h>
#include <stdint.h>
#include <ADXL.h>
#include "main.h"
#include <stdbool.h>
#include <errno.h>

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_rx;

#define ADXL_SPI_ReadOp             (1 << 7)
#define ADXL_SPI_MB                 (1 <<6)
#define ADXL_INT_ENABLE_REG         0x2E
#define ADXL_INT_MAP_REG            0x2F
#define ADXL_DEVICEID               0x00
#define ADXL_DATAX0                 0x32
#define ADXL_POWER_CTL              0x2D
#define ADXL345_SCALE_FACTOR        39  //  if is 2g  , if is 4g *2  8g *4 16g*8

static bool _ReadData(uint8_t regAddress, uint8_t *registerValue);
static void _Set_Measurebit(void);
static void _Stop_Measurebit(void);
static void _INIT_IT(void);
static void _readValuesXYZ(int16_t *x, int16_t *y, int16_t *z);

bool ACC_EXTI_Interrupt_g;

static uint8_t TxBuffer_s[] = { 0, 0, 0, 0, 0, 0, 0 };
static uint8_t RxBuffer_s[7];

void ADXL_Init(void)
{
    uint8_t devId;
    int16_t x, y, z;

    //reset ADXL
    HAL_GPIO_TogglePin(ADXL_RESET_GPIO_Port, ADXL_RESET_Pin);
    HAL_Delay(100);
    HAL_GPIO_TogglePin(ADXL_RESET_GPIO_Port, ADXL_RESET_Pin);

    // Enter low power mode
    _Stop_Measurebit();

    // Check SPI Communication
    _ReadData(ADXL_DEVICEID, &devId);
    if (devId != 0xE5)
    {
        Error_Handler();
    }
    _INIT_IT();

    // Start Measure
    _Set_Measurebit();

    //make dummy read to clear first EXTI interrupt
    _readValuesXYZ(&x, &y, &z);
}
void ADXL_GetValuesXYZ(int16_t *x, int16_t *y, int16_t *z)
{
    *x = (int16_t)(RxBuffer_s[2] << 8 | RxBuffer_s[1]);
    *y = (int16_t)(RxBuffer_s[4] << 8 | RxBuffer_s[3]);
    *z = (int16_t)(RxBuffer_s[6] << 8 | RxBuffer_s[5]);
    *x = (*x) * ADXL345_SCALE_FACTOR;
    *y = (*y) * ADXL345_SCALE_FACTOR;
    *z = (*z) * ADXL345_SCALE_FACTOR;
}

void ADXL_MeasureRawData_DMA(void)
{
    for(int i = 1; i < 7; i++)
    {
        TxBuffer_s[i] = 0;
    }

    TxBuffer_s[0] = ADXL_DATAX0 | ADXL_SPI_MB | ADXL_SPI_ReadOp;

    if(HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_READY)
    {
        ACC_EXTI_Interrupt_g = true;
        HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive_DMA(&hspi1, TxBuffer_s, RxBuffer_s, 7);
    }
}

static bool _WriteData(uint8_t regAddress, uint8_t registerValue)
{
	uint8_t TxBuffer[2];
	uint8_t RxBuffer[2];

	TxBuffer[0] = regAddress;
	TxBuffer[1] = registerValue;

	HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_RESET);
	if (HAL_SPI_TransmitReceive(&hspi1, TxBuffer, RxBuffer, 2, 200) != HAL_OK)
	{
		HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_SET);
        return false;
	}
	HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_SET);

	return true;
}

static bool _ReadData(uint8_t regAddress, uint8_t *registerValue)
{
	uint8_t TxBuffer[2];
	uint8_t RxBuffer[2];

	TxBuffer[0] = ADXL_SPI_ReadOp | regAddress;
	TxBuffer[1] = 0;

	HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_RESET);
	if (HAL_SPI_TransmitReceive(&hspi1, TxBuffer, RxBuffer, 2, 200) != HAL_OK)
	{
		HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_SET);
		return false;
	}
	HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_SET);
	*registerValue = RxBuffer[1];

	return true;
}

static void _readValuesXYZ(int16_t *x, int16_t *y, int16_t *z)
{
	uint8_t TxBuffer[] = { 0, 0, 0, 0, 0, 0, 0 };
	uint8_t RxBuffer[7];

	TxBuffer[0] = ADXL_DATAX0 | ADXL_SPI_MB | ADXL_SPI_ReadOp;

	HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi1, TxBuffer, RxBuffer, 7, 200);
	HAL_GPIO_WritePin(SPI_CS_ACC_GPIO_Port, SPI_CS_ACC_Pin, GPIO_PIN_SET);

	*x = (int16_t)((RxBuffer[2] << 8) + RxBuffer[1]);
	*y = (int16_t)((RxBuffer[4] << 8) + RxBuffer[3]);
	*z = (int16_t)((RxBuffer[6] << 8) + RxBuffer[5]);
}

static void _Stop_Measurebit(void)
{
	_WriteData(ADXL_POWER_CTL, 0x4);
}

static void _Set_Measurebit(void)
{
	_WriteData(ADXL_POWER_CTL, 0x0);
	_WriteData(ADXL_POWER_CTL, 0x8);
}

static void _INIT_IT(void)
{
	_WriteData(ADXL_INT_MAP_REG , 0x80);    //setam bit d7 ca sa activam intreruperea data ready pt int2
	_WriteData(ADXL_INT_ENABLE_REG, 0x80);
}


