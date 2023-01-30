/*
 * GYRO.h
 *
 *  Created on: Dec 8, 2022
 *      Author: Andrei
 */

#ifndef INC_GYRO_H_
#define INC_GYRO_H_

/**
 * @brief checks if an exti interrupt has started for the GYRO(gyroscope)
 *
 * @return 1 if exti interrupt has started else 0
 */
static inline bool GYRO_IsExtiDataReady()
{
	return (HAL_GPIO_ReadPin(GPIO_EXTI_GYRO_GPIO_Port, GPIO_EXTI_GYRO_Pin) == GPIO_PIN_SET);
}

/**
 * @brief: initializes the gyroscope with the default configuration we neee so we can
 *         read gyroscope values
 *
 */
void GYRO_Init();

/**
 * @brief  does calculations on the gyroscope raw data to get the real gyroscope values
 *
 * @param x  pointer to store the gyroscope value on X axys
 * @param y  pointer to store the gyroscope value on Y axys
 * @param z  pointer to store the gyroscope value on Z axys
 */
void GYRO_GetValues(int16_t *x, int16_t *y, int16_t *z);

/**
 * @brief starts measuring the gyroscope raw data
 *
 * @return true if data is available else false
 */
bool GYRO_StartMeasurement();
#endif /* INC_GYRO_H_ */
