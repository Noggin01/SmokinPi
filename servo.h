#define MIN_PHYSICAL_POSITION			600		// This is near the physical limit of the needle valve
#define MAX_PHYSICAL_POSITION			1200	// This is near the physical limit of the needle valve

#define MIN_POSITION_FOR_OPERATION		650		// Lowest setting at which fire is reliably present
#define MAX_POSITION_FOR_OPERATION		1100		// Highest setting to which the servo should travel	

// Returns a value >= 1 if successful
int Servo_Init( void );
int Servo_Shutdown( void );
void Servo_Service( int position );
