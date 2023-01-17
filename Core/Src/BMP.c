/*
 * BMP.c
 *
 *  Created on: Jan 4, 2023
 *      Author: Andrei
 */

#include <stdint.h>
#include "i2c.h"
#include <stdbool.h>
#include "main.h"
#include <stdio.h>
#include "BMP.h"
#include "tim.h"

//read and write defines

#define BMP_ADDR_WRITE 0xEE
#define BMP_ADDR_READ  0xEF

//read calibration data registers

#define AC1MSB 0xAA
#define AC1LSB 0xAB
#define AC2MSB 0xAC
#define AC2LSB 0xAD
#define AC3MSB 0xAE
#define AC3LSB 0xAF
#define AC4MSB 0xB0
#define AC4LSB 0xB1
#define AC5MSB 0xB2
#define AC5LSB 0xB3
#define AC6MSB 0xB4
#define AC6LSB 0xB5
#define B1MSB  0xB6
#define B1LSB  0xB7
#define B2MSB  0xB8
#define B2LSB  0xB9
#define MBMSB  0xBA
#define MBLSB  0xBB
#define MCMSB  0xBC
#define MCLSB  0xBD
#define MDMSB  0xBE
#define MDLSB  0xBF

typedef struct barometer
{
	int16_t AC1;
	int16_t AC2;
	int16_t AC3;
	uint16_t AC4;
	uint16_t AC5;
	uint16_t AC6;
	int16_t B1;
	int16_t B2;
	int16_t MB;
	int16_t MC;
	int16_t MD;
} baro;

static baro b1;
static int32_t uTemp; //uncompensated temperature
static int32_t temp;
static int32_t B5;
static int32_t uPres;
static int32_t pres;
static uint8_t tempMLsb[2];
static uint8_t presMLsb[2];
uint8_t temp_is_started = 0;
uint8_t temp_is_read = 0;
uint8_t pres_is_started = 0;
uint8_t pres_is_read = 0;
uint8_t temp_wait = 0;
uint8_t pres_wait = 0;

BMP_STATE BMP_State = BMP_State_Temp_StartMeasurement;

bool BMP_DevId(void)
{
	if (HAL_I2C_IsDeviceReady(&hi2c1, BMP_ADDR_WRITE, 5, HAL_MAX_DELAY)
			== HAL_OK)
	{
		return true;
	}
	return false;
}

bool BMP_ReadCalibration(void)
{

	uint8_t calibValues[22];
	if (HAL_I2C_Mem_Read(&hi2c1, BMP_ADDR_READ, AC1MSB, I2C_MEMADD_SIZE_8BIT,
			calibValues, 22, HAL_MAX_DELAY) != HAL_OK)
	{
		return false;
	}

	b1.AC1 = ((int16_t) calibValues[0] << 8) | calibValues[1];
	b1.AC2 = ((int16_t) calibValues[2] << 8) | calibValues[3];
	b1.AC3 = ((int16_t) calibValues[4] << 8) | calibValues[5];
	b1.AC4 = ((uint16_t) calibValues[6] << 8) | calibValues[7];
	b1.AC5 = ((uint16_t) calibValues[8] << 8) | calibValues[9];
	b1.AC6 = ((uint16_t) calibValues[10] << 8) | calibValues[11];
	b1.B1 = ((int16_t) calibValues[12] << 8) | calibValues[13];
	b1.B2 = ((int16_t) calibValues[14] << 8) | calibValues[15];
	b1.MB = ((int16_t) calibValues[16] << 8) | calibValues[17];
	b1.MC = ((int16_t) calibValues[18] << 8) | calibValues[19];
	b1.MD = ((int16_t) calibValues[20] << 8) | calibValues[21];

	return true;
}

bool BMP_Init(void)
{
	if (BMP_DevId() != true)
	{
		return false;
	}
	BMP_ReadCalibration();
	return true;
}

// read uncompensated temperature

void BMP_ReadUtemp(void)
{
	uint8_t tempBuffer = 0x2E;
	HAL_I2C_Mem_Write_IT(&hi2c1, BMP_ADDR_WRITE, 0xF4, I2C_MEMADD_SIZE_8BIT,
			&tempBuffer, 1);
	HAL_Delay(5);
	HAL_I2C_Mem_Read_IT(&hi2c1, BMP_ADDR_READ, 0xF6, I2C_MEMADD_SIZE_8BIT,
			tempMLsb, 2);
}

//calculate temperature

void BMP_CalcTemp(void)
{
	int32_t x1;
	int32_t x2;

	uTemp = (tempMLsb[0] << 8) | tempMLsb[1];  // am sters cast la templsb
	x1 = (uTemp - b1.AC6) * b1.AC5 / 32768;
	x2 = ((int32_t) (b1.MC * 2048)) / ((int32_t) (x1 + b1.MD));
	B5 = x1 + x2;
	temp = (B5 + 8) / 16;
}

int32_t BMP_GetTemp(void)
{
	BMP_CalcTemp();
	return temp;
}

//read uncompensated pressure

void BMP_ReadUpres(void)
{
	uint8_t writePres = 0x34;

	HAL_I2C_Mem_Write_IT(&hi2c1, BMP_ADDR_WRITE, 0xF4, I2C_MEMADD_SIZE_8BIT,
			&writePres, 1);
	HAL_Delay(5);
	HAL_I2C_Mem_Read_IT(&hi2c1, BMP_ADDR_READ, 0xF6, I2C_MEMADD_SIZE_8BIT,
			presMLsb, 2);

}

//calculate true pressure

void BMP_CalcPres(void)
{
	int32_t x1, x2, x3;
	int32_t B6;
	int32_t B3;
	uint32_t B4;
	uint32_t B7;

	uPres = ((presMLsb[0] << 16U) | (presMLsb[1] << 8U)) >> 8;
	B6 = B5 - 4000;
	x1 = ((int32_t) b1.B2 * ((B6 * B6) / 4096)) / 2048;
	x2 = ((int32_t) b1.AC2 * B6) / 2048;
	x3 = x1 + x2;
	B3 = ((int32_t) b1.AC1 * 4 + x3 + 2) / 4;
	x1 = ((int32_t) b1.AC3 * B6) / 8192;
	x2 = ((int32_t) b1.B1 * ((B6 * B6) / 4096)) / 65536;
	x3 = (x1 + x2 + 2) / 4;
	B4 = ((uint32_t) b1.AC4 * ((uint32_t) (x3 + 32768))) / 32768;
	B7 = ((uint32_t) uPres - (uint32_t) B3) * 50000;
	if (B7 < 0x80000000)
	{
		pres = ((B7 * 2) / B4);  //pos sa trb cast?
	}
	else
	{
		pres = (B7 / B4) * 2;
	}
	x1 = (pres / 256) * (pres / 256);
	x1 = (x1 * 3038) / 65536;
	x2 = (-7357 * pres) / 65536;
	pres = pres + (x1 + x2 + 3791) / 16;

}
int32_t BMP_GetPres(void)
{
	BMP_CalcPres();
	return pres;
}

void BMP_Start_IT_Temp(void)
{
	BMP_ReadUtemp();
}

void BMP_Start_IT_Pres(void)
{
	BMP_ReadUpres();
}

int32_t BMP_State_handler()
{
	uint8_t tempBuffer = 0x2E;
	uint8_t writePres = 0x34;
	uint8_t ok = 0;

	switch (BMP_State)
	{
	case BMP_State_Temp_StartMeasurement:
		if (temp_is_started == 0)
		{
			temp_is_started = 1;
			HAL_I2C_Mem_Write_IT(&hi2c1, BMP_ADDR_WRITE, 0xF4,I2C_MEMADD_SIZE_8BIT, &tempBuffer, 1);
		//	HAL_Delay(5);

		}
		// fa mem write it complete callback si treci cand intra acolo la urm state
		break;
	case BMP_State_Wait_TempMeasurement:
		if(temp_wait == 0)
		{
			temp_wait = 1;
		//	HAL_GPIO_WritePin(Timertest_GPIO_Port, Timertest_Pin, SET);
	   	    HAL_TIM_Base_Start_IT(&htim17);
		//    HAL_Delay(5);
        //    BMP_State = BMP_State_Temp_ReadResult;
		}
		break;
	case BMP_State_Temp_ReadResult:
		if (temp_is_read == 0)
		{
			temp_is_read = 1;
			HAL_I2C_Mem_Read_IT(&hi2c1, BMP_ADDR_READ, 0xF6,I2C_MEMADD_SIZE_8BIT, tempMLsb, 2);

			//trecem la urm state cand intra in callback
		}
		break;
	case BMP_State_Temp_Measure:
		BMP_GetTemp();
		BMP_State = BMP_State_Pressure_StartMeasurement;
		break;
	case BMP_State_Pressure_StartMeasurement:
		if (pres_is_started == 0)
		{
			pres_is_started = 1;
			HAL_I2C_Mem_Write_IT(&hi2c1, BMP_ADDR_WRITE, 0xF4,I2C_MEMADD_SIZE_8BIT, &writePres, 1);
		}
		// fa mem write it complete callback si treci cand intra acolo la urm state
		break;
	case BMP_State_Wait_PresMeasurement:
		if(pres_wait == 0)
		{
			pres_wait = 1;
			HAL_TIM_Base_Start_IT(&htim17);
		//	HAL_Delay(5);
		//	BMP_State = BMP_State_Pressure_ReadResult;
        }

        break;
	case BMP_State_Pressure_ReadResult:
		if (pres_is_read == 0)
		{
			pres_is_read = 1;
			HAL_I2C_Mem_Read_IT(&hi2c1, BMP_ADDR_READ, 0xF6,I2C_MEMADD_SIZE_8BIT, presMLsb, 2);

		}
		//trecem la urm state cand intra in callback
		break;
	case BMP_State_Pressure_Measure:
		BMP_State = BMP_State_Temp_StartMeasurement;
		temp_is_started = 0;
		temp_is_read = 0;
		pres_is_started = 0;
		pres_is_read = 0;
		temp_wait = 0;
		pres_wait = 0;
		ok = 1;
		break;
	}

	return ok;
}
