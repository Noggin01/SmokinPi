#ifndef _MONITOR_H
#define _MONITOR_H

typedef enum
{
	MONITOR_WAITING_FOR_FIRE = 0,
	MONITOR_FIRE_DETECTED = 1,
	MONITOR_FIRE_LOST = 2,
	
	NBR_MONITOR_STATES = 3,
} fire_detect_state_type;

void Monitor_Init( void* shared_data_address );

void Monitor_Service( void* shared_data_address );

void Monitor_Light_Fire( void );

void Monitor_Send_Notification( char* pSubject, char* pMsg );

#endif