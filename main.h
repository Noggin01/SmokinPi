#ifndef __MAIN_H
#define __MAIN_H

#include <pthread.h>			// For pthread_mutex_t
#include <stdint.h>
#include <stdbool.h>
#include "tlc1543.h"			// For NBR_ADC_CHANNELS
#include "thermistor.h"			// For NBR_OF_THERMISTORS
#include "cmd_line.h"			// For MAX_CMD_LENGTH
#include "monitor.h"

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
	uint16_t adc_results[NBR_ADC_CHANNELS];		// Data read by the ADC
	uint16_t servo_position;							// Current position of the servo
	float temp_deg_f[NBR_OF_THERMISTORS];			// Temperature data resulting from the ADC conversions
	float temp_deg_f_fire;								// Thermocouple temperature
   float temp_deg_f_cabinet_setpoint;           // Setpoint of the cabinet
	fire_detect_state_type fire_detect_state;		// Indicates whether the system is looking for fire, sees it, or has lost it
	debug_flags_type debug_flags;						// Debug flags for enabling and disabling debugging features										
} shared_data_type;

// Shared data used by the command line
typedef struct
{
	int cmd_number;
	char command[MAX_CMD_LENGTH];
} shared_cmd_line_data_type;

#endif //__MAIN_H
