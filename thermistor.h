#ifndef _THERMISTOR_H
#define _THERMISTOR_H

#include <stdint.h>

#define NBR_OF_THERMISTORS			(NBR_ADC_CHANNELS - 1)
#define NBR_OF_COEFFICIENTS		6

typedef enum
{
	CDDT_THERMISTOR,
	TAYLOR_THERMISTOR,

	NBR_THERMISTOR_TYPES,
} thermistor_types;

void Thermistor_Init( void );
void Thermistor_Service( uint16_t *p_adc_data, float *p_temperature_data );

#endif
