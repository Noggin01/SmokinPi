/********************************************************************************************
Description of the Application

Three threads
    Main thread - Main loop, spins off the other two threads
    TLC1543 - Thread for reading the ADC data
    Logging - Thread for logging data to the uSD card
********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "thermistor.h"
#include "tlc1543.h"
#include "servo.h"
#include "app.h"
#include "main.h"
#include "logging.h"

typedef enum 
{
    THREAD_ID_TLC1543 = 0,          // Thread for reading the ADC data
    THREAD_ID_LOGGING,              // Thread for logging data to the uSD card
//    THREAD_ID_THERMISTOR,
//    THREAD_ID_MAIN_APPLICATION,
//    THREAD_ID_SERVO,
   
    NBR_THREADS,
} thread_ids;

/* **** Global Data **** */

// Shared data used by multiple threads
shared_data_type shared_data;

// Mutex for obtaining control over the shared data
pthread_mutex_t  mutex;

// Signal to end main thread execution
static g_exit_signal_received = false;

static void Main_Signal_Handler( int signal );

void Main_Init_Hardware( void )
{
	Tlc1543_Init();
	Thermistor_Init();
	Servo_Init();
    App_Init();
    Logging_Init();
}

int main( void )
{
	pthread_t thread[NBR_THREADS];
	int32_t thread_result[NBR_THREADS];
    uint8_t i;

    signal(SIGINT, Main_Signal_Handler);

    printf("Smokin'Pi - Propane Smoker Controller\n");
    printf("Compiled: %s\n", __DATE__);

    Main_Init_Hardware();
    
    // Spin off the TLC1543 thread so that the ADC data may be read in the background
    pthread_create(&thread[THREAD_ID_TLC1543], NULL, (void*)&Tlc1543_Service, (void*)&shared_data);

    // Spin off the logging thread so that data may be logged to the SD card in the background
    pthread_create(&thread[THREAD_ID_LOGGING], NULL, (void*)&Logging_Service, (void*)&shared_data);
  
    while (!g_exit_signal_received)
    {
        App_Service();  // Execute App_Service approximately every 5 mS
        usleep(5000);
    }

    for (i = 0; i < NBR_THREADS; i++)
        pthread_join(thread[i], (void*)&thread_result[i]);

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
         g_exit_signal_received = true;
         break;
   }
}
