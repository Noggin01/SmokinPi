/**********************************************************************
File:  rev_history.h

**********************************************************************/

#define FIRMWARE_MAJOR		0
#define FIRMWARE_MINOR		1
#define FIRMWARE_REVISION	1

/* Description of changes. ********************************************

*** 17JUN14 *** Ver 0.1.1 *** HGM
1. Removed WiringPi dependancy which was used for SPI.  Updated the
PiGPIO library to the latest version which includes SPI support.  
Updated code to make use of the SPI functionality of PiGPIO as this
reduces the number of external dependancies.  PiGPIO version used here
is version #16.  
2. Updated servro control to use the PiGPIO pipe since that doesn't
require the use of 'sudo'

*** 16JUN14 *** Ver 0.1.0 *** HGM
1. Initial release.  Not very user friendly.

*/