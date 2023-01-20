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
void GYRO_Init();
void GYRO_IT_GetValuesXYZ(int16_t *x, int16_t *y, int16_t *z);
void GYRO_MeasureRawData_DMA();
#endif /* INC_GYRO_H_ */
