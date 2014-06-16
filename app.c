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

/* *** Global Variables *** */
static float g_forced_cabinet_temp_deg_f;
static int g_forced_servo_position;

static pid_type g_pid;

/* *** Accessors *** */
void App_Force_Cabinet_Temperature( float temp_deg_f ){ g_forced_cabinet_temp_deg_f = temp_deg_f; }
void App_Force_Servo_Position( int position ){ g_forced_servo_position = position; }
void App_Set_Kp( double gain ){ g_pid.proportional_gain = gain; }
void App_Set_Ki( double gain ){ g_pid.integral_gain = gain; }
void App_Set_Kd( double gain ){ g_pid.derivative_gain = gain; }
void App_Set_Kl( double limit ){ g_pid.windup_guard = limit; }

void App_Init( void )
{
	Pid_Reset( &g_pid );
	g_pid.windup_guard = 500000.0;
	g_pid.proportional_gain = 5.0;
	g_pid.integral_gain = 0.0002;
	g_pid.derivative_gain = 0.0;
}

/**************************************************************************
This is where the magic happens.  The ADC measurements are being taken
in the background, the logging is happening in the background.  This 
service routine is called from the main loop periodically and is
responsible for setting the desired setpoint, executing the PID, and 
setting the destination for the servo motor.
**************************************************************************/
void App_Service( void* shared_data_address )
{
	#define RECALCULATE_DELAY				(20000/MAIN_LOOP_TIME_US)		/* 20 ms */
	#define PRINT_DELAY						(1000000/MAIN_LOOP_TIME_US)	/* 1 Second */
	uint16_t adc_data[NBR_ADC_CHANNELS];
	float temperature_data[NBR_OF_THERMISTORS];
	shared_data_type* p_shared_data = (shared_data_type*)shared_data_address;
	int x, y;
	
	static float cabinet_temperature = 0;
	static int servo_position;
	static int timer = 0;
	static int print_timer = 0;
	float temperature_error;
	
	pthread_mutex_lock(&mutex);
	memcpy( (char*)adc_data, (char*)p_shared_data->adc_results, sizeof(adc_data) );
	memcpy( (char*)temperature_data, (char*)p_shared_data->temp_deg_f, sizeof(temperature_data) );
	pthread_mutex_unlock(&mutex);

	Thermistor_Service( adc_data, temperature_data );
	cabinet_temperature = temperature_data[0];
	temperature_error = (float)g_forced_cabinet_temp_deg_f - cabinet_temperature;
		
	if (++timer >= RECALCULATE_DELAY)
	{
		timer = 0;
		
		Pid_Update(&g_pid, (double)temperature_error, (double)(MAIN_LOOP_TIME_US/1000));
		
		servo_position = g_pid.control + MIN_POSITION_FOR_OPERATION;
		if (servo_position < MIN_POSITION_FOR_OPERATION)
			servo_position = MIN_POSITION_FOR_OPERATION;
		if (servo_position > MAX_POSITION_FOR_OPERATION)
			servo_position = MAX_POSITION_FOR_OPERATION;
		
		if (g_forced_servo_position > 0)
			Servo_Service(g_forced_servo_position);
		else
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
//		move( 5, 50 );
//		printw( "     KD:  %4.5f      ", g_pid.derivative_gain );
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
		move( y, x );
	}

	pthread_mutex_lock(&mutex);
	memcpy( (char*)p_shared_data->adc_results, (char*)adc_data, sizeof(adc_data) );
	memcpy( (char*)p_shared_data->temp_deg_f, (char*)temperature_data, sizeof(temperature_data) );
	pthread_mutex_unlock(&mutex);
}

#if 0
/**************************************************************************
This is where the magic happens.  The ADC measurements are being taken
in the background, the logging is happening in the background.  This 
service routine is called from the main loop periodically and is
responsible for setting the desired setpoint, executing the PID, and 
setting the destination for the servo motor.
**************************************************************************/
void App_Service( void* shared_data_address )
{
	#define RECALCULATE_DELAY_US						5000000	/* 5 seconds */
	uint16_t adc_data[NBR_ADC_CHANNELS];
	float temperature_data[NBR_OF_THERMISTORS];
	shared_data_type* p_shared_data = (shared_data_type*)shared_data_address;
	int x, y;
	int servo_position;
	
	static float cabinet_temperature = 0;
	static int calculated_servo_position = 1000;
	static int timer = 0;
	
	pthread_mutex_lock(&mutex);
	memcpy( (char*)adc_data, (char*)p_shared_data->adc_results, sizeof(adc_data) );
	memcpy( (char*)temperature_data, (char*)p_shared_data->temp_deg_f, sizeof(temperature_data) );
	pthread_mutex_unlock(&mutex);

	Thermistor_Service( adc_data, temperature_data );
	cabinet_temperature = temperature_data[0];
//	Pid_Service();
//	Servo_Service( 0 );

	if (timer <= 0)
	{
		if (cabinet_temperature > (g_forced_cabinet_temp_deg_f + 1))
			calculated_servo_position-=2;
		else if (cabinet_temperature < (g_forced_cabinet_temp_deg_f - 1))
			calculated_servo_position++;

		if (calculated_servo_position < MIN_POSITION_FOR_FIRE)
			calculated_servo_position = MIN_POSITION_FOR_FIRE;
		else if (calculated_servo_position > MAX_POSITION)
			calculated_servo_position = MAX_POSITION;

		if (g_forced_servo_position > 0)
		{
			servo_position = g_forced_servo_position;
			calculated_servo_position  = servo_position;
		}
		else
			servo_position = calculated_servo_position;

		if (servo_position < MIN_POSITION)
			servo_position = 0;
		else if (servo_position > MAX_POSITION)
			servo_position = 0;
		
		getyx(stdscr, y, x);
		move( 2, 0 );
		printw("Servo: %d    \n", servo_position);
		move( y, x );
		
		timer = RECALCULATE_DELAY_US;

		Servo_Service( servo_position );
	}
	else
		timer -= MAIN_LOOP_TIME_US;

	pthread_mutex_lock(&mutex);
	memcpy( (char*)p_shared_data->adc_results, (char*)adc_data, sizeof(adc_data) );
	memcpy( (char*)p_shared_data->temp_deg_f, (char*)temperature_data, sizeof(temperature_data) );
	pthread_mutex_unlock(&mutex);
}
#endif
