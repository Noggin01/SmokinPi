#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pigpio.h>
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
 static int32_t g_spi_handle;

/* *** Accessors *** */

/*******************************************************************************
Configure the wiringPi library to communicate via SPI at 2 MHz, channel 0, in
order to use the TLC1543 11 channel, 10-bit ADC

Prerequisite:  gpioInitialise() must have already been called

*******************************************************************************/
void Tlc1543_Init( void )
{
	uint8_t cmd[2] = { 0, 0 };

	printf("Initializing Tlc1543\n");

	g_spi_handle = spiOpen(SPI_CHANNEL, SPI_SPEED, SPI_MODE);

	// Send the command to start a conversion of channel 0 so that we have
	// valid data when we enter the service routine
	spiWrite(g_spi_handle, (char*)&cmd[0], sizeof(cmd));
}

/*******************************************************************************
Read each of the ADC channels and store the data in the appropriate storage location
*******************************************************************************/
void Tlc1543_Service( void *shared_data_address )
{
	uint8_t write_cmd[2];
	uint8_t read_data[2];
	uint8_t i;
	uint16_t result;
	uint16_t channel_adc_result[NBR_ADC_CHANNELS];
	shared_data_type* p_shared_data = (shared_data_type*)shared_data_address;

	while (1)
	{
		usleep(10000);	// Sleep for 10mS

		// start i at 1 as we've already sent the command to read channel 0
		for (i = 1; i < NBR_ADC_CHANNELS; i++)
		{
			write_cmd[0] = i << 4;
			write_cmd[1] = 0;

			usleep(200);
			spiXfer(g_spi_handle, (char*)&write_cmd, (char*)&read_data, sizeof(read_data));

			// cmd now contains the value returned from the ADC which is the result
			// of the ADC conversion of the previous channel.  Store the
			// value in result so that we can shit it right by 6 bits as it is
			// currently left adjusted
			result = (read_data[0] << 8) + read_data[1];
			result = result >> 6;

			// add the result to the data filter
			channel_adc_result[i-1] = result;
		}

		// at this point, we've sent the command to read the final channel,
		// so now we need to send the command to read channel 0.  This will also
		// let us read the value of the conversion of the latest channel
		write_cmd[0] = 0;
		write_cmd[1] = 0;

		usleep(200);
		spiXfer(g_spi_handle, (char*)&write_cmd, (char*)&read_data, sizeof(read_data));

		result = (read_data[0] << 8) + read_data[1];
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
