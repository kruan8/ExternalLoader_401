/*
 * FlashG25D10B.h
 *
 *  Created on: 7. 11. 2016
 *      Author: priesolv
 */

#ifndef FLASHG25_H_
#define FLASHG25_H_

#include "stm32f4xx.h"
#include "spi_f4.h"
#include <stdbool.h>

#define F25_PAGE_SIZE               256
#define F25_PAGES_PER_SECTOR        16
#define F25_SECTOR_SIZE             (F25_PAGE_SIZE * F25_PAGES_PER_SECTOR)

typedef enum
{
  BP_NONE = 0x00,
} Flash25_t;

typedef struct
{
  uint8_t WIP: 1;    // write in progress
  uint8_t WEL: 1;    // write enable latch
  Flash25_t BP: 3;
  uint8_t reserved1: 1;
  uint8_t reserved2: 1;
  uint8_t SRP: 1;          // status register protect
}Flash25Status_t;

typedef struct
{
  uint32_t identificationID;
  uint32_t pages;
  uint16_t sectors;
  const char* strType;
}Flash25Identify_t;

bool Flash25_Init(spi_drv_t* pSpi, gpio_pins_e eCS, uint32_t nMaxFreqMhz);
uint32_t Flash25_GetID();

Flash25Status_t Flash25_GetStatus();
void Flash25_ReadData(uint32_t nAddr, uint8_t* pBuffer, uint32_t length);
void Flash25_WriteEnable();
void Flash25_WriteDisable();
void Flash25_WriteData(uint32_t nAddr, uint8_t* pBuffer, uint32_t length);
void Flash25_SectorErase(uint32_t nSectorNumber);
void Flash25_Block32Erase(uint32_t nBlockNumber);
void Flash25_Block64Erase(uint32_t nBlockNumber);
void Flash25_ChipErase(void);
bool Flash25_IsPresent();
void Flash25_SetDeepPower();
uint32_t Flash25_GetSectors();
uint32_t Flash25_GetPages();
const char* Flash25_GetTypeString();
void Flash25_Send24bit(uint32_t nValue);

#endif /* FLASHG25_H_ */
