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
#include "main.h"
#include <stdio.h>
#include "BMP.h"

#define HMC_ADDR_READ   0x3D  // daca vrem sa citim in i2c folosim adresa asta
#define HMC_ADDR_WRITE  0x3C  // daca vrem sa scriem in i2c folosim adresa asta
#define HMC_DATAXO      0x3
extern I2C_HandleTypeDef hi2c1;
static uint8_t databuffer[6] = {0, 0, 0, 0, 0, 0};
extern bool HMC_EXTI_Ready;
extern bool HMC_IT_Ready ;


bool HMC_Devid ()
{
	uint8_t idA = 0xA;
	uint8_t idB = 0xB;
	uint8_t idC = 0xC;

	if(HAL_I2C_IsDeviceReady(&hi2c1, HMC_ADDR_WRITE, 3, HAL_MAX_DELAY)  != HAL_OK)
	{
		return false;
	}

   if( HAL_I2C_Master_Transmit(&hi2c1, HMC_ADDR_WRITE, (uint8_t*)&idA, 1, HAL_MAX_DELAY) != HAL_OK)
   {
	   return false;
   }

   if(HAL_I2C_Master_Receive(&hi2c1, (uint16_t)HMC_ADDR_READ, &idA, 1, HAL_MAX_DELAY) != HAL_OK)
   {
	   return false;
   }

    HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)HMC_ADDR_WRITE, &idB, 1, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive(&hi2c1, (uint16_t)HMC_ADDR_READ, &idB, 1, HAL_MAX_DELAY);
    HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)HMC_ADDR_WRITE, &idC, 1, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive(&hi2c1, (uint16_t)HMC_ADDR_READ, &idC, 1, HAL_MAX_DELAY);


    if(idA != 0x48 || idB != 0x34 || idC != 0x33)  // 0x48, 0x34 , 0x33 valori de la idA,B,C
    {
    	return false;
    }

    return true;

}

bool HMC_DEVIDv2()
{
	uint8_t idA = 0xA;
    uint8_t idle = 0x3;

	if(HAL_I2C_IsDeviceReady(&hi2c1, HMC_ADDR_WRITE, 3, HAL_MAX_DELAY)  != HAL_OK)
	{
		return false;
	}

    if(HAL_I2C_Mem_Read(&hi2c1, HMC_ADDR_WRITE, idA, I2C_MEMADD_SIZE_8BIT, &idA, 1, HAL_MAX_DELAY) != HAL_OK)
    {
    	return false;
    }

    if(idA != 0x48)
    {
    	return false;
    }

    HAL_I2C_Mem_Write(&hi2c1, HMC_ADDR_WRITE, 0x2, I2C_MEMADD_SIZE_8BIT, &idle, 1, HAL_MAX_DELAY);  //punem senzorul in idle state

    if(HAL_I2C_Mem_Read(&hi2c1, HMC_ADDR_WRITE, HMC_DATAXO, I2C_MEMADD_SIZE_8BIT, databuffer, 6,HAL_MAX_DELAY) != HAL_OK)
    	{
    		return false;   //facem un dummy read
    	}
    return true;

}

bool HMC_readtest()
{
	if(HAL_I2C_IsDeviceReady(&hi2c1, HMC_ADDR_WRITE, 3, HAL_MAX_DELAY)  != HAL_OK)
		{
			return false;
		}
	return true;
}

void dummyread()
{
	HAL_I2C_Mem_Read(&hi2c1, HMC_ADDR_WRITE, HMC_DATAXO, I2C_MEMADD_SIZE_8BIT, databuffer, 6,HAL_MAX_DELAY);
}

bool HMC_ReadValues ()
{
	if(HAL_I2C_Mem_Read_DMA(&hi2c1, HMC_ADDR_WRITE, HMC_DATAXO, I2C_MEMADD_SIZE_8BIT, databuffer, 6) != HAL_OK)
	{
		return false;
	}

	return true;
}



bool Set_SingleMeasureMode()
{
	uint8_t single = 0x1;  //punel pe 1 sa fie in sigle measurement mode, daca lam setat pe 0 ii in continous measure mode

	if(HAL_I2C_Mem_Write(&hi2c1, HMC_ADDR_WRITE, 0x2, I2C_MEMADD_SIZE_8BIT, &single, 1, HAL_MAX_DELAY) != HAL_OK)
	{
		return false;
	}
	return true;
}

bool Set_ContinousMeasureMode()
{
	uint8_t single = 0x0;  //punel pe 1 sa fie in sigle measurement mode, daca lam setat pe 0 ii in continous measure mode

	if(HAL_I2C_Mem_Write(&hi2c1, HMC_ADDR_WRITE, 0x2, I2C_MEMADD_SIZE_8BIT, &single, 1, HAL_MAX_DELAY) != HAL_OK)
	{
		return false;
	}
	return true;
}

void HMC_IT_GetValuesXYZ(int16_t *x, int16_t *y, int16_t *z)
{
	*x = ((int16_t) databuffer[0] << 8) | databuffer[1];
	*y = ((int16_t) databuffer[2] << 8) | databuffer[3];
	*z = ((int16_t) databuffer[4] << 8) | databuffer[5];

	*x = *x * 92;      // pentru a obtine valorile corecte la x ,y ,z
 	*y = *y * 92;      // trb inmultite valorile cu 0.92(fol 92 ca sa nu folosim float ,la afisare trb prelucrate)
	*z = *z * 92;      // folosim 0.92 pentru ca sensor field range +- 1.3GA pentru alte valori folosim alt scale pag 13 datasheet

}










