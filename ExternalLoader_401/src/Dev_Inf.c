/*********************************************
 * @file Dev_Inf.c
 * @brief modified by Mauro linking W25Qxxx library
 * @date: 01 august 2023
 * @version V.1.0.0
 * 
 *********************************************
 * Creating an new External loader you have to 
 * configure the first and the third field in 
 * below struct. Setup also Loader_Src.c
 *********************************************/

#include "Dev_Inf.h"
#include "Flash25.h"


/* This structure containes information used by ST-LINK Utility to program and erase the device */
const StorageInfo_t StorageInfo =
{
   "F401_W25Q128_LED", 	 	            // Device Name + version number
   SPI_FLASH,                  				// Device Type  (that's from Dev_Inf.h)
   0x90000000,                				// Device Start Address
   0x01000000,                 	   		// Device Size in Bytes (that's from Flash interface package)
   F25_PAGE_SIZE,                 		// Programming Page Size (that's from Flash interface package)
   0xFF,                       				// Initial Content of Erased Memory
// Specify Size and Address of Sectors (view example below)
   4096, 4096,
   0x00000000, 0x00000000,
};

/*  								Sector coding example
	A device with succives 16 Sectors of 1KBytes, 128 Sectors of 16 KBytes, 
	8 Sectors of 2KBytes and 16384 Sectors of 8KBytes
	
	0x00000010, 0x00000400,     							// 16 Sectors of 1KBytes
	0x00000080, 0x00004000,     							// 128 Sectors of 16 KBytes
	0x00000008, 0x00000800,     							// 8 Sectors of 2KBytes
	0x00004000, 0x00002000,     							// 16384 Sectors of 8KBytes
	0x00000000, 0x00000000,								// end
  */
