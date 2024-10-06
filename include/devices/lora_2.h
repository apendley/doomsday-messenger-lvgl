#pragma once

// If defined, LoRa radio will be used. Otherwise, the Feather's WiFi radio will be used (ESP-NOW).
#define USE_LORA
#define MY_LORA_ADDRESS         0x2A
#define OTHER_LORA_ADDRESS      0x13

// LoRa hardware definitions.
// You'll probably need to change these depending on which pins you soldered together
// on the LoRa Featherwing, which Feather you're using, and which country you live in.
#define RFM_FREQ        915.0
#define RFM_IRQ         16
#define RFM_CS          15
#define RFM_RST         14

// Other messenger MAC address
#define OTHER_MAC_ADDRESS       0x7C, 0xDF, 0xA1, 0x94, 0x8D, 0x80

// Both LoRa and ESP-NOW use this key. Must be 16 bytes.
#define PRIMARY_KEY     "0123456789ABCDEF"

// ESP-NOW local encryption key (must be 16 bytes)
#define LOCAL_KEY       "0123456789ABCDEF"

// Messenger name (for displaying message history)
#define MY_NAME         "Me"
#define OTHER_NAME      "Them"

// Touch screen calibration values
#define TS_MINX     160
#define TS_MAXX     3850
#define TS_MINY     150
#define TS_MAXY     3950

// Older ESP32-ish Feathers have the discontinued LC709203
// battery monitor, while more recent Feathers use the MAX17048.
// Uncomment the define below to use the older LC709203 monitor.
#define USE_LC709203

// If using the LC709203, set the capacity here to the closest matching your actual battery.
// Choices are: 100, 200, 500, 1000, 2000, 3000
#if defined(USE_LC709203)
#define LC_BATTERY_CAPACITY LC709203F_APA_2000MAH
#endif