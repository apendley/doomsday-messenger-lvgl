#include "Settings.h"

// #define LOGGER Serial
#include "Logger.h"

void Settings::setDefaults() {
    memoryMap.version = currentVersion;
    memoryMap.configuredRadio = configuredRadioType;    

#if defined(USE_LORA)
    memoryMap.activeRadio = RadioType::lora;
#else
    memoryMap.activeRadio = RadioType::espNow;
#endif    

    memcpy(memoryMap.pmk, PRIMARY_KEY, 16);
    memcpy(memoryMap.lmk, LOCAL_KEY, 16);

    const uint8_t addr[6] = {OTHER_MAC_ADDRESS};
    memcpy(memoryMap.otherMacAddress, addr, 6);
    
    memoryMap.myLoraAddress = MY_LORA_ADDRESS;
    memoryMap.otherLoraAddress = OTHER_LORA_ADDRESS;

    memoryMap.displayBrightness = 50;
    memoryMap.keyboardBrightness = 5;
}

void Settings::setPMK(const char key[16]) {
    memcpy(memoryMap.pmk, key, pmkSize);
}

void Settings::pmk(pmk_t& keyBuffer) const {
    memcpy(keyBuffer, memoryMap.pmk, pmkSize);
}

void Settings::pmkString(pmk_string_t& keyBuffer) const {
    memcpy(keyBuffer, memoryMap.pmk, pmkSize);
    keyBuffer[pmkSize] = 0;
}

void Settings::setLMK(const char key[16]) {
    memcpy(memoryMap.lmk, key, lmkSize);
}

void Settings::lmk(lmk_t& keyBuffer) const {
    memcpy(keyBuffer, memoryMap.lmk, lmkSize);
}

void Settings::lmkString(lmk_string_t& keyBuffer) const {
    memcpy(keyBuffer, memoryMap.lmk, lmkSize);
    keyBuffer[lmkSize] = 0;
}

void Settings::logCurrent() const {
#if defined(LOGGER)
    LOGLN("--------------------------\nSettings:\n--------------------------");
    LOGFMT("version: %d\n", version());
    LOGFMT("base radio: %s\n", (configuredRadio() == RadioType::espNow) ? "ESP-NOW" : "LoRa");

#if defined (USE_LORA)
    LOGFMT("Active radio: %s\n", (activeRadio() == RadioType::espNow) ? "ESP-NOW" : "LoRa");

    LOGFMT("My LoRa address: %02X\n", myLoraAddress());
    LOGFMT("Paired LoRa address: %02X\n", otherLoraAddress());
#endif    

    MacAddress otherMac(otherMacAddress());

    LOGFMT("Paired MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
        otherMac.rawAddress[0],
        otherMac.rawAddress[1],
        otherMac.rawAddress[2],
        otherMac.rawAddress[3],
        otherMac.rawAddress[4],
        otherMac.rawAddress[5]
    );

    Settings::pmk_string_t pmkStr;
    pmkString(pmkStr);
    LOGFMT("pmk: %s\n", pmkStr);

    Settings::lmk_string_t lmkStr;
    lmkString(lmkStr);
    LOGFMT("lmk: %s\n", lmkStr);

    LOGFMT("Display brightness: %d%%\n", displayBrightness());
    LOGFMT("Keyboard brightness: %d%%\n", keyboardBrightness());

    LOGLN("--------------------------\n");    

#endif    
}