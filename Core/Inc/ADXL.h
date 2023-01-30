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
/**
 * @brief checks if an exti interrupt has started for the ADXL(accelerometer)
 *
 * @return 1 if exti interrupt has started else 0
 */
static inline bool ADXL_IsExtiDataReady()
{
	return (HAL_GPIO_ReadPin(GPIO_EXTI_ACC_GPIO_Port, GPIO_EXTI_ACC_Pin) == GPIO_PIN_SET);
}

/**
 * @brief: initializes the accelorometer with the default configuration we neee so we can
 *         read acceleration values
 *
 */
void ADXL_Init(void);

/**
 * @brief starts measuring the acceleration raw data
 *
 * @return true if data is available else false
 */
bool ADXL_StartMeasurement(void);

/**
 * @brief  does calculations on the acceleration raw data to get the real acceleration values
 *
 * @param x  pointer to store the acceleration value on X axys
 * @param y  pointer to store the acceleration value on Y axys
 * @param z  pointer to store the acceleration value on Z axys
 */
void ADXL_GetValues(int16_t *x, int16_t *y, int16_t *z);

#endif /* INC_ADXL_H_ */
