#include "Device.h"

const lv_point_t Device::barButtonIndicatorPositions[barButtonCount] = { 
    {6, 236}, 
    {88, 236}, 
    {242, 236}, 
    {314, 236}
};

void Device::setPixelColor(const Color::RGB& c) {
    pixel.fill(c.packed());
    pixel.show();    
}
