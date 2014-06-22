#ifndef __FILE_FIFO_H
#define __FILE_FIFO_H

#define SMPI_INPFIFO "/tmp/smpiinp"
#define SMPI_OUTFIFO "/tmp/smpiout"
#define SMPI_ERRFIFO "/tmp/smpierr"

int File_Fifo_Init( void );

void File_Fifo_Service_Input( void *shared_data_address );
void File_Fifo_Service_Output( void *shared_data_address );

#endif //__FILE_FIFO_H
