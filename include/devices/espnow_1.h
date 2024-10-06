#pragma once

// Other messenger MAC address
#define OTHER_MAC_ADDRESS       0x7C, 0xDF, 0xA1, 0x94, 0x8A, 0x06

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