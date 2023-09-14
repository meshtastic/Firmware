// #define BUTTON_NEED_PULLUP // if set we need to turn on the internal CPU pullup during sleep

#define I2C_SDA 21
#define I2C_SCL 22

#define I2C_SDA1 14 // Second i2c channel on external IO connector
#define I2C_SCL1 15 // Second i2c channel on external IO connector

#define BUTTON_PIN 36 // The middle button GPIO on the Nano G1
// #define BUTTON_PIN_ALT 13 // Alternate GPIO for an external button if needed. Does anyone use this? It is not documented
//  anywhere.
#define EXT_NOTIFY_OUT 13 // Default pin to use for Ext Notify Module.

// common pinout for their SX1262 vs RF95 modules - both can be enabled and we will probe at runtime for RF95 and if
// not found then probe for SX1262
#define USE_RF95
#define USE_SX1262

#define LORA_DIO0 26 // a No connect on the SX1262 module
#define LORA_RESET 23
#define LORA_DIO1 33 // SX1262 IRQ
#define LORA_DIO2 32 // SX1262 BUSY
#define LORA_DIO3    // Not connected on PCB

#ifdef USE_SX1262
#define SX126X_CS RF95_NSS // FIXME - we really should define LORA_CS instead
#define SX126X_DIO1 LORA_DIO1
#define SX126X_BUSY LORA_DIO2
#define SX126X_RESET LORA_RESET
// #define SX126X_E22 // Not really an E22
//  Internally the module hooks the SX1262-DIO2 in to control the TX/RX switch (which is the default for the sx1262interface
//  code)
#define SX126X_MAX_POWER                                                                                                         \
    16 // Ensure the PA does not exceed the saturation output power. More
       // Info:https://uniteng.com/wiki/doku.php?id=meshtastic:station#rf_design_-_lora_station_edition_g1
#endif

#define BATTERY_PIN 35 // A battery voltage measurement pin, voltage divider connected here to measure battery voltage
#define ADC_CHANNEL ADC1_GPIO35_CHANNEL
#define BATTERY_SENSE_SAMPLES 30 // Set the number of samples, It has an effect of increasing sensitivity.
#define ADC_MULTIPLIER 6.45
#define BAT_FULLVOLT 12600
#define BAT_EMPTYVOLT 8200
#define BAT_CHARGINGVOLT 12600
#define BAT_NOBATVOLT 6690

// different screen
#define USE_SH1106
