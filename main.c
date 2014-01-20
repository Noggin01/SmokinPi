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
    THREAD_ID_TLC1543 = 0,
    THREAD_ID_THERMISTOR,
    THREAD_ID_MAIN_APPLICATION,
    THREAD_ID_SERVO,
    THREAD_ID_LOGGING,
   
    NBR_THREADS,
} thread_ids;

/* **** Global Data **** */

// Shared data used by multiple threads
shared_data_type shared_data;

// Mutex for obtaining control over the shared data
pthread_mutex_t  mutex;

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

    printf("Smokin'Pi - Propane Smoker Controller\n");
    printf("Compiled: %s\n", __DATE__);

    Main_Init_Hardware();
    
    pthread_create(&thread[THREAD_ID_TLC1543], NULL, (void*)&Tlc1543_Service, (void*)&shared_data);
    pthread_create(&thread[THREAD_ID_THERMISTOR], NULL, (void*)&Thermistor_Service, (void*)&shared_data);
    pthread_create(&thread[THREAD_ID_MAIN_APPLICATION], NULL, (void*)&App_Service, (void*)&shared_data);
    pthread_create(&thread[THREAD_ID_LOGGING], NULL, (void*)&Logging_Service, (void*)&shared_data);
    pthread_create(&thread[THREAD_ID_SERVO], NULL, (void*)&Servo_Service, (void*)&shared_data);
 
    for (i = 0; i < NBR_THREADS; i++)
        pthread_join(thread[i], (void*)&thread_result[i]);

    return 0;
}
