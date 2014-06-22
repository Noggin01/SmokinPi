#ifndef __CMD_LINE_H
#define __CMD_LINE_H

#define MAX_CMD_LENGTH									50

typedef struct
{
	int	print_temperature_data:1;
	int	reserved:31;
} debug_flags_type;

void Cmd_Line_Init( void* shared_data_address );
void Cmd_Line_Service( void* shared_data_address );

#endif
