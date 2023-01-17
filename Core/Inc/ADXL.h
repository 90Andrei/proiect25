/*
 * ADXL.h
 *
 *  Created on: Dec 6, 2022
 *      Author: Andrei
 */

#ifndef INC_ADXL_H_
#define INC_ADXL_H_

#include "main.h"
#include <stdbool.h>

#define ADXL_DATAX0 0x32
#define ADXL_POWER_CTL 0x2D
#define ADXL345_SCALE_FACTOR    39  //  if is 2g  , if is 4g *2  8g *4 16g*8

static inline bool ADXL_IsExtiDataReady()
{
	return (HAL_GPIO_ReadPin(GPIO_EXTI_ACC_GPIO_Port, GPIO_EXTI_ACC_Pin) == GPIO_PIN_SET);
}

void ADXL_ReadValuesXYZ(int16_t *x, int16_t *y, int16_t *z);
void ADXL_Set_Measurebit();
bool ADXL_ReadMultipleBytes(uint8_t firstRegAddress, uint8_t * regValues, uint8_t lenght);
void ADXL_ConvertXYZValuesG(int16_t *xg , int16_t *yg, int16_t *zg);
void ADXL_DMA_ReadValuesXYZ(int16_t *x, int16_t *y, int16_t *z);
void ADXL_INIT_IT();
bool ADXL_ReadData(uint8_t regAddress, uint8_t *registerValue);
int ADXL_Init(void);
void ADXL_IT_StartSPI();
void ADXL_IT_StopSPI();
void ADXL_IT_GetValuesXYZ(int16_t *x, int16_t *y, int16_t *z);

#endif /* INC_ADXL_H_ */
