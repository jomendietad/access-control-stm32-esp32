#include "bsp_rfid.h"
#include "bsp_placa.h"
#include "mfrc522.h"
#include "stm32l4xx_hal.h"
#include <string.h>

#define RFID_CS_PIN       GPIO_PIN_6
#define RFID_CS_PORT      GPIOB
#define RFID_RST_PIN      GPIO_PIN_7
#define RFID_RST_PORT     GPIOC

static SPI_HandleTypeDef hspi1;

/* Reemplaza esto con el código de TU tarjeta/llavero real */
static const uint8_t AUTHORIZED_UID[4] = {0xB6, 0x7B, 0x0C, 0x03};

void BSP_RFID_Init(void) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* SPI1 Pins */
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* CS & RST Pins */
    GPIO_InitStruct.Pin = RFID_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RFID_CS_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RFID_RST_PIN;
    HAL_GPIO_Init(RFID_RST_PORT, &GPIO_InitStruct);

    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; 
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi1);

    /* Hardware Reset Sequence */
    HAL_GPIO_WritePin(RFID_CS_PORT, RFID_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RFID_RST_PORT, RFID_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10); 
    HAL_GPIO_WritePin(RFID_RST_PORT, RFID_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    
    MFRC522_Init();
}

uint8_t BSP_RFID_SPI_Transfer(uint8_t data) {
    uint8_t rx_data = 0;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx_data, 1, 10);
    return rx_data;
}

void BSP_RFID_CS_Select(bool select) {
    HAL_GPIO_WritePin(RFID_CS_PORT, RFID_CS_PIN, select ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

bool BSP_RFID_IsTagValid(void) {
    static uint32_t last_poll_time = 0;
    uint8_t uid[5]; /* 4 bytes ID + 1 byte CRC */
    
    /* Strict 100ms polling rate to avoid blocking the main loop */
    if ((BSP_GetTicks() - last_poll_time) < 100) {
        return false; 
    }
    last_poll_time = BSP_GetTicks();

    if (MFRC522_Check(uid) == MI_OK) {
        if (memcmp(uid, AUTHORIZED_UID, 4) == 0) {
            return true;
        }
    }
    return false;
}