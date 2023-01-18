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

#define START_TEMP_CMD     0x2E
#define START_PRESSURE_CMD 0x34

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

typedef struct calibTable
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
} sBMP_calibTable_t;

typedef enum state
{
    BMP_State_Temp_StartMeasurement,
    BMP_State_Temp_StartDelay,
    BMP_State_Temp_ReadResult,
    BMP_State_Pressure_StartMeasurement,
    BMP_State_Presure_StartDelay,
    BMP_State_Pressure_ReadResult,
    BMP_State_CalcResult,
}sBMP_state_t;

static bool _devId(void);
static bool _readCalibration(void);
static void _calcTemp(int32_t *temperature, int32_t *pressure);

static bool stateUpdate_s;
static sBMP_state_t state_s;
static sBMP_calibTable_t b1;

static uint8_t tempBuff[2];
static uint8_t presBuff[2];

static int32_t temperature_s, pressure_s;

bool BMP_Init(void)
{
    if (_devId() != true)
    {
        return false;
    }

    _readCalibration();

    state_s = BMP_State_Temp_StartMeasurement;
    stateUpdate_s = true;
    return true;
}

void BMP_UpdateState(void)
{
    stateUpdate_s = true;
}

int32_t BMP_GetPresure(void)
{
    return pressure_s;
}

int32_t BMP_GetTemperature(void)
{
    return temperature_s;
}

bool BMP_CyclicTask()
{
    static uint8_t txByte;  //static because data is being sent later in ISR

    if(!stateUpdate_s)
    {
        return 0;
    }

    stateUpdate_s = false;

    switch (state_s)
    {
    case BMP_State_Temp_StartMeasurement:
        txByte = START_TEMP_CMD;
        HAL_I2C_Mem_Write_IT(&hi2c1, BMP_ADDR_WRITE, 0xF4, I2C_MEMADD_SIZE_8BIT, &txByte, 1);
        state_s = BMP_State_Temp_StartDelay;
        break;
    case BMP_State_Temp_StartDelay:
        HAL_GPIO_WritePin(Timertest_GPIO_Port, Timertest_Pin, GPIO_PIN_SET); //for testing purpose
        HAL_TIM_Base_Start_IT(&htim17);
        state_s = BMP_State_Temp_ReadResult;
        break;
    case BMP_State_Temp_ReadResult:
        HAL_I2C_Mem_Read_IT(&hi2c1, BMP_ADDR_READ, 0xF6, I2C_MEMADD_SIZE_8BIT, tempBuff, 2);
        state_s = BMP_State_Pressure_StartMeasurement;
        break;
    case BMP_State_Pressure_StartMeasurement:
        txByte = START_PRESSURE_CMD;
        HAL_I2C_Mem_Write_IT(&hi2c1, BMP_ADDR_WRITE, 0xF4, I2C_MEMADD_SIZE_8BIT, &txByte, 1);
        state_s = BMP_State_Presure_StartDelay;
        break;
    case BMP_State_Presure_StartDelay:
        HAL_GPIO_WritePin(Timertest_GPIO_Port, Timertest_Pin, GPIO_PIN_SET);
        HAL_TIM_Base_Start_IT(&htim17);
        state_s = BMP_State_Pressure_ReadResult;
        break;
    case BMP_State_Pressure_ReadResult:
        HAL_I2C_Mem_Read_IT(&hi2c1, BMP_ADDR_READ, 0xF6, I2C_MEMADD_SIZE_8BIT, presBuff, 2);
        state_s = BMP_State_CalcResult;
        break;
    case BMP_State_CalcResult:
        _calcTemp(&temperature_s, &pressure_s);
        state_s = BMP_State_Temp_StartMeasurement;
        return 1;
    default:
        break;
    }

    return 0;
}

static bool _devId(void)
{
    if (HAL_I2C_IsDeviceReady(&hi2c1, BMP_ADDR_WRITE, 5, HAL_MAX_DELAY) == HAL_OK)
    {
        return true;
    }

    return false;
}

static bool _readCalibration(void)
{
    uint8_t calibValues[22];
    if (HAL_I2C_Mem_Read(&hi2c1, BMP_ADDR_READ, AC1MSB, I2C_MEMADD_SIZE_8BIT, calibValues, 22, HAL_MAX_DELAY) != HAL_OK)
    {
        return false;
    }

    //TODO: REZOLVA WARNING
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

// read uncompensated temperature

//calculate temperature

static void _calcTemp(int32_t *temperature, int32_t *pressure)
{
    int32_t uTemp, uPres; //uncompensated temperature, pressure
    int32_t x1, x2, x3;
    int32_t B3, B5, B6;
    uint32_t B4, B7;

    uTemp = (tempBuff[0] << 8) | tempBuff[1];
    x1 = (uTemp - b1.AC6) * b1.AC5 / 32768;
    x2 = ((int32_t) (b1.MC * 2048)) / ((int32_t) (x1 + b1.MD));
    B5 = x1 + x2;

    *temperature = ((B5 + 8) / 16);

    uPres = (presBuff[0] << 8) | presBuff[1];
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
        uPres = ((B7 * 2) / B4);  //pos sa trb cast?
    }
    else
    {
        uPres = (B7 / B4) * 2;
    }
    x1 = (uPres / 256) * (uPres / 256);
    x1 = (x1 * 3038) / 65536;
    x2 = (-7357 * uPres) / 65536;

    *pressure = uPres + (x1 + x2 + 3791) / 16;
}
