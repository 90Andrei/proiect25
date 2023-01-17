/*
 * GYRO.h
 *
 *  Created on: Dec 8, 2022
 *      Author: Andrei
 */

#ifndef INC_GYRO_H_
#define INC_GYRO_H_

static inline bool GYRO_IsExtiDataReady()
{
	return (HAL_GPIO_ReadPin(GPIO_EXTI_GYRO_GPIO_Port, GPIO_EXTI_GYRO_Pin) == GPIO_PIN_SET);
}

void GYRO_PowerMode();
void GYRO_ReadValuesXYZ(int16_t *x, int16_t *y, int16_t *z);
void GYRO_ReadXYZv2(int16_t *x, int16_t *y, int16_t *z);
void GYRO_XYZConv(int16_t *xg, int16_t *yg, int16_t *zg);
void GYRO_IT_GetValuesXYZ(int16_t *x, int16_t *y, int16_t *z);
void GYRO_SPI_IT_START();
#endif /* INC_GYRO_H_ */
