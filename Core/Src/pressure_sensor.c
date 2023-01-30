/*
 * BMP.c
 *
 *  Created on: Jan 4, 2023
 *      Author: Andrei
 */

#include <pressure_sensor.h>
#include <stdint.h>
#include "i2c.h"
#include <stdbool.h>
//#include "main.h"
#include <stdio.h>
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

#define DATA_SIZE 2
#define CALIB_VALUES_SIZE  22

#define WRITE_SETUP_ADDR 0xF4
#define  READ_SETUP_ADDR 0xF6

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
} spressure_sensor_calibTable_t;

typedef enum state
{
    STATE_TEMP_STARTMEASUREMENT,
    STATE_TEMP_STARTDELAY,
    STATE_TEMP_READRESULT,
    STATE_PRESSURE_STARTMEASUREMENT,
    STATE_PRESURE_STARTDELAY,
    STATE_PRESSURE_READRESULT,
    STATE_CALCRESULT,
}spressure_sensor_state_t;

static bool _devId(void);
static int8_t _readCalibration(void);
static void _calcTempAndPressure(int32_t *temperature, int32_t *pressure);

static bool stateUpdate_s;
static spressure_sensor_state_t state_s;
static spressure_sensor_calibTable_t press_calibration_value_s;

static uint8_t tempBuff[DATA_SIZE];
static uint8_t presBuff[DATA_SIZE];

static int32_t temperature_s, pressure_s;

bool Pressure_Sensor_Init(void)
{
    if (_devId() != true)
    {
        return false;
    }
    _readCalibration();
    state_s = STATE_TEMP_STARTMEASUREMENT;
    stateUpdate_s = true;
    return true;
}

void Pressure_Sensor_UpdateState(void)
{
    stateUpdate_s = true;
}

int32_t Pressure_Sensor_GetPresure(void)
{
    return pressure_s;
}

int32_t Pressure_Sensor_GetTemperature(void)
{
    return temperature_s;
}

bool Pressure_Sensor_CyclicTask()
{
    static uint8_t txByte;  //static because data is being sent later in ISR

    if(!stateUpdate_s)
    {
        return 0;
    }

    stateUpdate_s = false;

    switch (state_s)
    {
    case STATE_TEMP_STARTMEASUREMENT:
        txByte = START_TEMP_CMD;
        HAL_I2C_Mem_Write_IT(&hi2c1, Pressure_Sensor_ADDR_WRITE, WRITE_SETUP_ADDR, I2C_MEMADD_SIZE_8BIT, &txByte, 1);
        state_s = STATE_TEMP_STARTDELAY;
        break;
    case STATE_TEMP_STARTDELAY:
        HAL_TIM_Base_Start_IT(&htim17);
        state_s = STATE_TEMP_READRESULT;
        break;
    case STATE_TEMP_READRESULT:
        HAL_I2C_Mem_Read_IT(&hi2c1, Pressure_Sensor_ADDR_READ, READ_SETUP_ADDR, I2C_MEMADD_SIZE_8BIT, tempBuff, DATA_SIZE);
        state_s = STATE_PRESSURE_STARTMEASUREMENT;
        break;
    case STATE_PRESSURE_STARTMEASUREMENT:
        txByte = START_PRESSURE_CMD;
        HAL_I2C_Mem_Write_IT(&hi2c1, Pressure_Sensor_ADDR_WRITE, WRITE_SETUP_ADDR, I2C_MEMADD_SIZE_8BIT, &txByte, 1);
        state_s = STATE_PRESURE_STARTDELAY;
        break;
    case STATE_PRESURE_STARTDELAY:
        HAL_TIM_Base_Start_IT(&htim17);
        state_s = STATE_PRESSURE_READRESULT;
        break;
    case STATE_PRESSURE_READRESULT:
        HAL_I2C_Mem_Read_IT(&hi2c1, Pressure_Sensor_ADDR_READ, READ_SETUP_ADDR, I2C_MEMADD_SIZE_8BIT, presBuff, DATA_SIZE);
        state_s = STATE_CALCRESULT;
        break;
    case STATE_CALCRESULT:
        _calcTempAndPressure(&temperature_s, &pressure_s);
        state_s = STATE_TEMP_STARTMEASUREMENT;
        return 1;
    default:
        break;
    }

    return 0;
}
/**
 * @brief this function checks if the comunication with the pressure sensor is
 * ok
 *
 * @return true if communication is ok else false
 */
static bool _devId(void)
{
    if (HAL_I2C_IsDeviceReady(&hi2c1, Pressure_Sensor_ADDR_WRITE, 5, HAL_MAX_DELAY) == HAL_OK)
    {
        return true;
    }

    return false;
}
/**
 * @brief this function reads and calculate calibration values from the pressure sensor
 *
 * @return the number of bytes read, a negative value is an error occurred
 */
static int8_t _readCalibration(void)
{
    uint8_t calibValues[CALIB_VALUES_SIZE];
    if (HAL_I2C_Mem_Read(&hi2c1, Pressure_Sensor_ADDR_READ, AC1MSB, I2C_MEMADD_SIZE_8BIT, calibValues, CALIB_VALUES_SIZE, HAL_MAX_DELAY) != HAL_OK)
    {
        return -1;
    }

    press_calibration_value_s.AC1 = (int16_t)(calibValues[0] << 8 | calibValues[1]);
    press_calibration_value_s.AC2 = (int16_t)(calibValues[2] << 8 | calibValues[3]);
    press_calibration_value_s.AC3 = (int16_t)(calibValues[4] << 8 | calibValues[5]);
    press_calibration_value_s.AC4 = (uint16_t)(calibValues[6] << 8 | calibValues[7]);
    press_calibration_value_s.AC5 = (uint16_t)(calibValues[8] << 8 | calibValues[9]);
    press_calibration_value_s.AC6 = (uint16_t)(calibValues[10] << 8 | calibValues[11]);
    press_calibration_value_s.B1 = (int16_t)(calibValues[12] << 8 | calibValues[13]);
    press_calibration_value_s.B2 = (int16_t)(calibValues[14] << 8 | calibValues[15]);
    press_calibration_value_s.MB = (int16_t)(calibValues[16] << 8 | calibValues[17]);
    press_calibration_value_s.MC = (int16_t)(calibValues[18] << 8 | calibValues[19]);
    press_calibration_value_s.MD = (int16_t)(calibValues[20] << 8 | calibValues[21]);

    return CALIB_VALUES_SIZE;
}

/**
 * @brief this function calculate temperature and barometric pressure
 * all calculations are done with the  instruction found in sensor datasheet
 *
 * @param temperature
 * @param pressure
 */
static void _calcTempAndPressure(int32_t *temperature, int32_t *pressure)
{
    int32_t uTemp, uPres; //uncompensated temperature, pressure
    int32_t x1, x2, x3;
    int32_t B3, B5, B6;
    uint32_t B4, B7;

    uTemp = (tempBuff[0] << 8) | tempBuff[1];
    x1 = (uTemp - press_calibration_value_s.AC6) * press_calibration_value_s.AC5 / 32768;
    x2 = ((int32_t) (press_calibration_value_s.MC * 2048)) / ((int32_t) (x1 + press_calibration_value_s.MD));
    B5 = x1 + x2;

    *temperature = ((B5 + 8) / 16);

    uPres = (presBuff[0] << 8) | presBuff[1];
    B6 = B5 - 4000;
    x1 = ((int32_t) press_calibration_value_s.B2 * ((B6 * B6) / 4096)) / 2048;
    x2 = ((int32_t) press_calibration_value_s.AC2 * B6) / 2048;
    x3 = x1 + x2;
    B3 = ((int32_t) press_calibration_value_s.AC1 * 4 + x3 + 2) / 4;
    x1 = ((int32_t) press_calibration_value_s.AC3 * B6) / 8192;
    x2 = ((int32_t) press_calibration_value_s.B1 * ((B6 * B6) / 4096)) / 65536;
    x3 = (x1 + x2 + 2) / 4;
    B4 = ((uint32_t) press_calibration_value_s.AC4 * ((uint32_t) (x3 + 32768))) / 32768;
    B7 = ((uint32_t) uPres - (uint32_t) B3) * 50000;
    if (B7 < 0x80000000)
    {
        uPres =(int32_t)((B7 * 2) / B4);
    }
    else
    {
        uPres =(int32_t)(B7 / B4) * 2;
    }
    x1 = (uPres / 256) * (uPres / 256);
    x1 = (x1 * 3038) / 65536;
    x2 = (-7357 * uPres) / 65536;

    *pressure = uPres + (x1 + x2 + 3791) / 16;
}
