#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pigpio.h>
#include "servo.h"
#include "main.h"
#include "app.h"

void Servo_Init( void )
{
	int32_t result;

	gpioTerminate();
	result = gpioInitialise();
	if (result < 1)
		printf("Failure initializing PiGPIO  - %d", result);
	else
		printf("Initializing Servo\n");
}

void Servo_Service( void *shared_data_address )
{
#define INCREMENT				20
#define DESIRED_TEMPERATURE		250.0

	shared_data_type* p_shared_data = (shared_data_type*)shared_data_address;
	uint16_t local_servo_position = (MAX_POSITION - MIN_POSITION) / 2 + MIN_POSITION;
	float local_cabinet_temperature;
	float temperature_error;
	uint8_t enable_servo = false;

	while (1)
	{
		Delay_Seconds(5);

		pthread_mutex_lock(&mutex);
		local_cabinet_temperature = p_shared_data->temp_deg_f[0];
		pthread_mutex_unlock(&mutex);

		temperature_error = DESIRED_TEMPERATURE - local_cabinet_temperature;

		if (temperature_error > 50.0)
		{
			if (local_servo_position == MAX_POSITION)
				enable_servo = false;
			else
			{
				enable_servo = true;
				local_servo_position = MAX_POSITION;
			}
		}
		else if (temperature_error < -50.0)
		{
			if (local_servo_position == MIN_POSITION_FOR_FIRE)
				enable_servo = false;
			else
			{
				enable_servo = true;
				local_servo_position = MIN_POSITION_FOR_FIRE;
			}
		}
		else if ((temperature_error < 1.0f) && (temperature_error > -1.0f))
		{
			enable_servo = false;
		}
		else if (temperature_error > 0)	// Not hot enough
		{
			if (local_servo_position == MAX_POSITION)
				enable_servo = false;
			else
			{
				enable_servo = true;
				local_servo_position += INCREMENT;
			}
		}
		else if (temperature_error < 0)	// Too hot
		{
			if (local_servo_position == MIN_POSITION_FOR_FIRE)
				enable_servo = false;
			else
			{
				enable_servo = true;
				local_servo_position -= INCREMENT;
				if (local_servo_position < MIN_POSITION_FOR_FIRE)
					local_servo_position = MIN_POSITION_FOR_FIRE;
			}
		}


		if (local_servo_position > MAX_POSITION)
			local_servo_position = MAX_POSITION;
		else if (local_servo_position < MIN_POSITION)
			local_servo_position = MIN_POSITION;

		printf("Servo:  %d %d %4.2f\n", local_servo_position, enable_servo, temperature_error);

		if (!enable_servo)
			gpioServo( 18, 0 );
		else
			gpioServo( 18, local_servo_position );

		pthread_mutex_lock(&mutex);
		p_shared_data->servo_position = local_servo_position;
		pthread_mutex_unlock(&mutex);
	}
}
