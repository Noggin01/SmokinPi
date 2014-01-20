#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>
#include <pthread.h>
#include "tlc1543.h"
#include "thermistor.h"

#define false								0
#define true								(!false)

extern pthread_mutex_t mutex;

// Shared data used by multiple threads
typedef struct
{
	uint16_t adc_results[NBR_ADC_CHANNELS];		// Data read by the ADC
	uint8_t adc_data_available;					// Flag indicating new adc conversion data is available
	uint16_t servo_position;					// Position of the servo
	float temp_deg_f[NBR_OF_THERMISTORS];		// Temperature data resulting
} shared_data_type;								//	from the ADC conversions

#endif //__APP_H
