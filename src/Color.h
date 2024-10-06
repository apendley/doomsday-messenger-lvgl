#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "hsv2rgb.h"

namespace Color {
    uint8_t sine8(uint8_t theta);
    uint8_t gamma8(uint8_t x);
}

namespace Color {
    struct RGB {
        inline RGB() : r(0), g(0), b(0) { }
        inline RGB(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) { }
        inline RGB(uint32_t packedColor) : r(packedColor >> 16), g(packedColor >> 8), b(packedColor) { }
        inline RGB(const lv_color_t& c) : r(c.red), g(c.green), b(c.blue) { }

        static RGB fromPacked565(uint16_t packed) {
            uint8_t red   = (packed >> 11) & 0x1F;
            uint8_t green = (packed >> 5) & 0x3F;
            uint8_t blue  = packed & 0x1F;

            uint8_t red8   = (red << 3) | (red >> 2);
            uint8_t green8 = (green << 2) | (green >> 4);
            uint8_t blue8  = (blue << 3) | (blue >> 2);
            
            return Color::RGB(red8, green8, blue8);
        }

        static RGB gray(uint8_t brightness) {
            return RGB(brightness, brightness, brightness);
        }

        inline bool operator==(const RGB& other) const {
            return r == other.r && g == other.g && b == other.b;
        }

        inline bool isBlack() const {
            return (r == 0) && (g == 0) & (b == 0);
        }

        inline uint32_t packed() const {
            return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        }

        inline uint16_t packed565() const {
            return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        void interpolate(const RGB& toColor, float t) {
            t = min(1.0f, max(0.0f, t));
            r = r + (((int)(toColor.r - r) * (1.0f - t)));
            g = g + (((int)(toColor.g - g) * (1.0f - t)));
            b = b + (((int)(toColor.b - b) * (1.0f - t)));
        }        

        RGB interpolated(const RGB& toColor, float t) {
            RGB ret = (*this);
            ret.interpolate(toColor, t);
            return ret;
        }

        // 0.0 = original color, 1.0 = black
        void darkenTo(float t) {
            interpolate(RGB(0), 1.0 - t);
        }

        RGB darkenedTo(float t) {
            RGB ret = (*this);
            ret.darkenTo(t);
            return ret;
        }

        RGB darkenedTo8(uint8_t t) {
            RGB ret = (*this);
            ret.darkenTo((float)t / 255.0f);
            return ret;
        }        

        // 0.0 = original color, 1.0 = white
        void lightenTo(float t) {
            interpolate(RGB(255, 255, 255), 1.0 - t);
        }

        RGB lightenedTo(float t) {
            RGB ret = (*this);
            ret.lightenTo(t);
            return ret;
        }

        RGB lightenedTo8(uint8_t t) {
            RGB ret = (*this);
            ret.lightenTo((float)t / 255.0f);
            return ret;
        }        

        inline RGB gammaApplied() const {
            return RGB(gamma8(r), gamma8(g), gamma8(b));
        }

        static inline RGB fromHexString(const char* colorString) {
            if (colorString == nullptr || strlen(colorString) < 7) { 
                return RGB();
            }

            char colorBuffer[3] = {0};
            colorBuffer[2] = 0;

            // skip the #                    
            uint8_t i = 1;

            // First two bytes are red
            colorBuffer[0] = colorString[i++];
            colorBuffer[1] = colorString[i++];
            uint8_t red = strtol(colorBuffer, NULL, 16);

            // Next two bytes are green
            colorBuffer[0] = colorString[i++];
            colorBuffer[1] = colorString[i++];
            uint8_t green = strtol(colorBuffer, NULL, 16);

            // Next two bytes are blue
            colorBuffer[0] = colorString[i++];
            colorBuffer[1] = colorString[i++];
            uint8_t blue = strtol(colorBuffer, NULL, 16);                

            return RGB(red, green, blue);
        }    

        inline lv_color_t toLV() {
            return lv_color_make(r, g, b);
        }       

        uint8_t r;
        uint8_t g;
        uint8_t b;
    } __attribute__((packed));

    struct HSV {
        inline HSV(uint16_t h, uint8_t s = 255, uint8_t v = 255) 
            : h(h), s(s), v(v)
        {
        }

        HSV withValue(uint8_t value) {
            return HSV(h, s, value);
        }

        static inline HSV fromRGB(uint8_t r, uint8_t g, uint8_t b) {
            uint8_t rangeMin = min(min(r, g), b);
            uint8_t rangeMax = max(max(r, g), b);
            
            uint8_t v = rangeMax;            
            if (v == 0) {
                return HSV(0, 0, 0);
            }

            uint8_t s = 255 * long(rangeMax - rangeMin) / v;
            if (s == 0) {
                return HSV(0, 0, v);
            }

            uint16_t h;            
        
            if (rangeMax == r) {
                h = 0 + 11008 * (g - b) / (rangeMax - rangeMin);
            }
            else if (rangeMax == g) {
                h = 21760 + 11008 * (b - r) / (rangeMax - rangeMin);
            }
            else {
                h = 43776 + 11008 * (r - g) / (rangeMax - rangeMin);
            }
        
            return HSV(h, s, v);
        }

        static inline HSV fromRGB(RGB rgb) {
            return fromRGB(rgb.r, rgb.g, rgb.b);
        }

        inline RGB toRGB() const {
            uint8_t r, g, b;
            hsv2rgb(h, s, v, r, g, b);
            return RGB(r, g, b);
        }

        uint16_t h;
        uint8_t s;
        uint8_t v;
    } __attribute__((packed));
}
