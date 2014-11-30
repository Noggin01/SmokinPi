

#ifndef __COMMS_H
#define __COMMS_H

#include "app.h"			// For MAX_NAME_LENGTH
#include "tlc1543.h"		// For NBR_ADC_CHANNELS

#define SYNC_PATTERN		0xACED

typedef enum
{
	CMD_GET_VERSION = 0,
	CMD_SET_TEMPERATURE_SETPOINT,
	CMD_SET_KP,
	CMD_SET_KI,
	CMD_SET_KL,
	CMD_SET_CHANNEL_NAME,
	CMD_GET_STATUS,
	
	CMD_VERSION_RESPONSE,
	CMD_STATUS_RESPONSE,
} message_id_type;

typedef struct
{
	unsigned short sync_pattern;
	unsigned short length;
	unsigned short cmd_id;
} msg_header_type;

typedef struct
{
	msg_header_type header;
} msg_get_ver, msg_get_status;

typedef struct
{
	msg_header_type header;
	float value;
} msg_set_setpoint, msg_set_kp, msg_set_ki, msg_set_kl;

typedef struct
{
	msg_header_type header;
	unsigned char name[MAX_NAME_LENGTH];		// Null terminated character string
} msg_set_chan_name;

typedef struct
{
	msg_header_type header;
	unsigned int major_ver;
	unsigned int minor_ver;
	unsigned int revision;
} msg_ver_response;

typedef struct
{
	msg_header_type header;
	float temperature_setpoint;
	float channel_temperatures[NBR_ADC_CHANNELS];
	unsigned char channel_names[NBR_ADC_CHANNELS][MAX_NAME_LENGTH];
	float kp;
	float ki;
	float kl;
	float i_windup;
	float pid_output;
	unsigned short servo_position;
	unsigned short servo_physical_min;
	unsigned short servo_physical_max;
	unsigned short servo_control_min;
	unsigned short servo_control_max;
} msg_status_response;

int Eth_Comms_Init( void );
void Eth_Comms_Service( void* shared_data_address );

#endif