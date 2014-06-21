#ifndef __MAIN_H
#define __MAIN_H

#include <pthread.h>			// For pthread_mutex_t
#include "tlc1543.h"			// For NBR_ADC_CHANNELS
#include "thermistor.h"			// For NBR_OF_THERMISTORS
#include "cmd_line.h"			// For MAX_CMD_LENGTH

#ifndef false
#define false												0
#endif
#ifndef true
#define true												(!false)
#endif

#define MAIN_LOOP_TIME_US									5000

extern pthread_mutex_t mutex;
extern pthread_mutex_t pigpio_mutex;

// Shared data used by multiple threads
typedef struct
{
	unsigned int adc_results[NBR_ADC_CHANNELS];			// Data read by the ADC
	unsigned int adc_data_available;					// Flag indicating new adc conversion data is available
	int servo_position;									// Current position of the servo
	float temp_deg_f[NBR_OF_THERMISTORS];				// Temperature data resulting from the ADC conversions
	debug_flags_type debug_flags;						// Debug flags for enabling and disabling debugging features										
} shared_data_type;

// Shared data used by the command line
typedef struct
{
	int cmd_number;
	char command[MAX_CMD_LENGTH];
} shared_cmd_line_data_type;

#endif //__MAIN_H
