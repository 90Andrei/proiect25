/*
 * pressure_sensor.h
 *
 *  Created on: Jan 4, 2023
 *      Author: Andrei
 */

#ifndef INC_PRESSURE_SENSOR_H_
#define INC_PRESSURE_SENSOR_H_

#include <stdbool.h>
#include <stdint.h>

#define Pressure_Sensor_ADDR_WRITE 0xEE
#define Pressure_Sensor_ADDR_READ  0xEF

/**
 * @brief: initializes the pressure sensor with the default configuration we neee so we can
 *         read pressure values
 *
 */
bool Pressure_Sensor_Init(void);
/**
 * @brief updates the pressure sensor state, makes state_Update_s true
 *
 */
void Pressure_Sensor_UpdateState(void);
/**
 * @brief this is a function which returns the pressure value
 *
 * @return pressure value
 */
int32_t Pressure_Sensor_GetPresure(void);
/**
 * @brief this is a function which return the temperature value
 *
 * @return temperature
 */
int32_t Pressure_Sensor_GetTemperature(void);

/**
 * @brief cyclic task that passes threw all states necesarly to calculate temperature and pressure
 *
 * @return 1 when pressure and temperature is calculated else 0
 */
bool Pressure_Sensor_CyclicTask();

#endif /* INC_PRESSURE_SENSOR_H_ */
