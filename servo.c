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
	int result;

	result = gpioInitialise();
	if (result < 1)
	{
		gpioTerminate();
		result = gpioInitialise();
	}
	
	if (result < 1)
		printf("Failure initializing PiGPIO  - %d\n", result);
	else
		printf("Initializing Servo\n");

	return result;
}

void Servo_Shutdown( void )
{
	gpioTerminate();
}

/*******************************************************************************
This function accepts a position command from the main loop and moves to that
position.  If the current position of the servo is unknown, the servo's motion
will be enabled for 5 seconds.  If the servo's position is known, then the
motion will be enabled for 
*******************************************************************************/
void Servo_Service( int position_cmd )
{
	static unsigned char position_known = false;
	static int last_position_cmd;
	static int timer_us = 0;
	int distance;

	if (!position_known)
	{
		timer_us = 5000000;	// 5 seconds
		position_known = true;
	}
	else if (last_position_cmd != position_cmd)
	{
		distance = abs(last_position_cmd - position_cmd);
		timer_us = distance * 8000;
	}

	if (position_cmd < MIN_PHYSICAL_POSITION)
		position_cmd = MIN_PHYSICAL_POSITION;
	if (position_cmd > MAX_PHYSICAL_POSITION)
		position_cmd = MAX_PHYSICAL_POSITION;
	
	if (timer_us <= 0)
		gpioServo( 18, 0 );
	else
	{
		gpioServo( 18, position_cmd );
		timer_us -= MAIN_LOOP_TIME_US;
	}

	last_position_cmd = position_cmd;
}
