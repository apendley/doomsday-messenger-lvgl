#pragma once

// Uncomment to wait for serial console on boot.
// Don't forget to comment again when you finish debugging.
// #define WAIT_FOR_SERIAL_CONSOLE_ON_BOOT

// Keyboard Featherwing pin definitions for the Feather ESP32-S2.
// You might need to change these if you use a different microcontroller.
#define SD_CS           5
#define KEYBOARD_IRQ    6
#define TFT_CS          9
#define TFT_DC          10
#define NEOPIXEL_PIN    11

// This includes the device-specific configuration file.
#include CONFIG_FILE

// Automatically define these to invalid values if not defined in device-specific config.
#if !defined(USE_LORA)
#define MY_LORA_ADDRESS         0xFF
#define OTHER_LORA_ADDRESS      0xFF

#define RFM_FREQ        0.0
#define RFM_IRQ         -1
#define RFM_CS          -1
#define RFM_RST         -1
#endif