/*
 * HMC.h
 *
 *  Created on: Dec 29, 2022
 *      Author: Andrei
 */

#ifndef INC_HMC_H_
#define INC_HMC_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief: initializes the HMC(compass) with the default configuration we neee so we can
 *         read acceleration values
 *
 */
bool HMC_Init();

/**
 * @brief sets the HMC in single measurement mode
 *
 * @return 1 if the setup was ok else 0
 */
bool HMC_SetSingleMeasurentMode();

/**
 * @brief  reads HMC raw values and stores them in a data buffer
 *
 * @return size of data that was read , -1 in case of error
 */
int8_t HMC_ReadValues();

/**
 * @brief  does calculations on the HMC raw data to get the real compass values
 *
 * @param x  pointer to store the compass value on X axys
 * @param y  pointer to store the compass value on Y axys
 * @param z  pointer to store the compass value on Z axys
 */
void HMC_GetValues(int16_t *x, int16_t *y, int16_t *z);

#endif /* INC_HMC_H_ */
