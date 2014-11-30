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
#include "main.h"
#include "app.h"
#include "eth_comms.h"

/* **** Data Types **** */

/* **** Defined Values **** */
#define TCP_LISTENING_PORT		46879
#define BULK_BUFFER_SIZE		1024
#define READ_BUFFER_SIZE		64

/* **** Global Variables **** */
static int g_eth_fd;
static struct sockaddr_in g_serv_addr;
static int g_conn_fd = -1;

static unsigned char g_bulk_buffer[BULK_BUFFER_SIZE];
static int g_buff_idx_in;
static int g_buff_idx_out;

/* **** Function Prototypes **** */
static void Eth_Comms_Signal_Handler( int signalnum );
static void Eth_Comms_Receive( unsigned char* pData, int bytes );
static void Eth_Comms_Process_Commands( void* shared_data_address );
static int Eth_Comms_Get_Byte( unsigned char* ch );
static int Eth_Comms_Buffer_Bytes_Available( void );

/* **** Accessors **** */

/* **** Function Definitions **** */

/**************************************************************************************************
Description:  Initialization routine for setting up Ethernet communications and data storage 
**************************************************************************************************/
int Eth_Comms_Init( void )
{
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
void Eth_Comms_Service( void* shared_data_address )
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
			g_conn_fd = accept(g_eth_fd, NULL, NULL);
		
		if (g_conn_fd >= 0)
		{	
			memset(read_buffer, 0, sizeof(read_buffer));
			bytes_read = read(g_conn_fd, read_buffer, sizeof(read_buffer));

			if (bytes_read > 0)
			{
				Eth_Comms_Receive(read_buffer, bytes_read);
				Eth_Comms_Process_Commands(shared_data_address);
//				sprintf(BuffOut, "You Wrote: %s\n", BuffIn);
//				write (g_conn_fd, BuffOut, strlen(BuffOut));
			}
			
			if (bytes_read <= 0)
			{
				close(g_conn_fd);
				g_conn_fd = -1;
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
static void Eth_Comms_Process_Commands( void* shared_data_address )
{
	typedef enum { STATE_FIND_SYNC, STATE_GET_MSG_LENGTH, STATE_GET_MESSAGE_REMAINDER, 
		STATE_VERIFY_CRC, STATE_PROCESS_MSG } state_type;
	
	static state_type state = STATE_FIND_SYNC;
	static char msg[64];
	static char* pCh;
	msg_header_type* header;
	unsigned char* payload;
	float temp_float;
	int i;
	char ch;
	bool done = false;

	header = (msg_header_type*)msg;
	payload = msg + sizeof(msg_header_type);

	// i = Eth_Comms_Buffer_Bytes_Available();
	// while (i--)
	// {
	// 	unsigned char buff[64];
		
	// 	Eth_Comms_Get_Byte(&ch);
	// 	sprintf( buff, "0x%02X ", ch );
	// 	write (g_conn_fd, buff, strlen(buff));
	// }

	// done = true;

	while (!done)
	{
		switch (state)
		{
			case STATE_FIND_SYNC:
				Eth_Print( "Searching for sync\r\n" );
				if (Eth_Comms_Get_Byte( &ch ))
				{
					header->sync_pattern = ((header->sync_pattern << 8) & 0xFF00) | ((ch << 0) & 0x00FF);
//					header->sync_pattern = ((header->sync_pattern >> 8) & 0x00FF) | ((ch << 8) & 0xFF00);
					if (header->sync_pattern == SYNC_PATTERN)
						state = STATE_GET_MSG_LENGTH;
				}
				else 
					done = true;
				break;

			case STATE_GET_MSG_LENGTH:
				if (Eth_Comms_Buffer_Bytes_Available() >= sizeof(header->length))
				{
					pCh = msg + sizeof(header->sync_pattern);
					Eth_Comms_Get_Byte(pCh++);
					Eth_Comms_Get_Byte(pCh++);
					
					if (header->length > sizeof(msg))
					{
						Eth_Print("Invalid length\r\n");
						state = STATE_FIND_SYNC;
					}
					else
						state = STATE_GET_MESSAGE_REMAINDER;
				}
				else
					done = true;
				break;

			case STATE_GET_MESSAGE_REMAINDER:
				i = header->length - sizeof(header->length) - sizeof(header->sync_pattern);
				if (Eth_Comms_Buffer_Bytes_Available() >= i)
				{
					while (i--)
						Eth_Comms_Get_Byte(pCh++);
					state = STATE_VERIFY_CRC;
				}
				else
					done = true;
				break;

			case STATE_VERIFY_CRC:
				state = STATE_PROCESS_MSG;
				break;

			case STATE_PROCESS_MSG:
			{
				unsigned char BuffOut[512] = { 0 };

				switch (header->cmd_id)
				{
					case CMD_GET_VERSION:
						Eth_Print( "CMD_GET_VERSION\r\n" );
						
						break;
					
					case CMD_SET_TEMPERATURE_SETPOINT:	
						memcpy( (unsigned char*)&temp_float, payload, sizeof(temp_float) );
						App_Set_Cabinet_Setpoint( temp_float );
						break;
					
					case CMD_SET_KP:							Eth_Print( "CMD_SET_KP\r\n" );							break;
					case CMD_SET_KI:							Eth_Print( "CMD_SET_KI\r\n" );							break;
					case CMD_SET_KL:							Eth_Print( "CMD_SET_KL\r\n" );							break;
					case CMD_SET_CHANNEL_NAME:				Eth_Print( "CMD_SET_CHANNEL_NAME\r\n" );				break;
					case CMD_GET_STATUS:						Eth_Print( "CMD_GET_STATUS\r\n" );						break;
				}
				header->sync_pattern = 0;
				state = STATE_FIND_SYNC;
			}
				break;
		}
	}
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
