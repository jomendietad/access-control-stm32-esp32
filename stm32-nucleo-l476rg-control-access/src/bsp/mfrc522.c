#include "bsp/mfrc522.h"
#include "bsp/bsp_rfid.h" /* To use BSP_RFID_SPI_Transfer and BSP_RFID_CS_Select */

static void MFRC522_WriteRegister(uint8_t addr, uint8_t val) {
    BSP_RFID_CS_Select(true);
    BSP_RFID_SPI_Transfer((addr << 1) & 0x7E);
    BSP_RFID_SPI_Transfer(val);
    BSP_RFID_CS_Select(false);
}

static uint8_t MFRC522_ReadRegister(uint8_t addr) {
    uint8_t val;
    BSP_RFID_CS_Select(true);
    BSP_RFID_SPI_Transfer(((addr << 1) & 0x7E) | 0x80);
    val = BSP_RFID_SPI_Transfer(0x00);
    BSP_RFID_CS_Select(false);
    return val;
}

static void MFRC522_SetBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp = MFRC522_ReadRegister(reg);
    MFRC522_WriteRegister(reg, tmp | mask);
}

static void MFRC522_ClearBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp = MFRC522_ReadRegister(reg);
    MFRC522_WriteRegister(reg, tmp & (~mask));
}

static void MFRC522_AntennaOn(void) {
    uint8_t temp = MFRC522_ReadRegister(MFRC522_REG_TX_CONTROL);
    if (!(temp & 0x03)) {
        MFRC522_SetBitMask(MFRC522_REG_TX_CONTROL, 0x03);
    }
}

static uint8_t MFRC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint32_t *backLen) {
    uint8_t status = MI_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t lastBits;
    uint8_t n;
    uint32_t i;

    if (command == MFRC522_CMD_TRANSCEIVE) {
        irqEn = 0x77;
        waitIRq = 0x30;
    }

    MFRC522_WriteRegister(MFRC522_REG_COM_IEN, irqEn | 0x80);
    MFRC522_ClearBitMask(MFRC522_REG_COM_IRQ, 0x80);
    MFRC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);

    for (i = 0; i < sendLen; i++) {
        MFRC522_WriteRegister(MFRC522_REG_FIFO_DATA, sendData[i]);
    }

    MFRC522_WriteRegister(MFRC522_REG_COMMAND, command);
    if (command == MFRC522_CMD_TRANSCEIVE) {
        MFRC522_SetBitMask(MFRC522_REG_BIT_FRAMING, 0x80);
    }

    /* Fast hardware timeout (approx 25ms maximum wait, non-blocking at OS level) */
    i = 2000;
    do {
        n = MFRC522_ReadRegister(MFRC522_REG_COM_IRQ);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    MFRC522_ClearBitMask(MFRC522_REG_BIT_FRAMING, 0x80);

    if (i != 0) {
        if (!(MFRC522_ReadRegister(MFRC522_REG_ERROR) & 0x1B)) {
            status = MI_OK;
            if (n & irqEn & 0x01) status = MI_NOTAGERR;
            if (command == MFRC522_CMD_TRANSCEIVE) {
                n = MFRC522_ReadRegister(MFRC522_REG_FIFO_LEVEL);
                lastBits = MFRC522_ReadRegister(MFRC522_REG_CONTROL) & 0x07;
                if (lastBits) *backLen = (n - 1) * 8 + lastBits;
                else *backLen = n * 8;

                if (n == 0) n = 1;
                if (n > 16) n = 16;
                for (i = 0; i < n; i++) backData[i] = MFRC522_ReadRegister(MFRC522_REG_FIFO_DATA);
            }
        } else {
            status = MI_ERR;
        }
    }
    return status;
}

static uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *tagType) {
    uint8_t status;  
    uint32_t backBits;

    MFRC522_WriteRegister(MFRC522_REG_BIT_FRAMING, 0x07);
    tagType[0] = reqMode;
    status = MFRC522_ToCard(MFRC522_CMD_TRANSCEIVE, tagType, 1, tagType, &backBits);
    if ((status != MI_OK) || (backBits != 0x10)) status = MI_ERR;
    return status;
}

static uint8_t MFRC522_Anticoll(uint8_t *serNum) {
    uint8_t status;
    uint8_t i;
    uint8_t serNumCheck = 0;
    uint32_t unLen;

    MFRC522_WriteRegister(MFRC522_REG_BIT_FRAMING, 0x00);
    serNum[0] = PICC_CMD_ANTICOLL;
    serNum[1] = 0x20;
    status = MFRC522_ToCard(MFRC522_CMD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (status == MI_OK) {
        for (i = 0; i < 4; i++) serNumCheck ^= serNum[i];
        if (serNumCheck != serNum[4]) status = MI_ERR;
    }
    return status;
}

void MFRC522_Init(void) {
    MFRC522_WriteRegister(MFRC522_REG_COMMAND, MFRC522_CMD_SOFT_RESET);
    
    MFRC522_WriteRegister(MFRC522_REG_T_MODE, 0x8D);
    MFRC522_WriteRegister(MFRC522_REG_T_PRESCALER, 0x3E);
    MFRC522_WriteRegister(MFRC522_REG_T_RELOAD_L, 30);
    MFRC522_WriteRegister(MFRC522_REG_T_RELOAD_H, 0);
    
    MFRC522_WriteRegister(MFRC522_REG_TX_ASK, 0x40);
    MFRC522_WriteRegister(MFRC522_REG_MODE, 0x3D);
    MFRC522_AntennaOn();
}

uint8_t MFRC522_Check(uint8_t* id) {
    uint8_t status;
    status = MFRC522_Request(PICC_CMD_REQIDL, id);
    if (status == MI_OK) {
        status = MFRC522_Anticoll(id);
    }
    return status;
}