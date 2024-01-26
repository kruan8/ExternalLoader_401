/*********************************************
 * @file Dev_Inf.c
 * @brief modified by Mauro linking W25Qxxx library
 * @date: 01 august 2023
 * @version V.1.0.0
 * 
 *********************************************
 * Creating an new External loader you have to 
 * follow the below step 1 to 3 and set the
 * first field in the struct of Dev_Inf.c
 *********************************************/

#include "stm32f4xx.h"
#include "spi_f4.h"
#include "Flash25.h"

#define EXT_FLASH_ADDR_MASK 0x0FFFFFFF

/* STEP 1 *************************************
 * remove comment in below #define if you have
 * a led for the External Loader.
 * On CubeMX, led's pin label must be LED
 * ********************************************/
#define IS_LED

/* STEP 2 *************************************
 * change the below #define assigning Pin name
 * and port you used for led,
 * specify also pin level to turn on led
 * ********************************************/
#ifdef IS_LED
#define LED_PIN 	        PC13    	//this is the GPIO level turning on led
#endif //IS_LED

#define FLASH_CS                PA4

static spi_drv_t*                g_pSpi1 = spi1;

void LOC_LedOn()
{
#ifdef IS_LED
  GPIO_RESETPIN(LED_PIN);
#endif //IS_LED
}

void LOC_LedOff()
{
#ifdef IS_LED
  GPIO_SETPIN(LED_PIN);
#endif //IS_LED
}

/**
  * @brief  The Init function defines the used GPIO pins which are connecting the external
  * memory to the device, and initializes the clock of the used IPs.
  *
  * Returns 1 if success, and 0 in failure
  *
  * @param  None
  * @retval  1      : Operation succeeded
  * @retval  0      : Operation failed
  */
int Init (void)
{
  *(uint32_t*)0xE000EDF0 = 0xA05F0000; //enable interrupts in debug

  SystemInit();

  /* ADAPTATION TO THE DEVICE
   *
   * change VTOR setting for H7 device
   * SCB->VTOR = 0x24000000 | 0x200;
   *
   * change VTOR setting for other devices
   * SCB->VTOR = 0x20000000 | 0x200;
   *
   */

  SCB->VTOR = 0x20000000 | 0x200;

  __set_PRIMASK(0); //enable interrupts

  spi_Init(g_pSpi1, PA5, PA7, PA6);

	LOC_LedOn();
  bool bRes = Flash25_Init(g_pSpi1, FLASH_CS, 50000000);
  if (bRes == false)
  {
    spi_DeInit(g_pSpi1, PA5, PA7, PA6);
    spi_Init(g_pSpi1, PA5, PA7, PB4);
    bRes = Flash25_Init(g_pSpi1, FLASH_CS, 50000000);
  }

	LOC_LedOff();

  __set_PRIMASK(1); //disable interrupts

  return bRes;
}

/**
 * The Read function is used to read a specific range of memory, and returns the reading in a buffer in the RAM
 *
 * @param Address = start address of read operation
 * @param Size = size of the read operation
 * @param buffer = is the pointer to data read
 * @return Returns 1 if success, and 0 if failure.
 */
int Read (uint32_t Address, uint32_t Size, uint8_t* buffer)
{
  __set_PRIMASK(0); //enable interrupts
	Address = Address & EXT_FLASH_ADDR_MASK;
	LOC_LedOn();
	Flash25_ReadData(Address, buffer, Size);
	LOC_LedOff();
  __set_PRIMASK(1); //disable interrupts
	return 1;
} 

/**
  * @brief   The Write function programs a buffer defined by an address in the RAM range.
  *
  * @param   Address: page address
  * @param   Size   : size of data
  * @param   buffer : pointer to data buffer
  * @retval  1      : Operation succeeded
  * @retval  0      : Operation failed
  */
int Write (uint32_t Address, uint32_t Size, uint8_t* buffer)
{
  __set_PRIMASK(0); //enable interrupts
	Address = Address & EXT_FLASH_ADDR_MASK;
	LOC_LedOn();
	Flash25_WriteData(Address, buffer, Size);
	LOC_LedOff();
  __set_PRIMASK(1); //disable interrupts
	return 1;
} 

/**
  * @brief   Full erase of the device
  * @param   Parallelism : 0
  * @retval  1           : Operation succeeded
  * @retval  0           : Operation failed
  */
int MassErase (void)
{
  __set_PRIMASK(0); //enable interrupts

	LOC_LedOn();
	Flash25_ChipErase();
	LOC_LedOff();

  __set_PRIMASK(1); //disable interrupts
	return 1;
}

/**
  * @brief   The SectorErase function erases the memory specified sectors.
  *          This function is not used in case of an SRAM.
  *
  * @param   EraseStartAddress :  erase start address
  * @param   EraseEndAddress   :  erase end address
  * @retval  None
  */
int SectorErase (uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
	EraseStartAddress = EraseStartAddress & EXT_FLASH_ADDR_MASK;
	EraseEndAddress = EraseEndAddress & EXT_FLASH_ADDR_MASK;

  __set_PRIMASK(0); //enable interrupts

	EraseStartAddress = (EraseStartAddress -  (EraseStartAddress % F25_SECTOR_SIZE));
	while (EraseEndAddress >= EraseStartAddress)
	{
		LOC_LedOn();
		Flash25_SectorErase(EraseStartAddress / F25_SECTOR_SIZE);
		LOC_LedOff();
		EraseStartAddress += F25_SECTOR_SIZE;
	}

  __set_PRIMASK(1); //disable interrupts
	return 1;
}

/**
  * Description :
  * Calculates checksum value of the memory zone
  * Inputs    :
  *      StartAddress  : Flash start address
  *      Size          : Size (in WORD)  
  *      InitVal       : Initial CRC value
  * outputs   :
  *     R0             : Checksum value
  * Note - Optional for all types of device
  * NOTE - keeping original ST algorithm: not verified and optimized
  */
uint32_t CheckSum(uint32_t StartAddress, uint32_t Size, uint32_t InitVal)
{
  uint8_t missalignementAddress = StartAddress % 4;
  uint8_t missalignementSize = Size ;
  int cnt;
  uint32_t Val;
  //uint8_t value;
	
  StartAddress -= StartAddress % 4;
  Size += (Size % 4 == 0) ? 0 : 4 - (Size % 4);
  
  for(cnt = 0; cnt < Size ; cnt += 4)
  {
    LOC_LedOn();
    Flash25_ReadData(StartAddress + 1, (uint8_t *)&Val, 4);
    LOC_LedOff();

    if(missalignementAddress)
    {
      for (uint8_t k=missalignementAddress; k <= 3;k++)
      {
          InitVal += (uint8_t) (Val >> (8 * k) & 0xff);
      }

      missalignementAddress = 0;
    }
    else if((Size-missalignementSize) % 4 && (Size - cnt) <=4)
    {
      for (uint8_t k = (Size-missalignementSize); k <= 3;k++)
      {
          InitVal += (uint8_t) (Val>>(8 * (k - 1)) & 0xff);
      }

      missalignementSize = 2 * missalignementSize - Size;
    }
    else
    {
        for (uint8_t k = 0; k <= 3; k++)
        {
            InitVal += (uint8_t) (Val >> (8 * k) & 0xff);
        }
    }

    StartAddress += 4;
  }
  
  return (InitVal);
}

/**
 * The Verify function is called when selecting the “verify while programming” mode.
 * This function checks if the programmed memory corresponds to the buffer defined in the RAM.
 *
 * @param MemoryAddr
 * @param RAMBufferAddr
 * @param Size
 * @param missalignement
 * @return
 */
uint64_t Verify (uint32_t MemoryAddr, uint32_t RAMBufferAddr, uint32_t Size, uint32_t missalignement)
{
#define BUF_SIZE 2

  uint32_t InitVal = 0;
  uint32_t VerifiedData = 0;
//  uint8_t TmpBuffer = 0x00;
	uint64_t checksum;
  Size *= 4;
  uint8_t Buffer[BUF_SIZE];
  uint32_t LocAddr = MemoryAddr & EXT_FLASH_ADDR_MASK;
  uint32_t posBuf;
        
  checksum = CheckSum((uint32_t)LocAddr + (missalignement & 0xf), Size - ((missalignement >> 16) & 0xF), InitVal);
  
  while (Size > VerifiedData)
  {
    LOC_LedOn();
    Flash25_ReadData(MemoryAddr + VerifiedData, Buffer, BUF_SIZE);
    LOC_LedOff();

    posBuf = 0;
    while ((Size > VerifiedData) && (posBuf<1024))
    {
        if (Buffer[posBuf] != *((uint8_t*)RAMBufferAddr+VerifiedData))
        {
          return ((checksum << 32) + MemoryAddr + VerifiedData);
        }

        posBuf++;
        VerifiedData++;
    }
  }
       
  return (checksum << 32);
}
