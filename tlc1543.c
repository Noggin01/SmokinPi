#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pigpio.h>
#include <pthread.h>
#include "tlc1543.h"
#include "main.h"

/* *** Defined Values *** */
#define SPI_CHANNEL     0
#define SPI_SPEED		2000000
#define SPI_MODE		0

/*************
Spi mode table
Mode POL PHA
 0    0   0
 1    0   1
 2    1   0
 3    1   1
**************/


/* *** Global Variables *** */

/* *** Function Declarations *** */
static int Tlc1543_Transfer( uint8_t* pData, int length );

/* *** Accessors *** */

/*******************************************************************************
Configure the wiringPi library to communicate via SPI at 2 MHz, channel 0, in
order to use the TLC1543 11 channel, 10-bit ADC

Prerequisite:  gpioInitialise() must have already been called

*******************************************************************************/
int Tlc1543_Init( void )
{
	uint8_t data[2] = { 0 };

	printf("Initializing Tlc1543\n");

	return Tlc1543_Transfer( data, sizeof(data) );
}

/*******************************************************************************
This function obtains the mutex for the PiGPIO pipes, writes the specified
command to the pipe, and reads the result.  If the command is valid, the 
dev/pigout pipe will return 0.

Returns	-1 if the file pipes can't be opened
		 0 if the response is non-zero
		 1 if the response is zero
*******************************************************************************/
static int Tlc1543_Transfer( uint8_t* pData, int length )
{
	FILE *pigpio_write;
	FILE *pigpio_read;

	uint8_t* write_data = pData;
	uint8_t* read_data = pData;
	char carriage_return;
	
	int pigpio_handle;
	int pigpio_response = 0;
	int result = 1;
	int i;

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
		fprintf(pigpio_write, "spio %d %d %d\n", SPI_CHANNEL, SPI_SPEED, SPI_MODE);
		fflush(pigpio_write);

		fscanf(pigpio_read, "%d", &pigpio_handle);
		if (pigpio_handle < 0)
		{
			printf("Error retrieving handle: %s.%d\n", __FILE__, __LINE__);
			result = -1;
		}
		else
		{
			fprintf(pigpio_write, "spix %d", pigpio_handle);
			for (i = 0; i < length; i++)
				fprintf(pigpio_write, " 0x%02X", *write_data++);
			fputs("\n", pigpio_write);
			fflush(pigpio_write);

			fscanf(pigpio_read, "%d", &pigpio_response);
			carriage_return = fgetc(pigpio_read);

			if ((pigpio_response == length) && (carriage_return == '\n'))
			{
				for (i = 0; i < length; i++)
					*read_data++ = fgetc(pigpio_read);
			}

			fprintf(pigpio_write, "spic %d\n", pigpio_handle);
			fflush(pigpio_write);
			fscanf(pigpio_read, "%d", &pigpio_response);
			if (pigpio_response != 0)
				result = 0;
		}
	}

	if (pigpio_write)
		fclose(pigpio_write);
	if (pigpio_read)
		fclose(pigpio_read);

	pthread_mutex_unlock(&pigpio_mutex);

	return result;
}

/*******************************************************************************
Read each of the ADC channels and store the data in the appropriate storage location
*******************************************************************************/
void Tlc1543_Service( void *shared_data_address )
{
	uint8_t data[2];
	int i;
	uint16_t result;
	uint16_t channel_adc_result[NBR_ADC_CHANNELS];
	shared_data_type* p_shared_data = (shared_data_type*)shared_data_address;

	while (1)
	{
		usleep(10000);	// Sleep for 10mS

		// start i at 1 as we've already sent the command to read channel 0
		for (i = 1; i < NBR_ADC_CHANNELS; i++)
		{
			data[0] = i << 4;
			data[1] = 0;

			usleep(200);
			Tlc1543_Transfer(data, sizeof(data));

			// cmd now contains the value returned from the ADC which is the result
			// of the ADC conversion of the previous channel.  Store the
			// value in result so that we can shit it right by 6 bits as it is
			// currently left adjusted
			result = (data[0] << 8) + data[1];
			result = result >> 6;

			// add the result to the data filter
			channel_adc_result[i-1] = result;
		}

		// at this point, we've sent the command to read the final channel,
		// so now we need to send the command to read channel 0.  This will also
		// let us read the value of the conversion of the latest channel
		data[0] = 0;
		data[1] = 0;

		usleep(200);
		Tlc1543_Transfer(data, sizeof(data));

		result = (data[0] << 8) + data[1];
		result = result >> 6;

		channel_adc_result[i-1] = result;

		pthread_mutex_lock(&mutex);
		memcpy( (uint8_t*)p_shared_data->adc_results,
				(uint8_t*)channel_adc_result,
				sizeof(p_shared_data->adc_results));
		p_shared_data->adc_data_available = true;
		pthread_mutex_unlock(&mutex);
	}
}
