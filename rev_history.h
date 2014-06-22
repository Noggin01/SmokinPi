/***************************************************************************************************
File:  rev_history.h

***************************************************************************************************/

#define FIRMWARE_MAJOR		0
#define FIRMWARE_MINOR		1
#define FIRMWARE_REVISION	2

/* Description of changes. *************************************************************************

*** 22JUN14 *** Ver 0.1.2 *** HGM


*** 17JUN14 *** Ver 0.1.1 *** HGM
1. Removed WiringPi dependancy which was used for SPI.  Updated the PiGPIO library to the latest 
version which includes SPI support.  Updated code to make use of the SPI functionality of PiGPIO as 
this reduces the number of external dependancies.  PiGPIO version used here is version #16 but has 
been modified to make the SPI responses predictable.
2. Updated servro control to use the PiGPIO pipe to remove the sudo requirement
3. Updated tlc1543 ADC services to use the PiGPIO pipe to remove the sudo requirement
4. Updated app_service to not overwrite shared ADC readings with its local copy at the end of the 
service routine. 

*** 16JUN14 *** Ver 0.1.0 *** HGM
1. Initial release.  Not very user friendly.

*/