#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pigpio.h>
#include <math.h>
#include "servo.h"
#include "main.h"
#include "app.h"

/* **** Function Declarations **** */
static int Servo_Set_Position( int pulse_width );

/***************************************************************************************************
Set the servo pulse with to 0 in order to turn off the PWM

***************************************************************************************************/
int Servo_Init( void ) { return Servo_Set_Position(0); }
int Servo_Shutdown( void ) { return Servo_Set_Position(0); }

/***************************************************************************************************
This function accepts a position command from the main loop and moves to that position.  A static 
variable timer is used to limit the time which the servo maybe energized in order to limit the 
seeking the servo performs.

If the current position is unknown, the static timer is set to 5 seconds in order to give the servo 
plenty of time to move to its new position. 

If the current position is known, the static timer is incremented by 8 mS for every count the servo 
needs to move from its current position.  If the position oscillates by a small amount, the timer 
may incur windup, so the timer is limited to a maximum of 5 seconds.

Every minute, the servo's known position flag is set to false.  This allows the servo's position to 
be reset in the event that it is not where it is expected.
***************************************************************************************************/
void Servo_Service( int position_cmd )
{
	#define FORCE_ENABLE_DELAY		(60 * 1000000)		/* 1 Minute */
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
		Servo_Set_Position(0);
		timer_us = 0;
	}
	else
	{
		Servo_Set_Position(position_cmd);
		timer_us -= MAIN_LOOP_TIME_US;
	}

	last_position_cmd = position_cmd;
}

/***************************************************************************************************
Much of the data returned from PIGPIOD is in ASCII format.  This function reads characters from
the file handle, converts them to an 8-bit signed value, and returns it as an int.
***************************************************************************************************/
static int Servo_Read_Ascii_Byte( FILE* pFile )
{
	char read_buffer[10] = { 0 };	// Longest expected read is 4 characters then a space or \n
	char ch;
	int i = 0;

	do {
		ch = fgetc(pFile);
		read_buffer[i++] = ch;
	} while (((ch >= '0') && (ch <= '9')) || (ch == '-'));

	return atoi(read_buffer);
}

/*******************************************************************************
This function obtains the mutex for the PiGPIO pipes, writes the specified
command to the pipe, and reads the result.  If the command is valid, the 
dev/pigout pipe will return 0.

Returns	-1 if the file pipes can't be opened
		 0 if the response is non-zero
		 1 if the response is zero
*******************************************************************************/
static int Servo_Set_Position( int width )
{
	FILE *pigpio_write;
	FILE *pigpio_read;

	int pigpio_response;
	int result = 0;

	pthread_mutex_lock(&pigpio_mutex);

	pigpio_write = fopen("/dev/pigpio", "w");
	pigpio_read = fopen("/dev/pigout", "r");

	if ((pigpio_write == NULL) || (pigpio_read == NULL))
	{
		printf("Error opening file handles - %s.%u\n", __FILE__, __LINE__);
		result = -1;
	}
	else
	{
		fprintf(pigpio_write, "s 18 %d\n", width);
		fflush(pigpio_write);

		// read result from pigout
		pigpio_response = Servo_Read_Ascii_Byte(pigpio_read);
		if (pigpio_response == 0)	// On a successful write to pigpio, pigout should return 0
			result = 1;
	}

	if (pigpio_write)
		fclose(pigpio_write);
	if (pigpio_read)
		fclose(pigpio_read);

	pthread_mutex_unlock(&pigpio_mutex);

	return result;
}

/* **** End of File **** */
