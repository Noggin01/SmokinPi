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
	CMD_GET_VERSION = 0,
	CMD_EXIT,
	CMD_SET_CAB_TEMP,
	CMD_GET_CAB_TEMP,
	CMD_SET_PROBE_TARGET_TEMP,
	CMD_GET_PROBE_TARGET_TEMP,
	CMD_GET_PROBE_TEMP,
	CMD_GET_ALL_TEMPS,
	CMD_SET_CHANNEL_NAME,
	CMD_GET_CHANNEL_NAMES,
	CMD_SET_P_GAIN,
	CMD_SET_I_GAIN,
	CMD_SET_I_LIMIT,
	CMD_GET_P_GAIN,
	CMD_GET_I_GAIN,
	CMD_GET_I_LIMIT,

	NBR_OF_CMDS,
	CMD_UNKNOWN=-1
} cmd_id_type;

static const cmd_type cmd_list[NBR_OF_CMDS] = {
	{ "VER",					"Gets the firmware versioninformation\n"	},
	{ "EXIT",					"Closes the program\n"						},
	{ "SET_CABINET_TARGET",		"Set the target cabinet temperature\n" 		},
	{ "GET_CABINET_TARGET",		"Gets the target cabinet temperature\n"		},
	{ "SET_PROBE_TARGET",		"Set the target channel temperature\n"		},
	{ "GET_PROBE_TARGET",		"Gets the target channel temperature\n"		},
	{ "GET_PROBE_TEMP",			"Gets the current probe temperature\n"		},
	{ "GET_ALL_TEMPS",		"Gets the temperature of all sensors\n"		},
	{ "SET_CHANNEL_NAME",		"Set the name of the target channel\n"		},
	{ "GET_CHANNEL_NAMES",		"Gets the names of all probe channels\n"		},
	{ "SET_KP",					"Set proportaional gain\n"					},
	{ "SET_KI",					"Set integral gain\n"						},
	{ "SET_KL",					"Set integral limit\n"						},
	{ "GET_KP",					"Gets the proportional gain\n"				},
	{ "GET_KI",					"Gets the integral gain\n"					},
	{ "GET_KL",					"Gets the integral windup limit\n"			},
};


/* **** Global Variables *** */

// Pointer to the shared data
shared_data_type* gp_shared_data;

// File handlers for the named pipes
static FILE *g_fifo_output;
static FILE *g_fifo_input;
static FILE *g_fifo_errout;

// Buffer for reading files
static char g_in_buffer[1024];		// Buffer for holding incoming data
static char g_out_buffer[1024];		// Buffer for holding outgoing data
static char g_err_buffer[1024];		// Buffer for holding output error

static int g_write_data_bytes = 0;
static int g_write_error_data_bytes = 0;

static void File_Fifo_Process_Cmd( char* cmd_buff );
static void File_Fifo_Signal_Handler( int signalnum );

	// Command processing functions
static void File_Fifo_Set_Cook_Temp( char* tokens );
static void File_Fifo_Set_P_Gain( char* tokens );
static void File_Fifo_Set_I_Gain( char* tokens );
static void File_Fifo_Set_I_Limit( char* tokens );
static void File_Fifo_Get_Probe_Temp( char* tokens );
static void File_Fifo_Get_Cook_Temp( void );
static void File_Fifo_Get_P_Gain( void );
static void File_Fifo_Get_I_Gain( void );
static void File_Fifo_Get_I_Limit( void );
static void File_Fifo_Get_All_Probe_Temps( void );
static void File_Fifo_Get_Probe_Temp( char* tokens );
static void File_Fifo_Set_Channel_Name( char* tokens );
static void File_Fifo_Get_Channel_Names( void );

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
	if (mkfifo(SMPI_INPFIFO, 0777) || mkfifo(SMPI_OUTFIFO, 0777) || mkfifo(SMPI_ERRFIFO, 0777))
		result = -1;
	else
		result = 1;

	return result;
}

void File_Fifo_Service_Input( void *shared_data_address )
{
	int rd_index;
	char ch;

	gp_shared_data = (shared_data_type*)shared_data_address;

	signal(SIGINT, File_Fifo_Signal_Handler);
	signal(SIGPIPE, SIG_IGN);
	
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

	gp_shared_data = (shared_data_type*)shared_data_address;

	signal(SIGINT, File_Fifo_Signal_Handler);
	signal(SIGPIPE, SIG_IGN);

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

void File_Fifo_Respond( char* pResponse )
{
	strcpy(g_out_buffer, pResponse);
	g_write_data_bytes = strlen(pResponse);
}

void File_Fifo_Report_Error( char* pError )
{
	strcpy( g_err_buffer, pError );
	g_write_error_data_bytes = strlen(pError);
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
	char buffer[50];
	
	cmd = strtok(cmd_buff, " \n");	// cmd will now point to the CMD in the input buffer
	
	if (cmd)
	{
		tokens = strtok(NULL, " \n");
	
		for (cmd_nbr = 0; cmd_nbr < NBR_OF_CMDS; cmd_nbr++)
		{
			if (strcasecmp(cmd, cmd_list[cmd_nbr].cmd) == 0)
				break;
		}
	}
	else
		cmd_nbr = -1;

	switch (cmd_nbr)
	{
		default:
		case CMD_UNKNOWN:
			File_Fifo_Respond("-1\n");
			break;

		case CMD_GET_VERSION:
			sprintf(buffer, "1\n%d.%d.%d\n", FIRMWARE_MAJOR, FIRMWARE_MINOR, FIRMWARE_REVISION);
			File_Fifo_Respond(buffer);
			break;
	
		case CMD_EXIT:
		case CMD_SET_PROBE_TARGET_TEMP:
		case CMD_GET_PROBE_TARGET_TEMP:
			File_Fifo_Respond("-1\n");
			File_Fifo_Report_Error("Cmd not implemented\n");
			break;

		case CMD_SET_CHANNEL_NAME:
			File_Fifo_Set_Channel_Name( tokens );
			break;

		case CMD_GET_CHANNEL_NAMES:	
			File_Fifo_Get_Channel_Names();
			break;

		case CMD_SET_CAB_TEMP:
			File_Fifo_Set_Cook_Temp( tokens );
			break;

		case CMD_GET_CAB_TEMP:
			File_Fifo_Get_Cook_Temp();
			break;

		case CMD_GET_PROBE_TEMP:
			File_Fifo_Get_Probe_Temp( tokens );
			break;

		case CMD_GET_ALL_TEMPS:
			File_Fifo_Get_All_Probe_Temps();
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
	
		case CMD_GET_P_GAIN:
			File_Fifo_Get_P_Gain();
			break;
	
		case CMD_GET_I_GAIN:
			File_Fifo_Get_I_Gain();
			break;

		case CMD_GET_I_LIMIT:
			File_Fifo_Get_I_Limit();
			break;
	}
}

/***************************************************************************************************
This function takes a pointer to an ASCII floating point value and sets the cabinet temperature to
that value.
***************************************************************************************************/
static void File_Fifo_Set_Cook_Temp( char* tokens )
{
	float new_temp = 0.0;
	char buffer[50];
	int response;

	if (tokens != NULL)
	{
		sscanf(tokens, "%f", &new_temp);
		App_Set_Cabinet_Setpoint( new_temp );
		response = 0;
	}
	else
	{
		strcpy(buffer, "Error:  Tokens - Set Cook Temp\n");
		File_Fifo_Report_Error(buffer);
		response = -1;
	}

	sprintf(buffer, "%d\n", response);
	File_Fifo_Respond(buffer);
}

/***************************************************************************************************
This function takes a pointer to an ASCII floating point value and sets the P gain to that value.
***************************************************************************************************/
static void File_Fifo_Set_P_Gain( char* tokens )
{
	float new_gain = 0.0;
	char buffer[50];
	int response;

	if (tokens != NULL)
	{
		sscanf(tokens, "%f", &new_gain);
		App_Set_Kp( new_gain );
		response = 0;
	}
	else
	{
		strcpy(buffer, "Error:  Tokens - Set P Gain\n");
		File_Fifo_Report_Error(buffer);
		response = -1;
	}

	sprintf(buffer, "%d\n", response);
	File_Fifo_Respond(buffer);
}

/***************************************************************************************************
This function takes a pointer to an ASCII floating point value and sets the I gain to that value.
***************************************************************************************************/
static void File_Fifo_Set_I_Gain( char* tokens )
{
	float new_gain = 0.0;
	char buffer[50];
	int response;

	if (tokens != NULL)
	{
		sscanf(tokens, "%f", &new_gain);
		App_Set_Ki( new_gain );
		response = 0;
	}
	else
	{
		strcpy(buffer, "Error:  Tokens - Set I Gain\n");
		File_Fifo_Report_Error(buffer);
		response = -1;
	}

	sprintf(buffer, "%d\n", response);
	File_Fifo_Respond(buffer);
}

/***************************************************************************************************
This function takes a pointer to an ASCII floating point value and sets the I windup limit to that 
value.
***************************************************************************************************/
static void File_Fifo_Set_I_Limit( char* tokens )
{
	float new_gain = 0.0;
	char buffer[50];
	int response;

	if (tokens != NULL)
	{
		sscanf(tokens, "%f", &new_gain);
		App_Set_Kl( new_gain );
		response = 0;
	}
	else
	{
		strcpy(buffer, "Error:  Tokens - Set I Limit\n");
		File_Fifo_Report_Error(buffer);
		response = -1;
	}

	sprintf(buffer, "%d\n", response);
	File_Fifo_Respond(buffer);
}

static void File_Fifo_Get_Cook_Temp( void )
{
	char buffer[50];
	float temperature;

	temperature = App_Get_Cabinet_Setpoint();
	sprintf(buffer, "1\n%4.2f\n", temperature);
	File_Fifo_Respond(buffer);
}

static void File_Fifo_Get_P_Gain( void )
{
	char buffer[50];
	float gain;

	gain = App_Get_Kp();
	sprintf(buffer, "1\n%4.2f\n", gain);
	File_Fifo_Respond(buffer);
}

static void File_Fifo_Get_I_Gain( void )
{
	char buffer[50];
	float gain;

	gain = App_Get_Ki();
	sprintf(buffer, "1\n%4.2f\n", gain);
	File_Fifo_Respond(buffer);
}

static void File_Fifo_Get_I_Limit( void )
{
	char buffer[50];
	float limit;

	limit = App_Get_Kl();
	sprintf(buffer, "1\n%4.2f\n", limit);
	File_Fifo_Respond(buffer);
}

static void File_Fifo_Get_Probe_Temp( char* tokens )
{
	char buffer[50];
	float temperature;
	int response;
	int channel;

	if (tokens != NULL)
	{
		sscanf(tokens, "%d", &channel);
		if ((channel >= 0) && (channel < NBR_OF_THERMISTORS))
		{
			pthread_mutex_lock(&mutex);
			temperature = gp_shared_data->temp_deg_f[channel];
			pthread_mutex_unlock(&mutex);
			response = 1;
		}
		else
		{
			response = -1;	
		}
	}
	else
	{
		strcpy(buffer, "Error:  Tokens - Get Probe Temp\n");
		File_Fifo_Report_Error(buffer);
		response = -1;
	}

	if (response == 1)
		sprintf(buffer, "%d\n%4.2f\n", response, temperature);
	else
		sprintf(buffer, "%d\n", response);
	File_Fifo_Respond(buffer);
}

static void File_Fifo_Set_Channel_Name( char* tokens )
{
	int response;
	int channel;
	char buffer[128];
	char name[128];

	if (tokens != NULL)
	{
		sscanf(tokens, "%d", &channel);
		tokens = strtok(NULL, " \n");
		if (strlen(tokens) < (sizeof(name)-1))
			strcpy(name, tokens);
		App_Set_Channel_Name( channel, name );
		response = 0;
	}
	else
	{
		response = -1;
	}

	sprintf(buffer, "%d\n", response);
	File_Fifo_Respond(buffer);
}

static void File_Fifo_Get_Channel_Names( void )
{
	int i;
	char buffer[1024] = { 0 };
	char* name;
	
	strcpy( buffer, "1\n" );
	for (i = 0; i < NBR_OF_THERMISTORS; i++)
	{
		name = App_Get_Channel_Name( i );
		sprintf(buffer, "%s%s,", buffer, name);
	}

	// convert the ',' at the end of the string to a '\n'
	buffer[strlen(buffer)-1] = '\n';
	File_Fifo_Respond(buffer);
}


static void File_Fifo_Get_All_Probe_Temps( void )
{
	char buffer[150];
	int channel;

	strcpy(buffer, "1\n");
	pthread_mutex_lock(&mutex);
	for (channel = 0; channel < NBR_OF_THERMISTORS; channel++)
		sprintf(buffer, "%s%4.2f,", buffer, gp_shared_data->temp_deg_f[channel]);
	pthread_mutex_unlock(&mutex);

	// Change the ',' at the end of the string to a '\n'
	buffer[strlen(buffer)-1] = '\n';

	File_Fifo_Respond(buffer);
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
