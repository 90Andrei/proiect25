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

typedef enum
{
	BMP_State_Temp_StartMeasurement,
	BMP_State_Wait_TempMeasurement,
	BMP_State_Temp_ReadResult,
    BMP_State_Temp_Measure,
	BMP_State_Pressure_StartMeasurement,
	BMP_State_Wait_PresMeasurement,
	BMP_State_Pressure_ReadResult,
    BMP_State_Pressure_Measure,
}BMP_STATE;

bool BMP_Init(void);
int32_t BMP_GetPres(void);
int32_t BMP_State_handler();
int32_t BMP_GetPres(void);

#endif /* INC_BMP_H_ */
