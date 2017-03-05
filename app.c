#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <ncurses.h>
#include "main.h"
#include "thermistor.h"
#include "tlc1543.h"
#include "servo.h"
#include "app.h"
#include "pid.h"
#include "monitor.h"

/* *** Global Variables *** */

static int g_forced_servo_position;

static pid_type g_pid;

static char g_channel_names[NBR_OF_THERMISTORS][MAX_NAME_LENGTH];

static shared_data_type* p_shared_data;

/* *** Prototypes *** */
static float App_Calculate_Thermocouple_Temperature( uint16_t adc_counts );

/* *** Accessors *** */
void App_Set_Kp( float gain ){ if (gain > 0) g_pid.proportional_gain = gain; }
void App_Set_Ki( float gain ){ if (gain > 0) g_pid.integral_gain = gain; }
void App_Set_Kd( float gain ){ if (gain > 0) g_pid.derivative_gain = gain; }
void App_Set_Kl( float limit ){ if (limit > 0) g_pid.windup_guard = limit; }

float App_Get_Kp( void ){ return g_pid.proportional_gain; }
float App_Get_Kd( void ){ return g_pid.derivative_gain; }
float App_Get_Ki( void ){ return g_pid.integral_gain; }
float App_Get_Kl( void ){ return g_pid.windup_guard; }

void App_Init( void* shared_data_address )
{
	int i;

	Pid_Reset( &g_pid );
	g_pid.windup_guard = 4000000.0;
	g_pid.proportional_gain = 20.0;
	g_pid.integral_gain = 0.0001;
	g_pid.derivative_gain = 0.0;

	strcpy(g_channel_names[0], "Cabinet");
	for (i = 1; i < NBR_OF_THERMISTORS; i++)
		strcpy( g_channel_names[i], "Not Set" );
    
    p_shared_data = (shared_data_type*)shared_data_address;
    
    pthread_mutex_lock(&mutex);
	p_shared_data->temp_deg_f_cabinet_setpoint = 225.0;
	pthread_mutex_unlock(&mutex);	
}

/**************************************************************************
This is where the magic happens.  The ADC measurements are being taken
in the background, the logging is happening in the background.  This 
service routine is called from the main loop periodically and is
responsible for setting the desired setpoint, executing the PID, and 
setting the destination for the servo motor.
**************************************************************************/
void App_Service( void )
{
	#define RECALCULATE_DELAY				(20000/MAIN_LOOP_TIME_US)		   /* 20 ms */
	#define PRINT_DELAY						(2500000/MAIN_LOOP_TIME_US)		/* 250 ms */
	uint16_t adc_data[NBR_ADC_CHANNELS];
	float temperature_data[NBR_OF_THERMISTORS];
	float thermocouple_temperature;
    float setpoint;
	int x, y;
	fire_detect_state_type fire_detect_state;

	static float cabinet_temperature = 0.0;
	static int servo_position;
	static int timer = 0;
	static int print_timer = 0;
	float temperature_error;

	// Obtain a lock on the shared data so that a copy can be made
	pthread_mutex_lock(&mutex);
	memcpy( (char*)adc_data, (char*)p_shared_data->adc_results, sizeof(adc_data) );
	memcpy( (char*)temperature_data, (char*)p_shared_data->temp_deg_f, sizeof(temperature_data) );
	fire_detect_state = p_shared_data->fire_detect_state;
    setpoint = p_shared_data->temp_deg_f_cabinet_setpoint;
	pthread_mutex_unlock(&mutex);

	// Call the thermistor service routine and have it convert the ADC measurements to temperatures
	Thermistor_Service( adc_data, temperature_data );
	
	// Calculate the thermocouple temperature.  The conversion data is the last ADC channel
	thermocouple_temperature = App_Calculate_Thermocouple_Temperature( adc_data[NBR_ADC_CHANNELS-1] );
	cabinet_temperature = temperature_data[0];
	temperature_error = setpoint - cabinet_temperature;

	// Periodically update the PID data
	if (++timer >= RECALCULATE_DELAY)
	{
		timer = 0;
		
		Pid_Update(&g_pid, (double)temperature_error, (double)(MAIN_LOOP_TIME_US/1000));
		
		// The PID outputs a number from 0 to X depending on the gains.  Limit the servo
		// position to minimum and maximum values.  Too low and the flame will go out,
		// and too high just doesn't do anybody any good
		servo_position = g_pid.control + MIN_POSITION_FOR_OPERATION;
		if (servo_position < MIN_POSITION_FOR_OPERATION)
			servo_position = MIN_POSITION_FOR_OPERATION;
		if (servo_position > MAX_POSITION_FOR_OPERATION)
			servo_position = MAX_POSITION_FOR_OPERATION;
		
		switch (fire_detect_state)
		{
			case MONITOR_WAITING_FOR_FIRE:
				servo_position = MAX_POSITION_FOR_OPERATION;
				break;
				
			case MONITOR_FIRE_DETECTED:
				// do nothing
				break;
	
			default:
			case MONITOR_FIRE_LOST:
				servo_position = MIN_PHYSICAL_POSITION;
				break;
		}
		
		Servo_Service( servo_position );
	}
	
		// Print PID information to the console for easy monitoring
	if (print_timer++ >= PRINT_DELAY)
	{
		print_timer = 0;
		getyx(stdscr, y, x);
		move( 3, 50 );
		printw( "Tmp Err:  %4.5f      ", temperature_error );
		move( 4, 50 );
		printw( "     KP:  %4.5f      ", g_pid.proportional_gain );
		move( 5, 50 );
		printw( "     KI:  %4.5f      ", g_pid.integral_gain );
		move( 6, 50 );
		printw( "     KL:  %4.5f      ", g_pid.windup_guard );
		move( 7, 50 );
		printw( "Pro Err:  %4.5f      ", temperature_error * g_pid.proportional_gain );
		move( 8, 50 );
		printw( "Int Err:  %4.5f      ", g_pid.int_error );
		move( 9, 50 );
		printw( " Output:  %4.5f      ", g_pid.control );
		move( 10, 50 );
		printw( "  Servo:  %d         ", servo_position);
		move (12, 50);
		printw( "   Fire:  %4.2f      ", thermocouple_temperature);
		move (14, 50);
		printw( "   Raw:   %u         ", adc_data[NBR_ADC_CHANNELS-1]);
		move( y, x );
	}
	
	// Update the shared data
	pthread_mutex_lock(&mutex);
	memcpy( (char*)p_shared_data->temp_deg_f, (char*)temperature_data, sizeof(temperature_data) );
	p_shared_data->temp_deg_f_fire = thermocouple_temperature;
	p_shared_data->servo_position = servo_position;
	pthread_mutex_unlock(&mutex);
	
}

/** ***********************************************************************************************
@brief Sets the temperature setpoint of the cabinet_temperature

@param[in] temp_deg_f   Setpoint for the system
**************************************************************************************************/
void App_Set_Cabinet_Setpoint( float temp_deg_f )
{
    pthread_mutex_lock(&mutex);
	p_shared_data->temp_deg_f_cabinet_setpoint = temp_deg_f;
    pthread_mutex_unlock(&mutex);
}

/**************************************************************************
Sets the channel names so that they can be used for displaying data at a
later time
**************************************************************************/
void App_Set_Channel_Name( int channel, char* pName )
{
	if ((channel < NBR_OF_THERMISTORS) && (strlen(pName) < MAX_NAME_LENGTH))
		strcpy( g_channel_names[channel], pName );
}

char* App_Get_Channel_Name( int channel )
{
	static char name[MAX_NAME_LENGTH];

	if (channel < NBR_OF_THERMISTORS)
		strcpy( name, g_channel_names[channel] );
	else
		name[0] = 0;

	return name;
}

/**************************************************************************
Converts the ADC counts value to a K Type thermocouple temperature.  The 
conversion is 

	temperature = (Voltage - 1.25v)/0.005v
	
The voltage reference on the ADC converter is 3.3v and is a 10-bit ADC
**************************************************************************/
static float App_Calculate_Thermocouple_Temperature( uint16_t adc_counts )
{
	#define FILTER_WEIGHT			0.99
	static float filtered_deg_f = 0;
	static uint8_t initialized = 0;

	float voltage = (float)adc_counts * 5.0 / 1024.0;
	float deg_c = (voltage - 1.25) / .005;
	float deg_f = deg_c * 1.8 + 32.0;
	
	if (!initialized)
	{
		filtered_deg_f = deg_f;
		initialized = true;
	}

	filtered_deg_f = (FILTER_WEIGHT * filtered_deg_f) + ((1.0-FILTER_WEIGHT) * deg_f);

	return filtered_deg_f;
}
