/*******************************************************************************
Description of the Application

Required Libraries
	ncurses	- for better console support
	rt			- for accurate sleeping
	pthread	- for multi-threading
	pigpio	- for servo control
	wiringPi	- for SPI functionality

Five threads
    Main thread - Main loop, spins off the other two threads
    TLC1543 - Thread for reading the ADC data
    Logging - Thread for logging data to the uSD card
    Command Line - Thread for reading commands from the command line interface
    Ethernet Comms - Thread for listening for system commands over ethernt
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
#include "rev_history.h"
//#include "file_fifo.h"
#include "eth_comms.h"			// For Ethernet communications
#include "monitor.h"

typedef enum 
{
	THREAD_ID_TLC1543 = 0,		// Thread for reading the ADC data
	THREAD_ID_LOGGING,			// Thread for logging data to the uSD card
	THREAD_ID_CMD_LINE,			// Thread for reading data from the cmd line
	THREAD_ID_ETHERNET,			// Thread for communication via Ethernet
	THREAD_ID_MONITOR,			// Thread for monitoring the system and sending notifications
//	THREAD_ID_FILE_FIFO_IN,		// Thread for reading from external programs
//	THREAD_ID_FILE_FIFO_OUT,	// Thread for writing to external programs

	NBR_THREADS,
} thread_ids;

/* **** Global Data **** */

// Shared data used by multiple threads
shared_data_type shared_data;

// Mutex for obtaining control over the shared data
pthread_mutex_t mutex;

// Mutex for obtaining control over the PiGPIO files
pthread_mutex_t pigpio_mutex;

// Signal to end main thread execution
static int g_exit_signal_received = false;

static void Main_Signal_Handler( int signal );

void Main_Init_Hardware( void )
{
	if (Servo_Init() < 0)
	{
		printf("Unable to obtain servo control.\nIs pigpiod running?\n");
		_exit(3);
	}

//	if (File_Fifo_Init() <= 0)
//	{
//		printf("Error making pipes\n");
//		_exit(3);
//	}

	if (Eth_Comms_Init(&shared_data) < 0)
	{
		printf("Error iniializing Ethernet comms\n");
		_exit(3);
	}

	Tlc1543_Init();
	Thermistor_Init();
	App_Init( &shared_data );
	Logging_Init();
	Cmd_Line_Init( &shared_data );
	Monitor_Init( &shared_data );
	sleep(1);
}

int main( void )
{
	pthread_t thread[NBR_THREADS];
	int thread_result[NBR_THREADS];
	int i;
	
	signal(SIGINT, Main_Signal_Handler);
	
	printf("Smokin'Pi - Propane Smoker Controller\n");
	printf("Compiled: %s\n", __DATE__);
	printf("Version: %u.%u.%u\n", FIRMWARE_MAJOR, FIRMWARE_MINOR, FIRMWARE_REVISION);
	
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

	// Spin off the ethernet thread
	pthread_create(&thread[THREAD_ID_ETHERNET], NULL, (void*)&Eth_Comms_Service, (void*)&shared_data);
	
	// Spin off the monitor thread
	pthread_create(&thread[THREAD_ID_MONITOR], NULL, (void*)&Monitor_Service, (void*)&shared_data);
	
	// Spin off the file fifo threads so that external programs can communicate via pipes
//	pthread_create(&thread[THREAD_ID_FILE_FIFO_IN], NULL, (void*)&File_Fifo_Service_Input, (void*)&shared_data);
//	pthread_create(&thread[THREAD_ID_FILE_FIFO_OUT], NULL, (void*)&File_Fifo_Service_Output, (void*)&shared_data);
	
	while (!g_exit_signal_received)
	{
		App_Service();
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
