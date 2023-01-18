/*
 * BMP.h
 *
 *  Created on: Jan 4, 2023
 *      Author: Andrei
 */

#ifndef INC_BMP_H_
#define INC_BMP_H_

#include <stdbool.h>
#include <stdint.h>

#define BMP_ADDR_WRITE 0xEE
#define BMP_ADDR_READ  0xEF

bool BMP_Init(void);
void BMP_UpdateState(void);
int32_t BMP_GetPresure(void);
int32_t BMP_GetTemperature(void);
bool BMP_CyclicTask();

#endif /* INC_BMP_H_ */
