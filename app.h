#define MAX_NAME_LENGTH			64

void App_Init( void );
void App_Service( void* shared_data_address );

void App_Set_Cabinet_Setpoint( float temp_deg_f );
float App_Get_Cabinet_Setpoint( void );

void App_Force_Servo_Position( int position );

void App_Set_Kp( float gain );
void App_Set_Ki( float gain );
void App_Set_Kd( float gain );
void App_Set_Kl( float limit );

float App_Get_Kp( void );
float App_Get_Ki( void );
float App_Get_Kl( void );
float App_Get_Kd( void );

void App_Set_Channel_Name( int channel, char* pName );
char* App_Get_Channel_Name( int channel );

