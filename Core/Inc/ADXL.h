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

static inline bool ADXL_IsExtiDataReady()
{
	return (HAL_GPIO_ReadPin(GPIO_EXTI_ACC_GPIO_Port, GPIO_EXTI_ACC_Pin) == GPIO_PIN_SET);
}
void ADXL_Init(void);
void ADXL_MeasureRawData_DMA(void);
void ADXL_GetValuesXYZ(int16_t *x, int16_t *y, int16_t *z);

#endif /* INC_ADXL_H_ */
