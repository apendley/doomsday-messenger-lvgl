#include "BatteryMonitor.h"

// #define LOGGER Serial
#include "Logger.h"

bool BatteryMonitor::begin() {
    if (!monitor.begin()) {
        return false;
    }
    
#if defined(USE_LC709203)
    LOGLN("Using LC709203 battery monitor");

    monitor.setPackSize(LC_BATTERY_CAPACITY);
    monitor.setThermistorB(3950);        
    monitor.setAlarmVoltage(3.8);        
#else
    LOGLN("Using MAX17048 battery monitor");

    // Need to give it a bit of time or else we'll just get zeros on the first read.
    delay(100);
#endif

    percentage = readPercentage();

    LOGFMT("Battery: %d%d\n", percentage);

    lastUpdate = millis();
    isInitialized = true;

    return true;
}

void BatteryMonitor::update() {
    if (!isInitialized) {
        return;
    }

    uint32_t now = millis();

    if (now - lastUpdate < updateIntervalMS) {
        return;
    }

    lastUpdate = now;

    const uint8_t newPercentage = readPercentage();

    if (newPercentage != percentage) {
        percentage = newPercentage;

        if (percentChangedCallback) {
            percentChangedCallback(percentage);
        }
    }
}
