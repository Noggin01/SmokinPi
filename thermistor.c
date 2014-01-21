#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include "main.h"
#include "thermistor.h"
#include "tlc1543.h"
#include "app.h"

/* *** Constants *** */
#define NBR_TABLE_ENTRIES	(sizeof(conversion_table)/sizeof(temp_lookup_entry))

/* *** Types *** */
typedef struct
{
	uint16_t adc;
	float deg_f;
} temp_lookup_entry;

/* *** Global Variables *** */
static temp_lookup_entry conversion_table[] =
{
	{ 1024, -70}, { 1020, -58 },   { 1019, -50.8 }, { 1018, -45.4 }, { 1017, -40 },   { 1016, -36.4 },
	{ 1015, -32.8 }, { 1014, -29.2 }, { 1013, -25.6 }, { 1012, -22 },   { 1011, -20.2 },
	{ 1010, -18.4 }, { 1009, -14.8 }, { 1008, -13 },   { 1007, -11.2 }, { 1006, -9.4 },
	{ 1005, -7.6 },  { 1004, -5.8 },  { 1003, -4 },    { 1002, -2.2 },  { 1000, -0.399999999999999 },
	{ 999, 1.4 }, { 998, 3.2 }, { 996, 5 }, { 995, 6.8 }, { 993, 8.6 }, { 992, 10.4 }, { 990, 12.2 },
	{ 988, 14 }, { 986, 15.8 }, { 984, 17.6 }, { 982, 19.4 }, { 980, 21.2 }, { 978, 23 },
	{ 976, 24.8 }, { 973, 26.6 }, { 971, 28.4 }, { 968, 30.2 }, { 965, 32 }, { 962, 33.8 },
	{ 959, 35.6 }, { 956, 37.4 }, { 953, 39.2 }, { 950, 41 }, { 946, 42.8 }, { 943, 44.6 },
	{ 939, 46.4 }, { 935, 48.2 }, { 931, 50 }, { 927, 51.8 }, { 923, 53.6 }, { 918, 55.4 },
	{ 914, 57.2 }, { 909, 59 }, { 904, 60.8 }, { 899, 62.6 }, { 894, 64.4 }, { 889, 66.2 },
	{ 883, 68 }, { 878, 69.8 }, { 872, 71.6 }, { 866, 73.4 }, { 860, 75.2 }, { 854, 77 },
	{ 847, 78.8 }, { 841, 80.6 }, { 834, 82.4 }, { 827, 84.2 }, { 820, 86 }, { 813, 87.8 },
	{ 806, 89.6 }, { 798, 91.4 }, { 791, 93.2 }, { 783, 95 }, { 775, 96.8 }, { 767, 98.6 },
	{ 759, 100.4 }, { 751, 102.2 }, { 743, 104 }, { 734, 105.8 }, { 726, 107.6 }, { 717, 109.4 },
	{ 708, 111.2 }, { 700, 113 }, { 691, 114.8 }, { 682, 116.6 }, { 673, 118.4 }, { 663, 120.2 },
	{ 654, 122 }, { 645, 123.8 }, { 636, 125.6 }, { 626, 127.4 }, { 617, 129.2 }, { 608, 131 },
	{ 598, 132.8 }, { 589, 134.6 }, { 579, 136.4 }, { 570, 138.2 }, { 561, 140 }, { 551, 141.8 },
	{ 542, 143.6 }, { 533, 145.4 }, { 523, 147.2 }, { 514, 149 }, { 505, 150.8 }, { 496, 152.6 },
	{ 487, 154.4 }, { 477, 156.2 }, { 469, 158 }, { 460, 159.8 }, { 451, 161.6 }, { 442, 163.4 },
	{ 433, 165.2 }, { 425, 167 }, { 416, 168.8 }, { 408, 170.6 }, { 400, 172.4 }, { 391, 174.2 },
	{ 383, 176 }, { 375, 177.8 }, { 367, 179.6 }, { 360, 181.4 }, { 352, 183.2 }, { 345, 185 },
	{ 337, 186.8 }, { 330, 188.6 }, { 323, 190.4 }, { 316, 192.2 }, { 309, 194 }, { 302, 195.8 },
	{ 295, 197.6 }, { 289, 199.4 }, { 282, 201.2 }, { 276, 203 }, { 270, 204.8 }, { 263, 206.6 },
	{ 257, 208.4 }, { 252, 210.2 }, { 246, 212 }, { 240, 213.8 }, { 235, 215.6 }, { 229, 217.4 },
	{ 224, 219.2 }, { 219, 221 }, { 214, 222.8 }, { 209, 224.6 }, { 204, 226.4 }, { 199, 228.2 },
	{ 195, 230 }, { 190, 231.8 }, { 186, 233.6 }, { 181, 235.4 }, { 177, 237.2 }, { 173, 239 },
	{ 169, 240.8 }, { 165, 242.6 }, { 161, 244.4 }, { 157, 246.2 }, { 154, 248 }, { 150, 249.8 },
	{ 147, 251.6 }, { 143, 253.4 }, { 140, 255.2 }, { 137, 257 }, { 133, 258.8 }, { 130, 260.6 },
	{ 127, 262.4 }, { 124, 264.2 }, { 122, 266 }, { 119, 267.8 }, { 116, 269.6 }, { 113, 271.4 },
	{ 111, 273.2 }, { 108, 275 }, { 106, 276.8 }, { 103, 278.6 }, { 101, 280.4 }, { 99, 282.2 },
	{ 96, 284 }, { 94, 285.8 }, { 92, 287.6 },{ 90, 289.4 }, { 88, 291.2 }, { 86, 293 },
	{ 84, 294.8 }, { 82, 296.6 }, { 80, 298.4 }, { 78, 300.2 }, { 77, 302 }, { 75, 303.8 },
	{ 73, 305.6 }, { 72, 307.4 }, { 70, 309.2 }, { 69, 311 }, { 67, 312.8 }, { 66, 314.6 },
	{ 64, 316.4 }, { 63, 318.2 }, { 61, 320 }, { 60, 321.8 }, { 59, 323.6 }, { 57, 325.4 },
	{ 56, 327.2 }, { 55, 329 }, { 54, 330.8 }, { 53, 332.6 }, { 52, 334.4 }, { 50, 336.2 },
	{ 49, 338 }, { 48, 339.8 }, { 47, 341.6 }, { 46, 343.4 }, { 45, 345.2 }, { 44, 348.8 },
	{ 43, 350.6 }, { 42, 352.4 }, { 41, 354.2 }, { 40, 356 }, { 39, 357.8 }, { 38, 361.4 },
	{ 37, 363.2 }, { 36, 365 }, { 35, 368.6 }, { 34, 370.4 }, { 33, 374 }, { 32, 375.8 },
	{ 31, 379.4 }, { 30, 383 }, { 29, 384.8 }, { 28, 386.6 },
};

uint8_t thermistor_data_initialized = false;


/* *** Function Declarations *** */
float Thermistor_Convert_Adc_To_Deg_x10( uint16_t adc, uint8_t i );

void Thermistor_Init( void )
{
	printf("Initializing Thermistor Data\n");
}

void Thermistor_Service( void *shared_data_address )
{
	uint8_t i;
	uint16_t raw_adc_data;
	float degf;
	uint16_t local_adc_results[NBR_ADC_CHANNELS];
	float local_temperature_deg_f[NBR_OF_THERMISTORS];
	uint16_t local_servo_position;

	shared_data_type* p_shared_data = (shared_data_type*)shared_data_address;

	while (1)
	{
		Delay_Seconds(1);

		pthread_mutex_lock(&mutex);
		memcpy( (uint8_t*)local_adc_results,
				(uint8_t*)p_shared_data->adc_results,
				sizeof(local_adc_results));
		p_shared_data->adc_data_available = false;
		local_servo_position = p_shared_data->servo_position;
		pthread_mutex_unlock(&mutex);

		for (i = 0; i < NBR_OF_THERMISTORS; i++)
		{
			raw_adc_data = local_adc_results[i];
			degf = Thermistor_Convert_Adc_To_Deg_x10( raw_adc_data, i );
			if (thermistor_data_initialized)
			{
				local_temperature_deg_f[i] *= 9;
				local_temperature_deg_f[i] += degf;
				local_temperature_deg_f[i] /= 10;
			}
			else
				local_temperature_deg_f[i] = degf;

			printf("%2u:%4.2f ", i, local_temperature_deg_f[i]);
		}
		printf( "%4u\n", local_servo_position );

		pthread_mutex_lock(&mutex);
		memcpy( (uint8_t*)p_shared_data->temp_deg_f,
				(uint8_t*)local_temperature_deg_f,
				sizeof(p_shared_data->temp_deg_f));
		pthread_mutex_unlock(&mutex);

		thermistor_data_initialized = true;
	}
}

float Thermistor_Convert_Adc_To_Deg_x10( uint16_t adc, uint8_t flag )
{
	uint16_t i;
	float temperature_above;
	float temperature_below;
	float temperature_difference;
	float calculated_temperature;
	float degrees_per_count;
	uint16_t counts_difference;
	uint16_t counts_above_temperature_below;

	for (i = 0; i < NBR_TABLE_ENTRIES; i++)
		if (adc >= conversion_table[i].adc)
			break;

	temperature_below = conversion_table[i-1].deg_f;
	temperature_above = conversion_table[i].deg_f;

	counts_difference = conversion_table[i-1].adc - conversion_table[i].adc;
	temperature_difference = temperature_above - temperature_below;

	degrees_per_count = temperature_difference / (float)counts_difference;
	counts_above_temperature_below = conversion_table[i-1].adc - adc;

	calculated_temperature = temperature_below + (counts_above_temperature_below * degrees_per_count);

	// if (flag == 0)
	// {
	// 	printf("\n");
	// 	printf("           i - %u\n", i );
	// 	printf("         ADC - %u\n", adc );
	// 	printf("  Temp Below - %f\n", temperature_below );
	// 	printf("  Temp Above - %f\n", temperature_above );
	// 	printf(" Counts Diff - %u\n", counts_difference );
	// 	printf("   Temp Diff - %f\n", temperature_difference );
	// 	printf("   Deg / Cnt - %f\n", degrees_per_count );
	// 	printf("Counts Above - %d\n", counts_above_temperature_below );
	// 	printf("      Result - %f\n", calculated_temperature );
	// }

    return calculated_temperature;
}

