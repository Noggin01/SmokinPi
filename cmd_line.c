#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include "cmd_line.h"
#include "main.h"
#include "app.h"

/* *** Data Types *** */
typedef struct 
{
	char* cmd;
	char* description;	
} cmd_type;

/* *** Defined Values *** */
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

/* *** Global Variables *** */
static const cmd_type cmd_list[NBR_OF_CMDS] = {
	{ "HELP",			"Shows this menu\n"									},
	{ "EXIT",			"Closes the program\n"								},
	{ "DEBUG=",			"Toggle debug flag\n"								},
	{ "DEBUG?",			"Lists available debug flags\n"					},
	{ "SETTEMP=",		"Set the target cabinet temperature\n" 		},
	{ "SERVO!",			"Toggles PID control of the servo\n"			},
	{ "SERVO=",			"Sets the pulse width of the servo\n"			},
	{ "KP=",				"Set proportaional gain\n"							},
	{ "KI=",				"Set integral gain\n"								},
	{ "KD=",				"Set derivative gain\n"								},
	{ "KL=",				"Set integral limit\n"								},
};

char g_cmd[MAX_CMD_LENGTH];

/* *** Function Prototypes *** */
static void Cmd_Line_Get_Command( void );
static void Cmd_Line_Process( void );

// Command Processors
void Cmd_Line_Print_Menu( void );

/* *** Accessors *** */

/*******************************************************************************
Prepares the program for control via the command line
*******************************************************************************/
void Cmd_Line_Init( void* shared_data_address )
{
//	pthread_mutex_lock(&cmd_line_mutex);
//	cmd_line_shared_data.cmd_number = NO_CMD_AVAILABLE;
//	pthread_mutex_unlock(&cmd_line_mutex);
}

/*******************************************************************************
Reads data from the cmd line and processes the data
*******************************************************************************/
void Cmd_Line_Service( void *shared_data_address )
{
	while (1)
	{
		usleep(25000);					// Sleep 25 mS
		Cmd_Line_Get_Command();
		Cmd_Line_Process();
	}
}

static void Cmd_Line_Get_Command( void )
{
	static int index = 0;
	int ch;
	
	move( 3, 0 );
	printw("                         ");
	move( 3, 0 );
	
	while (ch != '\n')
	{
		usleep(1000);		// Sleep 1mS
		ch = getch();
	
		if (ch != ERR)
		{
			switch (ch)
			{
				case KEY_BACKSPACE:
				case KEY_LEFT:
					g_cmd[index] = 0;
					index--;
					printf("\b \b");
					break;
					
				case '\n':
					break;
					
				default:
					if (index < (sizeof(g_cmd) - 1))
						g_cmd[index++] = (char)ch;
					g_cmd[index] = 0;
					break;		
			}
		}
	}
	
	if ((char)ch == '\n')
		index = 0;
}

static void Cmd_Line_Process( void )
{
	int cmd_nbr;
	int cmd_found = false;
	char* p_param = NULL;
	
	for (cmd_nbr = 0; cmd_nbr < NBR_OF_CMDS; cmd_nbr++)
	{
		if (strncasecmp( g_cmd, cmd_list[cmd_nbr].cmd, strlen(cmd_list[cmd_nbr].cmd) ) == 0)
		{
			cmd_found = true;
			p_param = &g_cmd[strlen(cmd_list[cmd_nbr].cmd)];
			break;	// Get out of the 'for' loop without incrementing 'i'
		}
	}
	
	if (cmd_found)
	{
		move( 4, 0 );
		
		switch (cmd_nbr)
		{
			case CMD_SHOW_MENU:
				Cmd_Line_Print_Menu();
				break;
				
			case CMD_EXIT:
				endwin();
				_exit(3);
				break;
				
			case CMD_TOGGLE_DEBUG_FLAG:
				printw("Not implemented yet\n");
				break;
				
			case CMD_SHOW_DEBUG_FLAGS:
				printw("Not implemented yet\n");
				break;
				
			case CMD_SET_SETPOINT:
				printw("New Setpoint: %4.2f\n", atof(p_param) );
				App_Set_Cabinet_Setpoint( atof(p_param) );
				break;
			
			case CMD_TOGGLE_AUTO_SERVO_CONTROL:
				printw("Not implemented yet\n");
				break;
				
			case CMD_SET_SERVO_PULSE_WIDTH:
				printw("New Pulse Width: %4d\n", atoi(p_param) );
				App_Force_Servo_Position( atoi(p_param) );
				break;

			case CMD_SET_KP:
				printw("New proportional gain: %0.3f\n", atof(p_param) );
				App_Set_Kp( atof(p_param) );
				break;
				
			case CMD_SET_KI:
				printw("New integral gain: %0.3f\n", atof(p_param) );
				App_Set_Ki( atof(p_param) );
				break;
				
			case CMD_SET_KD:
				printw("New derivative gain: %0.3f\n", atof(p_param) );
				App_Set_Kd( atof(p_param) );
				break;
				
			case CMD_SET_KL:
				printw("New integral limit: %0.3f\n", atof(p_param) );
				App_Set_Kl( atof(p_param) );
				break;
		}
	}
	else
		printw("Cmd not found: %s\n", g_cmd);
}

void Cmd_Line_Print_Menu( void )
{
	int i;

	for (i = 0; i < NBR_OF_CMDS; i++)
		printw( "%10s %s", cmd_list[i].cmd, cmd_list[i].description );
}
