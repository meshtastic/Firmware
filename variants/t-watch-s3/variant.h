// ST7789 TFT LCD
#define ST7789_CS 12
#define ST7789_RS 38  // DC
#define ST7789_SDA 13 // MOSI
#define ST7789_SCK 18
#define ST7789_RESET -1
#define ST7789_MISO -1
#define ST7789_BUSY -1
#define ST7789_BL 45
#define ST7789_SPI_HOST SPI2_HOST
//#define ST7789_BACKLIGHT_EN 15
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 16000000
#define TFT_HEIGHT 240
#define TFT_WIDTH 240
#define TFT_OFFSET_X 0
#define TFT_OFFSET_Y 0
#define SCREEN_ROTATE
#define SCREEN_TRANSITION_FRAMERATE 1 // fps
#define SCREEN_TOUCH_INT 16
#define TOUCH_SLAVE_ADDRESS 0x5D // GT911

// #define TWATCH_TFT_MISO             (GPIO_NUM_MAX)
// #define TWATCH_TFT_MOSI             (GPIO_NUM_19)
// #define TWATCH_TFT_SCLK             (GPIO_NUM_18)
// #define TWATCH_TFT_CS               (GPIO_NUM_5)
// #define TWATCH_TFT_DC               (GPIO_NUM_27)
// #define TWATCH_TFT_RST              (GPIO_NUM_MAX)
// #define TWATCH_TFT_BL               (GPIO_NUM_15)

// Leave undefined to disable our PMU IRQ handler.  DO NOT ENABLE THIS because the pmuirq can cause sperious interrupts
// and waking from light sleep
// #define PMU_IRQ 40
#define HAS_AXP2101

#define HAS_RTC 1

// Specify the PMU as Wire1. In the t-beam-s3 core, PCF8563 and PMU share the bus

// #define TOUCH_SDA (23)
// #define TOUCH_SCL (32)
// #define TOUCH_INT (38)
// #define TOUCH_RST (14)

// #define SEN_SDA (21)
// #define SEN_SCL (22)

// #define RTC_INT_PIN (37)
// #define AXP202_INT (35)
// #define BMA423_INT1 (39)

// #define TWATCH_DAC_IIS_BCK (26)
// #define TWATCH_DAC_IIS_WS (25)
// #define TWATCH_DAC_IIS_DOUT (33)

// #define MOTOR_PIN (4)

#define I2C_SDA 10 // For QMC6310 sensors and screens
#define I2C_SCL 11 // For QMC6310 sensors and screens

#define BUTTON_PIN 0
// #define BUTTON_NEED_PULLUP

#define HAS_GPS 0
#undef GPS_RX_PIN
#undef GPS_TX_PIN

#define USE_SX1262
#define USE_SX1268

#define RF95_SCK 3
#define RF95_MISO 4
#define RF95_MOSI 1
#define RF95_NSS 5

#define LORA_DIO0 -1 // a No connect on the SX1262 module
#define LORA_RESET 8
#define LORA_DIO1 9 // SX1262 IRQ
#define LORA_DIO2 7 // SX1262 BUSY
#define LORA_DIO3   // Not connected on PCB, but internally on the TTGO SX1262, if DIO3 is high the TXCO is enabled

#define SX126X_CS RF95_NSS // FIXME - we really should define LORA_CS instead
#define SX126X_DIO1 LORA_DIO1
#define SX126X_BUSY LORA_DIO2
#define SX126X_RESET LORA_RESET
#define SX126X_E22 // Not really an E22 but TTGO seems to be trying to clone that
// Internally the TTGO module hooks the SX1262-DIO2 in to control the TX/RX switch (which is the default for the sx1262interface
// code)