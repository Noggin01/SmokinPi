#define MAX_POSITION			1200		// This is near the physical limit of the needle valve
#define MIN_POSITION			600			// This is near the physical limit of the needle valve
#define MIN_POSITION_FOR_FIRE	620			// Lowest setting that fire is reliably present

// Returns a value >= 1 if successful
int Servo_Init( void );

void Servo_Service( int position );
