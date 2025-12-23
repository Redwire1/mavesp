#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

// Default UART0 pins for USB Serial
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// I2C pins for camera configuration
static const uint8_t SDA = 40;
static const uint8_t SCL = 39;

// SPI pins (FSPI/SPI2)
static const uint8_t SS = 10;
static const uint8_t MOSI = 11;
static const uint8_t MISO = 13;
static const uint8_t SCK = 12;

// Camera interface pins (OV2640/OV5640)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39

// Camera data pins (8-bit parallel)
#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    15
#define Y3_GPIO_NUM    13
#define Y2_GPIO_NUM    21
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  45

// SD Card interface pins (SDMMC 4-bit mode)
#define SDMMC_CMD  42
#define SDMMC_CLK  41
#define SDMMC_D0   3
#define SDMMC_D1   4
#define SDMMC_D2   5
#define SDMMC_D3   6

// LED pins
#define LED_FLASH      7

// Available GPIO pins for general use (including UART)
// GPIO 1, 2, 8, 9, 17, 18, 19, 20, 46

// Analog capable pins
static const uint8_t A1 = 1;
static const uint8_t A2 = 2;
static const uint8_t A3 = 3;
static const uint8_t A4 = 4;
static const uint8_t A5 = 5;
static const uint8_t A6 = 6;
static const uint8_t A7 = 7;
static const uint8_t A8 = 8;
static const uint8_t A9 = 9;
static const uint8_t A10 = 10;
static const uint8_t A11 = 11;
static const uint8_t A12 = 12;
static const uint8_t A13 = 13;
static const uint8_t A14 = 14;
static const uint8_t A15 = 15;
static const uint8_t A16 = 16;
static const uint8_t A17 = 17;
static const uint8_t A18 = 18;
static const uint8_t A19 = 19;
static const uint8_t A20 = 20;

// Touch pins
static const uint8_t T1 = 1;
static const uint8_t T2 = 2;
static const uint8_t T3 = 3;
static const uint8_t T4 = 4;
static const uint8_t T5 = 5;
static const uint8_t T6 = 6;
static const uint8_t T7 = 7;
static const uint8_t T8 = 8;
static const uint8_t T9 = 9;
static const uint8_t T10 = 10;
static const uint8_t T11 = 11;
static const uint8_t T12 = 12;
static const uint8_t T13 = 13;
static const uint8_t T14 = 14;

#endif /* Pins_Arduino_h */
