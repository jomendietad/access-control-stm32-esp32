#ifndef MFRC522_H
#define MFRC522_H

#include <stdint.h>
#include <stdbool.h>

/* MFRC522 Registers */
#define MFRC522_REG_COMMAND        0x01
#define MFRC522_REG_COM_IEN        0x02
#define MFRC522_REG_COM_IRQ        0x04
#define MFRC522_REG_DIV_IRQ        0x05
#define MFRC522_REG_ERROR          0x06
#define MFRC522_REG_STATUS2        0x08
#define MFRC522_REG_FIFO_DATA      0x09
#define MFRC522_REG_FIFO_LEVEL     0x0A
#define MFRC522_REG_CONTROL        0x0C
#define MFRC522_REG_BIT_FRAMING    0x0D
#define MFRC522_REG_COLL           0x0E
#define MFRC522_REG_MODE           0x11
#define MFRC522_REG_TX_CONTROL     0x14
#define MFRC522_REG_TX_ASK         0x15
#define MFRC522_REG_T_MODE         0x2A
#define MFRC522_REG_T_PRESCALER    0x2B
#define MFRC522_REG_T_RELOAD_H     0x2C
#define MFRC522_REG_T_RELOAD_L     0x2D

/* MFRC522 Commands */
#define MFRC522_CMD_IDLE           0x00
#define MFRC522_CMD_TRANSCEIVE     0x0C
#define MFRC522_CMD_SOFT_RESET     0x0F

/* PICC Commands (Tags) */
#define PICC_CMD_REQIDL            0x26
#define PICC_CMD_ANTICOLL          0x93

/* Status Codes */
#define MI_OK                      0
#define MI_NOTAGERR                1
#define MI_ERR                     2

/* Function Prototypes */
void MFRC522_Init(void);
uint8_t MFRC522_Check(uint8_t* id);

#endif /* MFRC522_H */