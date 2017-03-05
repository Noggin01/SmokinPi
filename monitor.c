
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pigpio.h>
#include <pthread.h>
#include <ncurses.h>
#include <time.h>
#include "monitor.h"
#include "main.h"
#include "email.h"

#ifndef NOTIFICATION_EMAIL_ADDRESS
#warning Need to define email reception address else notifications will not be sent
	#if 0
// Create a file named email.h with the following contents

	#ifndef _EMAIL_H
	#define _EMAIL_H

	#define NOTIFICATION_EMAIL_ADDRESS	"email@some_site.com"
	
	#endif

// Replace email@some_site.com with your email address.
// You may use an email-to-sms address if desired.
	#endif
#endif


/* *** Defined Values *** */
#define MONITOR_SERVICE_RATE_MS				100				

static fire_detect_state_type g_fire_detect_state = MONITOR_WAITING_FOR_FIRE;

/* *** Global Variables *** */

/* *** Function Declarations *** */
static void _Monitor_Service_Fire_Loss_Detection( void* shared_data_address );

/* *** Accessors *** */
void Monitor_Light_Fire( void )
{
	Monitor_Send_Notification( "Notice", "Opening valve for lighting" );
	g_fire_detect_state = MONITOR_WAITING_FOR_FIRE; 
}

/* *** Function Definitions *** */
void Monitor_Init( void* shared_data_address )
{
	shared_data_type* p_shared_data = (shared_data_type*)shared_data_address;
	
	pthread_mutex_lock(&mutex);
	p_shared_data->fire_detect_state = g_fire_detect_state;
	pthread_mutex_unlock(&mutex);
	Monitor_Send_Notification( "Notice", "Application is starting" );
}

void Monitor_Service( void* shared_data_address )
{
	while (1)
    {
        usleep(MONITOR_SERVICE_RATE_MS * 1000);  // Sleep for 100mS
		
		_Monitor_Service_Fire_Loss_Detection( shared_data_address );
	}
}

void _Monitor_Service_Fire_Loss_Detection( void* shared_data_address )
{
	#define DETECT_STATE_TIME			(10000/MONITOR_SERVICE_RATE_MS)		/* 0 seconds */
	#define NOTIFY_FIRE_LOST_REPEAT_TIME		(5 * 60000/MONITOR_SERVICE_RATE_MS)	/* 5 minutes */
	#define FIRE_DETECTED_TEMP			250.0		/* Fire is detected at 250.0°F */
	#define FIRE_LOST_TEMP				150.0		/* Fire is lost at 100.0°F */
	
	const char state_strings[NBR_MONITOR_STATES][32] = {
		{ "Waiting for fire" },
		{ "Fire detected   " },
		{ "Loss of fire    " },
	};
	static uint32_t timer = 0;
	shared_data_type* p_shared_data = (shared_data_type*)shared_data_address;
	float fire_temp;
	float cabin_temp;
	
	static int print_timer = 0;
	static int send_once = 0;
	int x, y;
	
	pthread_mutex_lock(&mutex);
	fire_temp = p_shared_data->temp_deg_f_fire;
	cabin_temp = p_shared_data->temp_deg_f[0];
	pthread_mutex_unlock(&mutex);
	
	if (++print_timer >= (1000/MONITOR_SERVICE_RATE_MS))
	{
		print_timer = 0;
		getyx(stdscr, y, x);
		move (13, 50);
		if (g_fire_detect_state < NBR_MONITOR_STATES)
			printw( "  State:  %s", state_strings[g_fire_detect_state] );
		else
			printw( "  State:  UNKNOWN STATE" );
		move( y, x );
	}
	
	switch (g_fire_detect_state)
	{
		case MONITOR_WAITING_FOR_FIRE:
			if (fire_temp > FIRE_DETECTED_TEMP)
			{
				if (++timer >= DETECT_STATE_TIME)
				{
					timer = 0;
					Monitor_Send_Notification( "Notice", "Fire detected" );
					g_fire_detect_state = MONITOR_FIRE_DETECTED;
				}
			}
			else
			{
				timer = 0;
			}
			break;
				
		case MONITOR_FIRE_DETECTED:
			if (fire_temp < FIRE_LOST_TEMP)
			{
				if (++timer >= DETECT_STATE_TIME)
				{
					timer = 0;
					Monitor_Send_Notification( "Warning", "Loss of fire has been detected" );
					g_fire_detect_state = MONITOR_FIRE_LOST;
				}
			}
			else
			{
				timer = 0;
			}
			break;
	
		default:
		case MONITOR_FIRE_LOST:
			if (++timer >= NOTIFY_FIRE_LOST_REPEAT_TIME)
			{
				Monitor_Send_Notification( "Warning", "Loss of fire has been detected" );
				
				timer = 0;
			}
			break;
	}
	
	pthread_mutex_lock(&mutex);
	p_shared_data->fire_detect_state = g_fire_detect_state;
	pthread_mutex_unlock(&mutex);
}

void Monitor_Send_Notification( char* pSubject, char* pMsg )
{
#ifdef NOTIFICATION_EMAIL_ADDRESS
	char cmd_buffer[256];
	time_t t = time(NULL);								// Used for obtaining current time
	struct tm tm;										// Used for obtaining current time
	
	t = time(NULL);
	tm = *localtime(&t);
	
	strcpy( cmd_buffer, "echo \"" );
	strcat( cmd_buffer, pMsg );
	strcat( cmd_buffer, "\" | mail -s \"");
	sprintf( cmd_buffer, "%s%2d:%02d - ", cmd_buffer, tm.tm_hour, tm.tm_min );
	strcat( cmd_buffer, pSubject );
	strcat( cmd_buffer, "\" ");
	strcat( cmd_buffer, NOTIFICATION_EMAIL_ADDRESS );
	
	system( cmd_buffer );
#endif
}
