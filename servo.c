#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pigpio.h>
#include <math.h>
#include "servo.h"
#include "main.h"
#include "app.h"


/*******************************************************************************
This function initializes the servo by first terminating the pigpio C library 
functions if they are enabled, and then initializing them again.

Note:  gpioInitialise() requires the program to be run as sudo
*******************************************************************************/
int Servo_Init( void )
{
	int result = 0;
	char cmd[50];
	int response;

	FILE *servo = fopen("/dev/pigpio", "w");

	if (servo == NULL)
	{
		printf("Error opening file: /dev/pigpio - %s.%u\n", __FILE__, __LINE__);
		result = -1;
	}
	else
	{
		fprintf(servo, "s 18 40\n");
		fflush(servo);
		fclose(servo);

		// read result from pigout
		servo = fopen("/dev/pigout", "r");
		if (servo == NULL)
		{
			printf("Error opening file: /dev/pigout - %s.%u\n", __FILE__, __LINE__);
			result = -1;
		}
		else
		{
			fscanf(servo, "%d", &response);
			if (response == 0)	// On a successful write to pigpio, pigout should return 0
				result = 1;
			fclose(servo);
		}
	}

	if (result == 1)
		printf("Servo control is happy!\n");
	else
		printf("Servo control is not happy :(\n");

	return result;
}

void Servo_Shutdown( void )
{
	gpioTerminate();
}

/*******************************************************************************
This function accepts a position command from the main loop and moves to that
position.  A static variable timer is used to limit the time which the servo may
be energized in order to limit the seeking the servo performs.

If the current position is unknown, the static timer is set to 5 seconds in
order to give the servo plenty of time to move to its new position. 

If the current position is known, the static timer is incremented by 8 mS for
every count the servo needs to move from its current position.  If the position
oscillates by a small amount, the timer may incur windup, so the timer is
limited to a maximum of 5 seconds.

Every minute, the servo's known position flag is set to false.  This allows the
servo's position to be reset in the event that it is not where it is expected.
*******************************************************************************/
void Servo_Service( int position_cmd )
{
	#define FORCE_ENABLE_DELAY		(60 * 1000000)		/* 1 Minutes */
	static unsigned char position_known = false;
	static int last_position_cmd;
	static int timer_us = 0;
	
	static int force_enable_timer = 0;

	int distance;

	// Every so often, set position known to false.  This will force the servo
	// to move to its intended position in case it is not where it is expected
	force_enable_timer += MAIN_LOOP_TIME_US;
	if (force_enable_timer >= FORCE_ENABLE_DELAY)
	{
		force_enable_timer = 0;
		position_known = false;
	}

	if (!position_known)
	{
		timer_us = 5000000;	// 5 seconds
		position_known = true;
	}
	else if (last_position_cmd != position_cmd)
	{
		distance = abs(last_position_cmd - position_cmd);
		timer_us += (distance * 8000);
		if (timer_us > 5000000)
			timer_us = 5000000;
	}

	if (position_cmd < MIN_PHYSICAL_POSITION)
		position_cmd = MIN_PHYSICAL_POSITION;
	if (position_cmd > MAX_PHYSICAL_POSITION)
		position_cmd = MAX_PHYSICAL_POSITION;
	
	if (timer_us <= 0)
	{
		gpioServo( 18, 0 );	
		timer_us = 0;
	}
	else
	{
		gpioServo( 18, position_cmd );
		timer_us -= MAIN_LOOP_TIME_US;
	}

	last_position_cmd = position_cmd;
}
