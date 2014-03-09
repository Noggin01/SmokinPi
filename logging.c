#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "main.h"
#include "logging.h"
#include "thermistor.h"

/* *** Constants *** */

/* *** Types *** */

/* *** Global Variables *** */
static FILE *write_ptr;               // File pointer for writing data

/* *** Function Declarations *** */
static void Logging_Signal_Handler( int signal );

/********************************************************
Open a file for logging data
********************************************************/
void Logging_Init( void )
{
   char filename[50];
   time_t t = time(NULL);
   struct tm tm;

   tm = *localtime(&t);
   sprintf(filename, "Log-%d-%d-%d.csv", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
   write_ptr = fopen(filename,"a");   // Open the log file for appending
}

/*****************************************************************************
The sleep() function stalls the thread execution for the specified number of
seconds; however, it can be interrupted.  In the case that the function is
interrupted, it returns the remaining number of seconds of sleep that did not
occur.  Continue sleeping until sleep() returns 0, which indicates that the
full sleep has occurred.

!!! If sleep is interrupted after less than 500mS, what does it return?
!!! If sleep is interrupted after more than 500mS, what does it return?
*****************************************************************************/
int Logging_Full_Sleep( int seconds )
{
   while (seconds > 0)
      seconds = sleep(seconds);

   return seconds;
}

/******************************************************************************
This service routine logs data to a comma separated file suitable for reading
by spreadsheet programs.  A file is opened for appending, data is written to
the file, then the file is closed.

Log Contents include temperature data from half of the thermistors, ADC
measurements from the remaining half of the analog channels, and the propane
valve's servo position.
******************************************************************************/
void Logging_Service( void *shared_data_address )
{
   float local_temperature_deg_f[NBR_OF_THERMISTORS];   // Local copy of the temperature data
   uint16_t local_adc_results[NBR_ADC_CHANNELS];      // Local copy of the shared ADC data
   uint16_t local_servo_position;                  // Local copy of the servo position
   time_t t = time(NULL);                        // Used for obtaining current time
   struct tm tm;                              // Used for obtaining current time
   uint8_t i;

      // Pointer for accessing shared data
   shared_data_type* p_shared_data = (shared_data_type*)shared_data_address;

   signal(SIGINT, Logging_Signal_Handler);

   sleep(5);      // Sleep 5 seconds before logging any data

   while (1)
   {
      // Obtain a lock to the shared data, create a local
      // copy of the data, then release the lock
		pthread_mutex_lock(&mutex);
		memcpy( (uint8_t*)local_temperature_deg_f, (uint8_t*)p_shared_data->temp_deg_f, sizeof(local_temperature_deg_f));
		memcpy( (uint8_t*)local_adc_results, (uint8_t*)p_shared_data->adc_results, sizeof(local_adc_results));
		local_servo_position = p_shared_data->servo_position;
		pthread_mutex_unlock(&mutex);

      // Get the current time, build the timestamp and write it to the file.  Might
      // as well write the servo position as well.  Good a time as any.
      t = time(NULL);
      tm = *localtime(&t);
      fprintf(write_ptr, "%d-%d-%d %2d:%02d:%02d,%u", tm.tm_year + 1900, tm.tm_mon + 1,
         tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, local_servo_position);

      // Write the temperature for each of the probes on the first 5 channels
      for (i = 0; i < NBR_OF_THERMISTORS; i++)
         fprintf(write_ptr, ",%4.2f", local_temperature_deg_f[i]);

      fwrite("\n", 1, 1, write_ptr);      // Append a new line to the file
      fflush(write_ptr);						// Force the write to disk
      
      Logging_Full_Sleep(15);      // Delay 15 seconds before writing the next log entry
   }
}

/******************************************************************************
When Ctrl+C is pressed to end the process, this function will catch the signal
and close the file stream before exiting the thread
******************************************************************************/
static void Logging_Signal_Handler( int signalnum )
{
   switch (signalnum)
   {
      case SIGINT:
         fclose(write_ptr);
         exit(signalnum);
         break;
   }
}

/* *** End of File *** */