#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "main.h"
#include "thermistor.h"
#include "tlc1543.h"
#include "servo.h"
#include "app.h"

/* *** Global Variables *** */

/**************************************************************************

**************************************************************************/
int Delay_Microseconds( unsigned int delay_us )
{
	return usleep(delay_us);
}

int Delay_Milliseconds( unsigned int delay_ms )
{
	int result = 0;

	while ((delay_ms > 1000000) && (result != -1))
	{
		result = Delay_Microseconds(1000000);
		delay_ms -= 1000000;
	}

	if (result != -1)
		result = Delay_Microseconds(delay_ms);

	return result;
}

int Delay_Seconds( unsigned int secs )
{
	while (secs)
		secs = sleep(secs);

	return secs;
}


void App_Init( void )
{

}

void App_Service( void )
{
	while (1)
	{
        sleep(1);
	}
}
