/*
 * HMC.c
 *
 *  Created on: Dec 29, 2022
 *      Author: Andrei
 */

#include "HMC.h"
#include <stdint.h>
#include "i2c.h"
#include <stdbool.h>
#include <stdio.h>

#define HMC_ADDR_READ   0x3D  //if we read with I2C on this device we use this value
#define HMC_ADDR_WRITE  0x3C  //if we write with I2C on this device we use this value
#define HMC_DATAXO      0x3
#define DATA_SIZE        6
#define IDA_VALUE       0x48
/*
 * in this variable we save compass raw values
 */
static uint8_t databuffer_s[DATA_SIZE] = {0};

extern I2C_HandleTypeDef hi2c1;

bool HMC_Init()
{
    uint8_t idA = 0xA;
    uint8_t idle = 0x3;

    if (HAL_I2C_IsDeviceReady(&hi2c1, HMC_ADDR_WRITE, 3, HAL_MAX_DELAY) != HAL_OK)
    {
        return false;
    }

    if (HAL_I2C_Mem_Read(&hi2c1, HMC_ADDR_WRITE, idA, I2C_MEMADD_SIZE_8BIT, &idA, 1, HAL_MAX_DELAY) != HAL_OK)
    {
        return false;
    }

    if (idA != IDA_VALUE)
    {
        return false;
    }

    HAL_I2C_Mem_Write(&hi2c1, HMC_ADDR_WRITE, 0x2, I2C_MEMADD_SIZE_8BIT, &idle, 1, HAL_MAX_DELAY); //we put the sensor in idle state

    if (HAL_I2C_Mem_Read(&hi2c1, HMC_ADDR_WRITE, HMC_DATAXO, I2C_MEMADD_SIZE_8BIT, databuffer_s, DATA_SIZE, HAL_MAX_DELAY)
            != HAL_OK)
    {
        return false;   //we make a dummy read to clear first Exti interrupt
    }
    HMC_SetSingleMeasurentMode();

    return true;
}

bool HMC_SetSingleMeasurentMode()
{
    uint8_t single = 0x1; //switch to 1 to be in sigle measurement mode, if I set it to 0 it is in continuous measure mode

    if (HAL_I2C_Mem_Write(&hi2c1, HMC_ADDR_WRITE, 0x2, I2C_MEMADD_SIZE_8BIT, &single, 1, HAL_MAX_DELAY) != HAL_OK)
    {
        return false;
    }

    return true;
}

int8_t HMC_ReadValues()
{
    if (HAL_I2C_Mem_Read_DMA(&hi2c1, HMC_ADDR_WRITE, HMC_DATAXO, I2C_MEMADD_SIZE_8BIT, databuffer_s, DATA_SIZE) != HAL_OK)
    {
        return -1;
    }

    return DATA_SIZE;
}

void HMC_GetValues(int16_t *x, int16_t *y, int16_t *z)
{
    *x = (int16_t)(databuffer_s[0] << 8 | databuffer_s[1]);
    *y = (int16_t)(databuffer_s[2] << 8 | databuffer_s[3]);
    *z = (int16_t)(databuffer_s[4] << 8 | databuffer_s[5]);

    *x = *x * 92;      // to get the correct values for x ,y, z
    *y = *y * 92;      // we have to multiply values with 0.92(we use 92 to not use float type ,we process the value when we need it)
    *z = *z * 92; // we use 0.92 for sensor field range +- 1.3GA ,for other sensor field range we use different scale pag 13 datasheet
}

