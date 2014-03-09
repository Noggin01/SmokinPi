#ifndef __CMD_LINE_H
#define __CMD_LINE_H

#define MAX_CMD_LENGTH									50

typedef enum
{
	CMD_SHOW_MENU=0,
	CMD_EXIT,
	CMD_TOGGLE_DEBUG_FLAG,
	CMD_SHOW_DEBUG_FLAGS,
	CMD_SET_SETPOINT,
	CMD_TOGGLE_AUTO_SERVO_CONTROL,
	CMD_SET_SERVO_PULSE_WIDTH,
	CMD_SET_KP,
	CMD_SET_KI,
	CMD_SET_KD,
	CMD_SET_KL,
	
	NBR_OF_CMDS,
	NO_CMD_AVAILABLE,
} cmd_ids;

typedef struct
{
	int	print_temperature_data:1;
	int	reserved:31;
} debug_flags_type;

void Cmd_Line_Init( void* shared_data_address );
void Cmd_Line_Service( void* shared_data_address );

#endif
