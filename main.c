/*******************************************************************************
Description of the Application

Required Libraries
	ncurses	- for better console support
	rt			- for accurate sleeping
	pthread	- for multi-threading
	pigpio	- for servo control
	wiringPi	- for SPI functionality

Four threads
    Main thread - Main loop, spins off the other two threads
    TLC1543 - Thread for reading the ADC data
    Logging - Thread for logging data to the uSD card
    Command Line - Thread for reading commands from the command line interface
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <ncurses.h>
#include "thermistor.h"
#include "tlc1543.h"
#include "servo.h"
#include "app.h"
#include "main.h"
#include "logging.h"
#include "cmd_line.h"

typedef enum 
{
    THREAD_ID_TLC1543 = 0,		// Thread for reading the ADC data
    THREAD_ID_LOGGING,			// Thread for logging data to the uSD card
    THREAD_ID_CMD_LINE,			// Thread for reading data from the cmd line
   
    NBR_THREADS,
} thread_ids;

/* **** Global Data **** */

// Shared data used by multiple threads
shared_data_type shared_data;

// Mutex for obtaining control over the shared data
pthread_mutex_t mutex;

// Signal to end main thread execution
static int g_exit_signal_received = false;

static void Main_Signal_Handler( int signal );

void Main_Init_Hardware( void )
{
	int result;
	
	result = Servo_Init();
	if (result < 0)
	{
		printf("Unable to obtain servo control.\n");
		printf("Run with sudo command?\n");
		printf("Cancel pigpio?  (sudo killall pigpiod)\n");
		_exit(3);
	}

	printf("Fixme %s.%u\n", __FILE__, __LINE__);
	_exit(3);

	Tlc1543_Init();
	Thermistor_Init();
	App_Init();

	Logging_Init();
	Cmd_Line_Init( &shared_data );
}

int main( void )
{
	pthread_t thread[NBR_THREADS];
	int thread_result[NBR_THREADS];
	int i;
	
	signal(SIGINT, Main_Signal_Handler);
	
	printf("Smokin'Pi - Propane Smoker Controller\n");
	printf("Compiled: %s\n", __DATE__);
	
	Main_Init_Hardware();
	
	initscr();					/* Start curses mode */
	cbreak();					/* getch returns each character as it is typed */
	keypad(stdscr, TRUE);	/* support special keys, such as arrows and backspace */
	nodelay(stdscr, TRUE);
	raw();
	
	// Spin off the TLC1543 thread so that the ADC data may be read in the background
	pthread_create(&thread[THREAD_ID_TLC1543], NULL, (void*)&Tlc1543_Service, (void*)&shared_data);
	
	// Spin off the logging thread so that data may be logged to the SD card in the background
	pthread_create(&thread[THREAD_ID_LOGGING], NULL, (void*)&Logging_Service, (void*)&shared_data);
	
	// Spin off the cmd line thread so that the program can accept data via the command line
	pthread_create(&thread[THREAD_ID_CMD_LINE], NULL, (void*)&Cmd_Line_Service, (void*)&shared_data);
	
	while (!g_exit_signal_received)
	{
		App_Service((void*)&shared_data);
		usleep(MAIN_LOOP_TIME_US);
	}
	
	// Join the threads so that we can make sure they exit before the main app does
	for (i = 0; i < NBR_THREADS; i++)
		pthread_join(thread[i], (void*)&thread_result[i]);
	
	Servo_Shutdown();
	endwin();                       	/* End curses mode */
	return 0;
}

/******************************************************************************
When Ctrl+C is pressed to end the process, this function will catch the signal
and alert the main thread that it is time to quit running
******************************************************************************/
static void Main_Signal_Handler( int signalnum )
{
   switch (signalnum)
   {
      case SIGINT:
      	Servo_Shutdown();
	      g_exit_signal_received = true;
         break;
   }
}
