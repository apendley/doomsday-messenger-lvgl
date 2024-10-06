#pragma once

#include <Arduino.h>
#include <lvgl.h>

struct GlobalTheme {
public:
    // Call this *after* LVGL is initialized.
    void init();

public:
    const lv_color_t amber = lv_color_make(255, 176, 0);
    const lv_color_t green = lv_color_make(0, 255, 51);

    lv_style_t statusBarText;

    lv_style_t titleText;
    lv_style_t titleBg;

    lv_style_t bodyText;

    lv_style_t buttonBarBg;
    lv_style_t barButtonIndicatorOne;
    lv_style_t barButtonIndicatorTwo;
    lv_style_t barButtonIndicatorThree;
    lv_style_t barButtonIndicatorFour;

    lv_style_t barButton;
    lv_style_t standardButton;
    lv_style_t pressedButton;

    lv_style_t sliderTrack;
    lv_style_t sliderKnob;
    lv_style_t sliderLabel;
    lv_style_t sliderFocus;

    lv_style_t tabViewContainerStyle;
    lv_style_t tabViewStyle;
    lv_style_t tabButtonsStyle;
    lv_style_t tabButtonsSelectedStyle;    
};

// Make sure to define this in main.
extern GlobalTheme globalTheme;