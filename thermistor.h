#define NBR_OF_THERMISTORS			(NBR_ADC_CHANNELS - 1)

void Thermistor_Init( void );
void Thermistor_Service( void *shared_data_address );