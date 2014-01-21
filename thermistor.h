#define NBR_OF_THERMISTORS			(NBR_ADC_CHANNELS - 1)
#define NBR_OF_COEFFICIENTS			5

typedef enum 
{  
	CDDT_THERMISTOR,
	TAYLOR_THERMISTOR,
	
	NBR_THERMISTOR_TYPES,
} thermistor_types;

void Thermistor_Init( void );
void Thermistor_Service( void *shared_data_address );