#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include "main.h"
#include "thermistor.h"
#include "tlc1543.h"
#include "app.h"

/* *** Constants *** */
#define NBR_TABLE_ENTRIES	(sizeof(conversion_table)/sizeof(temp_lookup_entry))

/* *** Types *** */
typedef struct
{
	uint16_t adc;
	float deg_f;
} temp_lookup_entry;

/* *** Global Variables *** */
const static double conversion_coefficients[NBR_THERMISTOR_TYPES][NBR_OF_COEFFICIENTS] =
{
	{ -9.8204691773678E-12, 3.40492154074195E-08, -4.76413138941701E-05, 0.0334734910682721, -12.0393270605721, 2037.5487133759 },		// CDDT Replacement Thermistor
	{ -4.84762768227874E-12, 1.08664475188776E-08, -9.8930466470565E-06, 0.00469182824980359, -1.38600106692554, 379.858164641871 },	// Taylor Thermometer
}

uint8_t thermistor_data_initialized = false;


/* *** Function Declarations *** */
float Thermistor_Convert_Adc_To_Deg_x10( uint16_t adc, uint8_t i );

void Thermistor_Init( void )
{
	printf("Initializing Thermistor Data\n");
}

void Thermistor_Service( void *shared_data_address )
{
	uint8_t i;
	uint16_t raw_adc_data;
	float degf;
	uint16_t local_adc_results[NBR_ADC_CHANNELS];
	float local_temperature_deg_f[NBR_OF_THERMISTORS];
	uint16_t local_servo_position;

	shared_data_type* p_shared_data = (shared_data_type*)shared_data_address;

	while (1)
	{
		Delay_Seconds(1);

		pthread_mutex_lock(&mutex);
		memcpy( (uint8_t*)local_adc_results,
				(uint8_t*)p_shared_data->adc_results,
				sizeof(local_adc_results));
		p_shared_data->adc_data_available = false;
		local_servo_position = p_shared_data->servo_position;
		pthread_mutex_unlock(&mutex);

		for (i = 0; i < NBR_OF_THERMISTORS; i++)
		{
			raw_adc_data = local_adc_results[i];
			degf = Thermistor_Convert_Adc_To_Deg_x10( raw_adc_data, i );
			if (thermistor_data_initialized)
			{
				local_temperature_deg_f[i] *= 9;
				local_temperature_deg_f[i] += degf;
				local_temperature_deg_f[i] /= 10;
			}
			else
				local_temperature_deg_f[i] = degf;

			printf("%2u:%4.2f ", i, local_temperature_deg_f[i]);
		}
		printf( "%4u\n", local_servo_position );

		pthread_mutex_lock(&mutex);
		memcpy( (uint8_t*)p_shared_data->temp_deg_f,
				(uint8_t*)local_temperature_deg_f,
				sizeof(p_shared_data->temp_deg_f));
		pthread_mutex_unlock(&mutex);

		thermistor_data_initialized = true;
	}
}

float Thermistor_Convert_Adc_To_Deg_x10( uint16_t adc, uint8_t thermistor_index )
{
	double temp_deg_f;
	unsigned int thermistor_type;

	switch (thermistor_index)
	{
		case 0:
		case 1:
			thermistor_type = TAYLOR_THERMISTOR;
			break;

		default:
			thermistor_type = CDDT_THERMISTOR;
			break;
	}

	temp_deg_f  = conversion_coefficients[thermistor_type][0] * adc * adc * adc * adc * adc;
	temp_deg_f += conversion_coefficients[thermistor_type][1] * adc * adc * adc * adc;
	temp_deg_f += conversion_coefficients[thermistor_type][2] * adc * adc * adc;
	temp_deg_f += conversion_coefficients[thermistor_type][3] * adc * adc;
	temp_deg_f += conversion_coefficients[thermistor_type][4] * adc;
	temp_deg_f += conversion_coefficients[thermistor_type][5];

	return (float)temp_deg_f;
}

