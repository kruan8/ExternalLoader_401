/*
 * Flash25.c
 *
 *  Created on: 20. 12. 2020
 *      Author: Priesol Vladimir
 */

#include "Flash25.h"
#include "common_f4.h"


#define F25_COMID_WRITE_ENABLE      0x06
#define F25_COMID_WRITE_DISABLE     0x04
#define F25_COMID_READ_SR           0x05
#define F25_COMID_WRITE_SR          0x01
#define F25_COMID_READ_DATA         0x03
#define F25_COMID_PAGE_PROGRAM      0x02
#define F25_COMID_SECTOR_ERASE      0x20
#define F25_COMID_BLOCK32_ERASE     0x52
#define F25_COMID_BLOCK64_ERASE     0xD8
#define F25_COMID_CHIP_ERASE        0xC7
#define F25_COMID_DEVICE_ID         0x90
#define F25_COMID_IDENTIFICATION    0x9F
#define F25_COMID_DEEP_POWER        0xB9

#define F25_STATUS_WIP              0x01     // write in progress
#define F25_STATUS_WEL              0x02
#define F25_STATUS_SRP              0x80

#define GIGADEVICE_ID               0xC8
#define ADESTO_ID                   0x1F

#define F25_CS                      PA4 // chip select


static const Flash25Identify_t g_F25types[] = {
    // ID      | pages | sectors
    { 0xC84011,    512,    32, "G25D10/1Mb" },        // G25D10 1Mbit
    { 0xC84013,   2048,   128, "G25Q41/4Mb" },        // G25Q41 4Mbit
    { 0xC84017,  32768,  2048, "G25Q64/64Mb" },       // GD25Q64 64Mbit
    { 0x1F8401,   2048,   128, "AT25SF041/4Mb" },     // AT25SF041 4Mbit
    { 0xEF4015,   8192,   512, "25Q16/16Mb" },        // WINBOND 25Q16 16Mbit
    { 0xEF4018,  65536,  4096, "W25Q128/128Mb" },     // WINBOND 25Q128 128Mbit
};

static uint32_t            g_nPages = 0;    // memory page count
static uint32_t            g_nSectors = 0;  // memory sector count
static const char*         g_pTypeString;   // memory description string
static spi_drv_t*          g_pSpi;          // SPI driver
static gpio_pins_e         g_eCS;
static spi_br_e            g_eSpiPrescaler; // prescaler for SPI interface (communication speed)

bool Flash25_Init(spi_drv_t* pSpi, gpio_pins_e eCS, uint32_t nMaxFreqMhz)
{
  g_pSpi = pSpi;
  g_eCS = eCS;

  // calculate SPI prescaler
  g_eSpiPrescaler = spi_CalculatePrescaler(pSpi->nBusFrequencyHz, nMaxFreqMhz);

  // set CS for output
  GPIO_ClockEnable(eCS);
  GPIO_SETPIN(eCS);
  GPIO_ConfigPin(eCS, mode_output, outtype_pushpull, pushpull_no, speed_veryhigh);

  if (!Flash25_IsPresent())
  {
    return false;
  }

  if (g_nSectors == 0)
  {
    return false;
  }

  return true;
}

Flash25Status_t Flash25_GetStatus()
{
  spi_TransactionBegin(g_pSpi, F25_CS, g_eSpiPrescaler);

  spi_SendData8(g_pSpi, F25_COMID_READ_SR);
  uint8_t sr = spi_SendData8(g_pSpi, SPI_DUMMY_BYTE);

  spi_TransactionEnd(g_pSpi, F25_CS);

  Flash25Status_t *pStatus = (Flash25Status_t *) &sr;
  return *pStatus;
}

void Flash25_ReadData(uint32_t nAddr, uint8_t* pBuffer, uint32_t length)
{
  spi_TransactionBegin(g_pSpi, F25_CS, g_eSpiPrescaler);

  spi_SendData8(g_pSpi, F25_COMID_READ_DATA);
  Flash25_Send24bit(nAddr);
  while (length--)
  {
    *pBuffer++ = spi_SendData8(g_pSpi, SPI_DUMMY_BYTE);
  }

  spi_TransactionEnd(g_pSpi, F25_CS);
}

void Flash25_WriteData(uint32_t nAddr, uint8_t* pBuffer, uint32_t length)
{
  uint16_t nBlockSize;
  uint16_t nPhysSize;
  while (length)
  {
    nBlockSize = length;
    nPhysSize = F25_PAGE_SIZE - (nAddr % F25_PAGE_SIZE);  // hranice fyzické stránky (PAGE SIZE)
    if (length > nPhysSize)
    {
      nBlockSize = nPhysSize;
    }

    Flash25_WriteEnable();
    spi_TransactionBegin(g_pSpi, F25_CS, g_eSpiPrescaler);
    spi_SendData8(g_pSpi, F25_COMID_PAGE_PROGRAM);
    Flash25_Send24bit(nAddr);
    uint16_t nSize = nBlockSize;
    while (nSize--)
    {
      spi_SendData8(g_pSpi, *pBuffer++);
    }

    spi_TransactionEnd(g_pSpi, F25_CS);

    // cekani na ukonceni programovaciho cyklu
    while (Flash25_GetStatus().WIP);
    nAddr += nBlockSize;
    length -= nBlockSize;
  }
}

void Flash25_SectorErase(uint32_t nSectorNumber)
{
  nSectorNumber *= F25_SECTOR_SIZE;
  Flash25_WriteEnable();
  spi_TransactionBegin(g_pSpi, F25_CS, g_eSpiPrescaler);

  spi_SendData8(g_pSpi, F25_COMID_SECTOR_ERASE);
  Flash25_Send24bit(nSectorNumber);
  spi_TransactionEnd(g_pSpi, F25_CS);

  while (Flash25_GetStatus().WIP);
}

void Flash25_Block32Erase(uint32_t nBlockNumber)
{
  nBlockNumber *= 1 << 15;
  Flash25_WriteEnable();
  spi_TransactionBegin(g_pSpi, F25_CS, g_eSpiPrescaler);

  spi_SendData8(g_pSpi, F25_COMID_BLOCK32_ERASE);
  Flash25_Send24bit(nBlockNumber);
  spi_TransactionEnd(g_pSpi, F25_CS);

  while (Flash25_GetStatus().WIP);
}

void Flash25_Block64Erase(uint32_t nBlockNumber)
{
  nBlockNumber *= 1 << 16;
  Flash25_WriteEnable();
  spi_TransactionBegin(g_pSpi, F25_CS, g_eSpiPrescaler);

  spi_SendData8(g_pSpi, F25_COMID_BLOCK64_ERASE);
  Flash25_Send24bit(nBlockNumber);
  spi_TransactionEnd(g_pSpi, F25_CS);

  while (Flash25_GetStatus().WIP);
}

void Flash25_ChipErase(void)
{
  Flash25_WriteEnable();
  spi_TransactionBegin(g_pSpi, F25_CS, g_eSpiPrescaler);

  spi_SendData8(g_pSpi, F25_COMID_CHIP_ERASE);
  spi_TransactionEnd(g_pSpi, F25_CS);

  while (Flash25_GetStatus().WIP);
}

void Flash25_WriteEnable()
{
  spi_TransactionBegin(g_pSpi, F25_CS, g_eSpiPrescaler);
  spi_SendData8(g_pSpi, F25_COMID_WRITE_ENABLE);
  spi_TransactionEnd(g_pSpi, F25_CS);
}

void FlashF25_WriteDisable()
{
  spi_TransactionBegin(g_pSpi, F25_CS, g_eSpiPrescaler);
  spi_SendData8(g_pSpi, F25_COMID_WRITE_DISABLE);
  spi_TransactionEnd(g_pSpi, F25_CS);
}

uint32_t FlashF25_GetID()
{
  spi_TransactionBegin(g_pSpi, F25_CS, g_eSpiPrescaler);

  uint32_t nID = 0;
  spi_SendData8(g_pSpi, F25_COMID_IDENTIFICATION);
  nID = spi_SendData8(g_pSpi, SPI_DUMMY_BYTE) << 16;
  nID |= spi_SendData8(g_pSpi, SPI_DUMMY_BYTE) << 8;
  nID |= spi_SendData8(g_pSpi, SPI_DUMMY_BYTE);

  spi_TransactionEnd(g_pSpi, F25_CS);

  return nID;
}

const char* FlashF25_GetTypeString()
{
  return g_pTypeString;
}

bool Flash25_IsPresent()
{
  bool bFounded = false;
  uint32_t nId = FlashF25_GetID();

  uint8_t nTabSize = sizeof (g_F25types) / sizeof(Flash25Identify_t);
  for (uint8_t i = 0; i < nTabSize; i++)
  {
    if (g_F25types[i].identificationID == nId)
    {
      g_nPages = g_F25types[i].pages;
      g_nSectors = g_F25types[i].sectors;
      g_pTypeString = g_F25types[i].strType;
      bFounded = true;
      break;
    }
  }

  return bFounded;
}

void Flash25_SetDeepPower()
{
  spi_TransactionBegin(g_pSpi, F25_CS, g_eSpiPrescaler);
  spi_SendData8(g_pSpi, F25_COMID_DEEP_POWER);
  spi_TransactionEnd(g_pSpi, F25_CS);
}

uint32_t Flash25_GetSectors()
{
  return g_nSectors;
}

uint32_t Flash25_GetPages()
{
  return g_nPages;
}

void Flash25_Send24bit(uint32_t nValue)
{
  spi_SendData8(g_pSpi, nValue >> 16);
  spi_SendData8(g_pSpi, nValue >> 8);
  spi_SendData8(g_pSpi, nValue);
}
