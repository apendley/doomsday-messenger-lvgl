#pragma once

#include <Arduino.h>
#include "MacAddress.h"
#include "config.h"

struct Settings {
    //
    // Constants
    //
    static uint16_t constexpr currentVersion = 1;

    enum class RadioType: uint8_t {
        espNow,
        lora
    };    

#if defined(USE_LORA)
    static constexpr RadioType configuredRadioType = RadioType::lora;
#else
    static constexpr RadioType configuredRadioType = RadioType::espNow;
#endif

    static constexpr size_t macSize = 6;

    static constexpr size_t pmkSize = 16;
    typedef uint8_t pmk_t[pmkSize];

    static constexpr size_t pmk_string_size = pmkSize + 1;
    typedef char pmk_string_t[pmk_string_size];

    static constexpr size_t lmkSize = 16;
    typedef uint8_t lmk_t[lmkSize];

    static constexpr size_t lmk_string_size = pmkSize + 1;    
    typedef char lmk_string_t[lmk_string_size];

    static constexpr uint8_t CHANGE_PRIMARY_KEY    = (1 << 0);
    static constexpr uint8_t CHANGE_LOCAL_KEY      = (1 << 1);
    static constexpr uint8_t CHANGE_MY_ADDRESS     = (1 << 2);
    static constexpr uint8_t CHANGE_OTHER_ADDRESS  = (1 << 3);

    //
    // File memory map
    //
    struct MemoryMap {
        // Save file version. If file's does not match the currentVersion, it's outdated.
        uint16_t version = 0;

        // The configuration this device was built with. If the file's configuredRadio doesn't
        // match configuredRadioType, the save file was created for a device with a different
        // hardware configuration (e.g. USE_LORA was enabled, built, ran on device, and then
        // was disabled, built, and ran on device).
        RadioType configuredRadio = configuredRadioType;

        // The radio currently in use by the device.
        // If USE_LORA is enabled, this will enable the LoRa radio by defualt.
        // Otherwise, the ESP-NOW radio will be enabled by default.
        // If USE_LORA is enabled, this can be switched in the settings screen.
        RadioType activeRadio = RadioType::espNow;

        // Display and keyboard brightness
        uint8_t displayBrightness = 0;
        uint8_t keyboardBrightness = 0;        

        // Primary encryption key (ESP-NOW and LoRa)
        uint8_t pmk[pmkSize] = {0};

        // Local encryption key (ESP-NOW only)
        uint8_t lmk[lmkSize] = {0};

        // Mac address of the paired device (ESP-NOW)
        uint8_t otherMacAddress[macSize] = {0};

        // LoRa address for this device (LoRa only)
        uint8_t myLoraAddress = 0xFF;

        // LoRa address for paired device (LoRa only)
        uint8_t otherLoraAddress = 0xFF;
    } __attribute__((packed));

    //
    // Interface to underlying memory map.
    //
    void setDefaults();

    void logCurrent() const;

    uint16_t version() const {
        return memoryMap.version;
    }

    RadioType configuredRadio() const {
        return memoryMap.configuredRadio;
    }

    void setPMK(const char key[16]);
    void pmk(pmk_t&) const;
    void pmkString(pmk_string_t &) const;

    void setLMK(const char key[16]);
    void lmk(lmk_t&) const;
    void lmkString(lmk_string_t &) const;

    void setOtherMacAddress(const MacAddress& mac) {
        memcpy(memoryMap.otherMacAddress, mac.rawAddress, macSize);
    }

    MacAddress otherMacAddress() const {
        return MacAddress(memoryMap.otherMacAddress);
    }

    void setOtherLoraAddress(uint8_t addr) {
        memoryMap.otherLoraAddress = addr;
    }

    inline uint8_t otherLoraAddress() const {
        return memoryMap.otherLoraAddress;
    }

    void setMyLoraAddress(uint8_t addr) {
        memoryMap.myLoraAddress = addr;
    }    

    inline uint8_t myLoraAddress() const {
        return memoryMap.myLoraAddress;
    }

    void setDisplayBrightness(uint8_t b) {
        memoryMap.displayBrightness = b;
    }

    uint8_t displayBrightness() const {
        return memoryMap.displayBrightness;
    }

    void setKeyboardBrightness(uint8_t b) {
        memoryMap.keyboardBrightness = b;
    }

    uint8_t keyboardBrightness() const {
        return memoryMap.keyboardBrightness;
    }

    void setActiveRadio(RadioType r) {
        memoryMap.activeRadio = r;
    }

    RadioType activeRadio() const {
        return memoryMap.activeRadio;
    }

    MemoryMap memoryMap;
};