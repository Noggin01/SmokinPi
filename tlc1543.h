
#include <stdint.h>

#define NBR_ADC_CHANNELS	11

void Tlc1543_Init( void );
void Tlc1543_Service( void* shared_data_address );

uint16_t Tlc1543_Get_Channel_Value( uint8_t chan );