#pragma once

#include "config.h"

#if defined(USE_LC709203)
#include <Adafruit_LC709203F.h>
#else
#include <Adafruit_MAX1704X.h>
#endif

class BatteryMonitor {
public:
    static constexpr uint32_t updateIntervalMS = 3000;

    bool begin();
    void update();

    void setPercentChangedCallback(void (*cb)(uint8_t)) {
        percentChangedCallback = cb;
    }    

    inline uint8_t getPercentage() const {
        return percentage;
    }    

private:

    uint8_t readPercentage() {
        // Sometimes the MAX library gives us values > 100  :|
        float pct = min(monitor.cellPercent(), 100.f);
        return ceil(pct);
    }

    uint16_t readVoltage() {
        return uint16_t(monitor.cellVoltage() * 1000);
    }

private:
#if defined(USE_LC709203)
    Adafruit_LC709203F monitor;
#else
    Adafruit_MAX17048 monitor;
#endif

    void (*percentChangedCallback)(uint8_t) = nullptr;    

    bool isInitialized = false;
    uint32_t lastUpdate = 0;
    uint8_t percentage = 0;
};

