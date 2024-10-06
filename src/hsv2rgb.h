// Adapted by Aaron Pendley from Adafruit_NeoPixel library. 

// Original license:
/*!
 * @section author Author
 *
 * Written by Phil "Paint Your Dragon" Burgess for Adafruit Industries,
 * with contributions by PJRC, Michael Miller and other members of the
 * open source community.
 *
 * @section license License
 *
 * This file is part of the Adafruit_NeoPixel library.
 *
 * Adafruit_NeoPixel is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Adafruit_NeoPixel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with NeoPixel. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <Arduino.h>

inline void hsv2rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b) {
    uint16_t hue = (h * 1530L + 32768) / 65536;

    // Red to Green-1
    if (hue < 510) {
        b = 0;

        // Red to Yellow-1
        if (hue < 255) {
            r = 255;
            // g = 0 to 254
            g = hue;
        } 
        // Yellow to Green-1
        else {
            // r = 255 to 1
            r = 510 - hue;
            g = 255;
        }
    } 
    // Green to Blue-1
    else if (hue < 1020) { 
        r = 0;

        // Green to Cyan-1
        if (hue < 765) {
            g = 255;
            // b = 0 to 254
            b = hue - 510;
        } 
        // Cyan to Blue-1
        else {
            // g = 255 to 1
            g = 1020 - hue;
            b = 255;
        }
    } 
    // Blue to Red-1
    else if (hue < 1530) {
        g = 0;

        // Blue to Magenta-1
        if (hue < 1275) {
            // r = 0 to 254
            r = hue - 1020;
            b = 255;
        } 
        // Magenta to Red-1
        else {
            r = 255;
            // b = 255 to 1
            b = 1530 - hue;
        }
    } 
    // Last 0.5 Red (quicker than % operator)
    else {
        r = 255;
        g = b = 0;
    }

    // Apply saturation and value to R,G,B, pack into 32-bit result:
    uint32_t v1 = 1 + v;  // 1 to 256; allows >>8 instead of /255
    uint16_t s1 = 1 + s;  // 1 to 256; same reason
    uint8_t s2 = 255 - s; // 255 to 0

    r = ((((r * s1) >> 8) + s2) * v1) >> 8;
    g = ((((g * s1) >> 8) + s2) * v1) >> 8;
    b = ((((b * s1) >> 8) + s2) * v1) >> 8;
}