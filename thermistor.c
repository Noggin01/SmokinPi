#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ncurses.h>
#include "main.h"
#include "thermistor.h"
#include "tlc1543.h"
#include "app.h"
#include "cmd_line.h"

/* *** Constants *** */
#define nan					(1/0)
/* *** Types *** */

/* *** Global Variables *** */
const static double conversion_coefficients[NBR_THERMISTOR_TYPES][NBR_OF_COEFFICIENTS] =
{
	{ -1.07141580387266E-11, 3.71346262993203E-08, -5.18467410539453E-05, 0.0363008256347458, -12.9765706452615, 2160.07499094298 },	// CDDT Replacement Thermistor
	{ -2.59799933143503E-12, 6.61760177285894E-09, -6.86167983753389E-06, 0.00368089722332718, -1.23009384419463, 371.041602633187 },	// Taylor Thermometer
};

/* *** Function Declarations *** */
float Thermistor_Convert_Adc_To_Deg_F( uint16_t adc, uint8_t i );

/***************************************************************************************************
Initialize thermistor data to nan (not a number) so that the service routine will recognize that 
the data isn't valid
***************************************************************************************************/
void Thermistor_Init( void )
{
	// Aint got shit to do right now
	printf("Thermistor data initialized\n");
}

/***************************************************************************************************
When new ADC data is available, convert it to temperature in Deg F
***************************************************************************************************/
void Thermistor_Service( uint16_t *p_adc_data, float *p_temperature_data )
{
	#define PRINT_DELAY					(1000000/MAIN_LOOP_TIME_US)	/* 1 seconds */
	uint8_t i;
	float degf;
	int y, x;
	static int timer = 0;
	static int initialized = 0;
	
	// Convert each of the ADC data points to temperature data
	for (i = 0; i < NBR_OF_THERMISTORS; i++)
	{
		degf = Thermistor_Convert_Adc_To_Deg_F( p_adc_data[i], i );
		if (initialized == 1)
			p_temperature_data[i] = ((p_temperature_data[i] * 9.0) + degf)/10.0;
		else
			p_temperature_data[i] = degf;
	}

	initialized = 1;
	
	// Print temperature information to the console for easy monitoring
	if (timer++ >= PRINT_DELAY)
	{
		timer = 0;
		getyx(stdscr, y, x);
		move( 0, 0 );
		for (i = 0; i < NBR_OF_THERMISTORS; i++)
			printw("%2u:%4.2f", i, p_temperature_data[i]);
		printw("          \n");
		move( y, x );
	}
}

/***************************************************************************************************
Determine which type of thermistor each this conversion is for.  Then calculate the temperature 
using a polynomial forumla obtained with ADC vs Temperature plots in Excel.
***************************************************************************************************/
float Thermistor_Convert_Adc_To_Deg_F( uint16_t adc, uint8_t thermistor_index )
{
	double temp_deg_f;
	unsigned int thermistor_type;

	switch (thermistor_index)
	{
//		case 0:
//		case 1:
//			thermistor_type = TAYLOR_THERMISTOR;
//			break;

		default:
			thermistor_type = CDDT_THERMISTOR;
			break;
	}

	// Convert the ADC reading to temperature in °F using the polynomial formula
	// Temperature = Ax^5 + Bx^4 + Cx^3 + Dx^2 + Ex + F
	temp_deg_f  = conversion_coefficients[thermistor_type][0] * adc * adc * adc * adc * adc;
	temp_deg_f += conversion_coefficients[thermistor_type][1] * adc * adc * adc * adc;
	temp_deg_f += conversion_coefficients[thermistor_type][2] * adc * adc * adc;
	temp_deg_f += conversion_coefficients[thermistor_type][3] * adc * adc;
	temp_deg_f += conversion_coefficients[thermistor_type][4] * adc;
	temp_deg_f += conversion_coefficients[thermistor_type][5];

	return (float)temp_deg_f;
}

