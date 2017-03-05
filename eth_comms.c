#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <syslog.h>
#include "main.h"
#include "app.h"
#include "eth_comms.h"
#include "rev_history.h"

/* **** Data Types **** */
	//! Prototype of function that is called after a sucessful determination of a command
typedef char* (*eth_function)(char*);

	//! Structure to associate a command to a description and function
typedef struct eth_cmd_type
{
   const char* cmd;
   const char* description;
   eth_function action;
} eth_cmd_type;

/* **** Defined Values **** */
#define TCP_LISTENING_PORT		46879
#define BULK_BUFFER_SIZE		1024
#define READ_BUFFER_SIZE		64

/* **** Global Variables **** */
static int g_eth_fd;
static struct sockaddr_in g_serv_addr;
static int g_conn_fd = -1;
static shared_data_type* p_shared_data;

static char* Eth_Comms_Version( char* param );
static char* Eth_Get_Temps(     char* param );
static char* Eth_Get_Status(    char* param );
static char* Eth_Set_Temp(      char* param );

static const eth_cmd_type		g_eth_cmds[] =				//!< List of standard commands
{
	// All *COMMANDS* must be fully uppercase
    {"VERSION?",	"Returns version information",                      Eth_Comms_Version   },
    {"TEMPS?",      "Returns the temperature information",              Eth_Get_Temps       },
    {"STATUS?",     "Returns most information about the SMPi",          Eth_Get_Status      },
    {"SETTEMP=",    "Sets the setpoint to the specified value",         Eth_Set_Temp        },
};
#define ETH_CMDS_SIZE		(sizeof (g_eth_cmds)/sizeof(g_eth_cmds[0]))

static unsigned char g_bulk_buffer[BULK_BUFFER_SIZE];
static int g_buff_idx_in;
static int g_buff_idx_out;

/* **** Function Prototypes **** */
static void Eth_Comms_Signal_Handler( int signalnum );
static void Eth_Comms_Receive( unsigned char* pData, int bytes );
static void Eth_Comms_Process_Commands( char* cmd );
static int Eth_Comms_Get_Byte( unsigned char* ch );
static int Eth_Comms_Buffer_Bytes_Available( void );

/* **** Accessors **** */

/* **** Function Definitions **** */

/**************************************************************************************************
Description:  Initialization routine for setting up Ethernet communications and data storage 
**************************************************************************************************/
int Eth_Comms_Init( void* shared_data_address )
{
    p_shared_data = (shared_data_type*)shared_data_address;
    
	g_eth_fd = socket(AF_INET, SOCK_STREAM, 0 );	// Create the socket
	memset((unsigned char*)&g_serv_addr, 0, sizeof(g_serv_addr));
	g_serv_addr.sin_family = AF_INET;
	g_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	g_serv_addr.sin_port = htons(TCP_LISTENING_PORT);
	bind(g_eth_fd, (struct sockaddr*)&g_serv_addr, sizeof(g_serv_addr));

	g_buff_idx_in = 0;
	g_buff_idx_out = 0;

	return listen(g_eth_fd, 10);
}

/**************************************************************************************************
Description:  Service routine for receiving data over ethernet, storing the data, and calling the
data processor and handling lost connections.
**************************************************************************************************/
void Eth_Comms_Service( void )
{
	int bytes_read;
	unsigned char read_buffer[READ_BUFFER_SIZE] = { 0 };
	
	signal(SIGINT, Eth_Comms_Signal_Handler);
	signal(SIGHUP, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	while (1)
	{
		if (g_conn_fd == -1)
		{
         g_conn_fd = accept(g_eth_fd, NULL, NULL);
         openlog("smpiethlog", LOG_ODELAY, LOG_USER);
         syslog(LOG_INFO, "Ethernet connection established");
         closelog();
      }
		
		if (g_conn_fd >= 0)
		{	
			memset(read_buffer, 0, sizeof(read_buffer));
			bytes_read = read(g_conn_fd, read_buffer, sizeof(read_buffer));

			if (bytes_read > 0)
			{
            uint16_t i;
            char log_msg[128];
            
            Eth_Comms_Receive(read_buffer, bytes_read);
            Eth_Comms_Process_Commands(read_buffer);
            openlog("smpiethlog", LOG_ODELAY, LOG_USER);
            sprintf(log_msg, "Rx: %u bytes - ", bytes_read);
            for (i = 0; i < bytes_read; i++)
            {
               sprintf(log_msg, "%s 0x%02X", log_msg, read_buffer[i]);
            }
            
            syslog(LOG_INFO, log_msg);
            closelog();
			}
			
			if (bytes_read <= 0)
			{
				close(g_conn_fd);
				g_conn_fd = -1;
            openlog("smpiethlog", LOG_ODELAY, LOG_USER);
            syslog(LOG_INFO, "Connection closed");
            closelog();
			}
		}
	}
}

static void Eth_Print( unsigned char* pStr )
{
	write(g_conn_fd, pStr, strlen(pStr));
}

/**************************************************************************************************
Description:  Ethernet communications processor

1. Search for sync header
2. Read message length field
3. Read message remainder of message
4. Verify CRC (not yet implemented)
5. Pass message body to relevant processing function

**************************************************************************************************/
static void Eth_Comms_Process_Commands( char* cmd )
{
    int i;
    int cmd_length;
   
    for (i = 0; i < ETH_CMDS_SIZE; i++)
    {
        cmd_length = strlen (g_eth_cmds[i].cmd);

		if (strncmp	(cmd, g_eth_cmds[i].cmd, cmd_length) == 0)
		{
			char* response = g_eth_cmds[i].action ((char *)&cmd[cmd_length] );
            Eth_Print( response );
			return;
		}
    }
    
    Eth_Print( cmd );
}

/**************************************************************************************************
Description:  Stores received data in the data buffer to be processed shortly
**************************************************************************************************/
static void Eth_Comms_Receive( unsigned char* pData, int bytes )
{
	int i = 0;

	for (i = 0; i < bytes; i++)
	{
		g_bulk_buffer[g_buff_idx_in++] = *pData++;
		if (g_buff_idx_in >= BULK_BUFFER_SIZE)
			g_buff_idx_in = 0;
	}
}

/**************************************************************************************************
Description:  Returns the number of bytes available in the Ethernet bulk storage buffer
**************************************************************************************************/
static int Eth_Comms_Buffer_Bytes_Available( void )
{
	int result = 0;

	if (g_buff_idx_in >= g_buff_idx_out)
		result = g_buff_idx_in - g_buff_idx_out;
	else
		result = BULK_BUFFER_SIZE - g_buff_idx_out + g_buff_idx_in;

	return result;
}

/**************************************************************************************************
Description:  Reads data from the bulk storage buffer.  Returns 0 if there is no data available
else returns 1.
**************************************************************************************************/
static int Eth_Comms_Get_Byte( unsigned char* pCh )
{
int result = 0;

	if (g_buff_idx_in != g_buff_idx_out)
	{
		*pCh = g_bulk_buffer[g_buff_idx_out++];
		if (g_buff_idx_out >= BULK_BUFFER_SIZE)
			g_buff_idx_out = 0;
		result = 1;
	}

	return result;
}

/** ***********************************************************************************************
 @brief Returns the firmware veraion
 
 @param[in] param           ASCII parameter associated with this command
 @param[in] shared_data     Pointer to the shared system data
 *************************************************************************************************/
static char* Eth_Comms_Version( char* param )
{
   static char version[64];
   
   sprintf(version, "VERSION,Smokin'Pi v%u.%03u.%03u", FIRMWARE_MAJOR, FIRMWARE_MINOR, FIRMWARE_REVISION);
	
   return version;
}

/** ***********************************************************************************************
 @brief Returns the firmware veraion
 
 @param[in] param           ASCII parameter associated with this command
 @param[in] shared_data     Pointer to the shared system data
 
 Response format:  TEMPS,<setpoint>,<ch 1 temp>,...,<ch 9 temp>,<fire temp>
 All temperatures are in ASCII floating point representation
 *************************************************************************************************/
static char* Eth_Get_Temps( char* param )
{
    static char temp_data[128];
    int i;
    float local_temperature_data[NBR_OF_THERMISTORS];
    float fire_temp;
    float cabinet_setpoint;
   
    pthread_mutex_lock(&mutex);
    memcpy( (char*)local_temperature_data, (char*)p_shared_data->temp_deg_f, sizeof(local_temperature_data) );
    fire_temp = p_shared_data->temp_deg_f_fire;
    cabinet_setpoint = p_shared_data->temp_deg_f_cabinet_setpoint;
	pthread_mutex_unlock(&mutex);

   strcpy(temp_data, "TEMPS");
   
   sprintf(temp_data, "%s,%f", temp_data, cabinet_setpoint);
   
   for (i = 0; i < NBR_OF_THERMISTORS; i++)
      sprintf(temp_data, "%s,%f", temp_data, local_temperature_data[i]);
   
   sprintf(temp_data, "%s,%f", temp_data, fire_temp);
   
   return temp_data;
}

/** ***********************************************************************************************
 @brief Returns much of the operational data from the SMPi
 
 @param[in] param           ASCII parameter associated with this command
 @param[in] shared_data     Pointer to the shared system data
 
 Response format:  STATUS,<setpoint>,<ch 1 temp>,...,<ch 9 temp>,<fire temp>,<ch 0 adc>,...,
                         <fire detected state>
 
 *************************************************************************************************/
static char* Eth_Get_Status( char* param )
{
    static char status_data[1024];
    int i;
    shared_data_type local_shared_data;
    
    pthread_mutex_lock(&mutex);
    memcpy( (char*)&local_shared_data, (char*)p_shared_data, sizeof(local_shared_data) );
    pthread_mutex_unlock(&mutex);

    strcpy(status_data, "STATUS");

    sprintf(status_data, "%s,%f", status_data, local_shared_data.temp_deg_f_cabinet_setpoint);

    for (i = 0; i < NBR_OF_THERMISTORS; i++)
      sprintf(status_data, "%s,%f", status_data, local_shared_data.temp_deg_f[i]);

    sprintf(status_data, "%s,%f", status_data, local_shared_data.temp_deg_f_fire);

    for (i = 0; i < NBR_ADC_CHANNELS; i++)
        sprintf(status_data, "%s,%u", status_data, (0x3FF & local_shared_data.adc_results[i]));
   
    sprintf(status_data, "%s,%u", status_data, local_shared_data.fire_detect_state);

    return status_data;
}

/** ***********************************************************************************************
 @brief Sets the setpoint for the system
 
 @param[in] param           ASCII parameter associated with this command
 @param[in] shared_data     Pointer to the shared system data
 
 Response format:  SETTEMP,<new setpoint>
 
 *************************************************************************************************/
static char* Eth_Set_Temp( char* param )
{
    static char response[128];
    float setpoint = atof(param);
    
    if ((setpoint > 0.0) && (setpoint < 400.0))
    {
        pthread_mutex_lock(&mutex);
        p_shared_data->temp_deg_f_cabinet_setpoint = setpoint;
        pthread_mutex_unlock(&mutex);
    }
    else
    {
        pthread_mutex_lock(&mutex);
        setpoint = p_shared_data->temp_deg_f_cabinet_setpoint;
        pthread_mutex_unlock(&mutex);
    }
    
    sprintf(response, "SETTEMP,%f", setpoint);

    return response;
}


/***************************************************************************************************
***************************************************************************************************/
static void Eth_Comms_Signal_Handler( int signalnum )
{
	switch (signalnum)
	{
		case SIGINT:
			if (g_conn_fd >= 0)
			{
				close(g_conn_fd);
				g_conn_fd = -1;
			}
			exit(signalnum);
			break;

		case SIGTERM:
		case SIGHUP:
		case SIGPIPE:
			if (g_conn_fd >= 0)
			{
				close(g_conn_fd);
				g_conn_fd = -1;
			}
			break;
	}
}


/* **** End of File **** */
