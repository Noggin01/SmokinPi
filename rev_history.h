/***************************************************************************************************
File:  rev_history.h

***************************************************************************************************/

#define FIRMWARE_MAJOR		0
#define FIRMWARE_MINOR		3
#define FIRMWARE_REVISION	0

/* Description of changes. *************************************************************************

*** 26FEB17 *** Ver 0.3.0 *** HGM
1. Changed thermocouple support to the Maverick PR-005 for availability purposes
2. Corrected an issue with the SPI speed that was causing ADC readings to be
occasionally right shifted by 1
3. Overhauled the ethernet port command structure to remove the raw data structs and replace them
with a human readable data string format to fascilitate debugging.  TCP protocol will handle
ensuring the data is transmitted and received correctly

*** 19NOV15 *** Ver 0.2.1 *** HGM
1. Began addition of thermocouple temperature measurement for fire detection purposes
2. Added a monitor module to send messages when the application starts and when the fire goes out

*** 19NOV15 *** Ver 0.2.0 *** HGM
1. Updated the ADC code to work with the version 40 of the PIGPIOD library.  Previously, version
16 was needed, and version 16 had a custom modification as well.  This change should make updating
libraries in the future easier.
2. Updates servo code to work with PIGPIODv40
3. More reasonable settings for the PID control were selected as defaults
4. The default setpoint is now 225°F

Dependancies:
	1. PIGPIO Library, currently at version 40, http://abyz.co.uk/rpi/pigpio
		pigpiod should be set to autostart on bootup
	2. ncurses-dev, use sudo apt-get install ncurses-dev
	3. raspbian-wheezy, raspbian-jessie is untested

*** 26NOV14 *** Ver 0.1.4 *** HGM
1. Began adding basic Ethernet functionality

*** 14JUL14 *** Ver 0.1.3 *** HGM
1. Added command to file_fifo to allow retrieval of all thermistor temps
2. Added Makefile to speed building the project

*** 22JUN14 *** Ver 0.1.2 *** HGM
1. Lots of stuff to support control via pipes (file_fifo.? mainly)

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
