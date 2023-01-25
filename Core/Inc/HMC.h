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

bool HMC_Init();
bool HMC_SetSingleMeasurentMode();
bool HMC_ReadValues();
void HMC_GetValues(int16_t *x, int16_t *y, int16_t *z);

#endif /* INC_HMC_H_ */
