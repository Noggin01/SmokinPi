void App_Init( void );
void App_Service( void* shared_data_address );

void App_Force_Cabinet_Temperature( float temp_deg_f );
void App_Force_Servo_Position( int position );

void App_Set_Kp( double gain );
void App_Set_Ki( double gain );
void App_Set_Kd( double gain );
void App_Set_Kl( double limit );

