#ifndef BSP_RFID_H
#define BSP_RFID_H

#include <stdint.h>
#include <stdbool.h>

void BSP_RFID_Init(void);
bool BSP_RFID_IsTagValid(void);

/* Helper functions exposed ONLY for mfrc522.c */
void BSP_RFID_CS_Select(bool select);
uint8_t BSP_RFID_SPI_Transfer(uint8_t data);

#endif