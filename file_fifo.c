/***************************************************************************************************
File FIFO

This thread opens files to read input commands, write output responses, and write error outputs for
control via external applications.
***************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "thermistor.h"
#include "tlc1543.h"
#include "servo.h"
#include "app.h"
#include "main.h"
#include "rev_history.h"
#include "file_fifo.h"

typedef struct 
{
	char* cmd;
	char* description;	
} cmd_type;

/* **** Global Data **** */

/* *** Global Variables *** */
typedef enum 
{
	CMD_SHOW_MENU = 0,
	CMD_EXIT,
	CMD_SET_COOK_TEMP,
	CMD_SET_P_GAIN,
	CMD_SET_I_GAIN,
	CMD_SET_I_LIMIT,

	NBR_OF_CMDS
} cmd_id_type;

static const cmd_type cmd_list[NBR_OF_CMDS] = {
	{ "HELP",			"Shows this menu\n"							},
	{ "EXIT",			"Closes the program\n"						},
	{ "SETTEMP=",		"Set the target cabinet temperature\n" 		},
	{ "KP=",			"Set proportaional gain\n"					},
	{ "KI=",			"Set integral gain\n"						},
	{ "KL=",			"Set integral limit\n"						},
};


static cmd_id_type Fifo_Get_Command_Id( void );
static void Fifo_Line_Process( void );

/* **** Global Variables *** */

// Signal to end thread execution
static int g_exit_signal_received = false;

// File handlers for the named pipes
static FILE *g_fifo_output;
static FILE *g_fifo_input;
static FILE *g_fifo_errout;

// Buffer for reading files
static char g_in_buffer[1024];
static char g_out_buffer[1024];
static char g_err_buffer[1024];

static int g_write_data_bytes = 0;

static void File_Fifo_Signal_Handler( int signal );
static void File_Fifo_Process_Cmd( char* cmd_buff );

	// Command processing functions
static void File_Fifo_Set_Cook_Temp( char* tokens );
static void File_Fifo_Set_P_Gain( char* tokens );
static void File_Fifo_Set_I_Gain( char* tokens );
static void File_Fifo_Set_I_Limit( char* tokens );

/***************************************************************************************************
Sets up the named pipes for receiving commands, sending responses, and reporting errors.

Returns -1 on error opening files
		 1 on successful opening of files
***************************************************************************************************/
int File_Fifo_Init( void )
{
	int result = 0;

	// attempt to remove the files in case they are already present
	remove(SMPI_INPFIFO);
	remove(SMPI_OUTFIFO);
	remove(SMPI_ERRFIFO);

	// Create the fifos
	if (mkfifo(SMPI_INPFIFO, 0666) || mkfifo(SMPI_OUTFIFO, 0666) || mkfifo(SMPI_ERRFIFO, 0666))
		result = -1;
	else
		result = 1;

	return result;
}

void File_Fifo_Service_Input( void *shared_data_address )
{
	int rd_index;
	char ch;

	while (1)
	{
		g_fifo_input = fopen(SMPI_INPFIFO, "r");

		rd_index = 0;
		g_in_buffer[0] = 0;
		do {
			ch = fgetc(g_fifo_input);
			g_in_buffer[rd_index++] = ch;
		} while ((ch != '\n') && (rd_index < sizeof(g_in_buffer)));

		File_Fifo_Process_Cmd( g_in_buffer );
		fclose(g_fifo_input);
	}
}

void File_Fifo_Service_Output( void *shared_data_address )
{
	int i;

	while (1)
	{
		g_fifo_output = fopen(SMPI_OUTFIFO, "w");

		while (g_fifo_output)
		{
			if (g_write_data_bytes > 0)
			{
				i = 0;
				while (i < g_write_data_bytes)
					fputc(g_out_buffer[i++], g_fifo_output);
				fflush(g_fifo_output);
				g_write_data_bytes = 0;
			}
			else
				usleep(1000);
		}
	}
}

/***************************************************************************************************
This function accepts a command buffer containing ascii data.  It tokenizes the string then
identifies the command passed to it.
***************************************************************************************************/
static void File_Fifo_Process_Cmd( char* cmd_buff )
{
	char* cmd;
	char* tokens;
	int cmd_nbr;

	cmd = strtok(cmd_buff, " ");	// cmd will now point to the CMD in the input buffer
	tokens = strtok(NULL, " ");
	for (cmd_nbr = 0; cmd_nbr < NBR_OF_CMDS; cmd_nbr++)
	{
		if (strcasecmp(cmd, cmd_list[cmd_nbr].cmd) == 0)
			break;
	}

	switch (cmd_nbr)
	{
		case CMD_SHOW_MENU:
			strcpy(g_out_buffer, "Cmd not implemented\n");
			g_write_data_bytes = strlen(g_out_buffer);
			break;
		
		case CMD_EXIT:
			strcpy(g_out_buffer, "Cmd not implemented\n");
			g_write_data_bytes = strlen(g_out_buffer);
			break;
		
		case CMD_SET_COOK_TEMP:
			File_Fifo_Set_Cook_Temp( tokens );
			break;
		
		case CMD_SET_P_GAIN:
			File_Fifo_Set_P_Gain( tokens );
			break;
		
		case CMD_SET_I_GAIN:
			File_Fifo_Set_I_Gain( tokens );
			break;
		
		case CMD_SET_I_LIMIT:
			File_Fifo_Set_I_Limit( tokens );
			break;
	}
}

static void File_Fifo_Set_Cook_Temp( char* tokens )
{
	float new_temp = 0.0;

	if (tokens != NULL)
	{
		sscanf(tokens, "%f", &new_temp);
		App_Force_Cabinet_Temperature( new_temp );
		strcpy(g_out_buffer, "New temp accepted\n");
		g_write_data_bytes = strlen(g_out_buffer);
	}
	else
	{
		strcpy(g_out_buffer, "Err tokens\n");
		g_write_data_bytes = strlen(g_out_buffer);
	}
}

static void File_Fifo_Set_P_Gain( char* tokens )
{

}
static void File_Fifo_Set_I_Gain( char* tokens )
{

}
static void File_Fifo_Set_I_Limit( char* tokens )
{

}



/***************************************************************************************************
When Ctrl+C is pressed to end the process, this function will catch the signal and close the file 
stream before exiting the thread
***************************************************************************************************/
static void File_Fifo_Signal_Handler( int signalnum )
{
	switch (signalnum)
	{
		case SIGINT:
			if (g_fifo_input)
				fclose(g_fifo_input);
			if (g_fifo_output)
				fclose(g_fifo_output);
			if (g_fifo_errout)
				fclose(g_fifo_errout);
			exit(signalnum);
			break;
	}
}

/* *** End of File *** */