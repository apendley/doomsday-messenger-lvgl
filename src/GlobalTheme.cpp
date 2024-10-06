#include "GlobalTheme.h"
#include "Device.h"
#include "Color.h"

void GlobalTheme::init() {
    // Default text
    lv_style_init(&bodyText);
    lv_style_set_text_color(&bodyText, globalTheme.amber);

    // Status bar
    lv_style_init(&statusBarText);
    lv_style_set_text_color(&statusBarText, amber);
    lv_style_set_width(&statusBarText, LV_SIZE_CONTENT);
    lv_style_set_text_font(&statusBarText, &lv_font_montserrat_12);

    // Title bar
    lv_style_init(&titleBg);
    lv_style_set_bg_color(&titleBg, amber);
    lv_style_set_border_width(&titleBg, 0);
    lv_style_set_radius(&titleBg, 0);
    lv_style_set_size(&titleBg, Device::displayWidth, 30);

    lv_style_init(&titleText);
    lv_style_set_bg_color(&titleText, amber);
    lv_style_set_text_font(&titleText, &lv_font_montserrat_24);
    lv_style_set_size(&titleText, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    
    // "Button bar"
    lv_style_init(&buttonBarBg);
    lv_style_set_bg_color(&buttonBarBg, Color::RGB(amber).darkenedTo(0.875).toLV());
    lv_style_set_size(&buttonBarBg, Device::displayWidth, 30);
    lv_style_set_align(&buttonBarBg, LV_ALIGN_BOTTOM_MID);
    lv_style_set_border_width(&buttonBarBg, 0);
    lv_style_set_shadow_width(&buttonBarBg, 0);
    lv_style_set_radius(&buttonBarBg, 0);

    lv_style_init(&barButton);
    lv_style_set_pad_left(&barButton, 6);
    lv_style_set_pad_right(&barButton, 6);    
    lv_style_set_height(&barButton, 21);    
    lv_style_set_radius(&barButton, 10);
    lv_style_set_shadow_width(&barButton, 0);
    lv_style_set_bg_color(&barButton, lv_color_black());
    lv_style_set_width(&barButton, LV_SIZE_CONTENT);
    lv_style_set_border_width(&barButton, 2);
    lv_style_set_border_color(&barButton, amber);
    lv_style_set_text_color(&barButton, globalTheme.amber);

    lv_style_t* barButtonStyles[Device::barButtonCount] = {
        &barButtonIndicatorOne,
        &barButtonIndicatorTwo,
        &barButtonIndicatorThree,
        &barButtonIndicatorFour
    };
    
    for (int bb = 0; bb < Device::barButtonCount; bb++) {
        lv_style_t* style = barButtonStyles[bb];

        lv_style_init(style);
        lv_style_set_border_width(style, 0);
        lv_style_set_bg_color(style, amber);
        lv_style_set_text_color(style, lv_color_black());      
        lv_style_set_size(style, 4, 4);
        const lv_point_t& indicatorPos = Device::getBarButtonIndicatorPosition(Device::BarButton(bb));
        lv_style_set_x(style, indicatorPos.x);
        lv_style_set_y(style, indicatorPos.y);        
    }

    // Default button styles
    lv_style_init(&standardButton);
    lv_style_set_height(&standardButton, 36);
    lv_style_set_radius(&standardButton, 18);
    lv_style_set_shadow_width(&standardButton, 0);
    lv_style_set_bg_color(&standardButton, lv_color_black());
    lv_style_set_width(&standardButton, LV_SIZE_CONTENT);
    lv_style_set_border_width(&standardButton, 2);
    lv_style_set_border_color(&standardButton, amber);
    lv_style_set_text_color(&standardButton, amber);

    lv_style_init(&pressedButton);
    lv_style_set_bg_color(&pressedButton, amber);
    lv_style_set_text_color(&pressedButton, lv_color_black());

    // Remove animation from the color properties of all button styles so the state change is immediate and apparent.
    static const lv_style_prop_t buttonTransitionPropertyOverrides[] = {LV_STYLE_TEXT_COLOR, LV_STYLE_BG_COLOR, 0};
    static lv_style_transition_dsc_t releasedButtonTransition;    
    lv_style_transition_dsc_init(&releasedButtonTransition, buttonTransitionPropertyOverrides, lv_anim_path_linear, 0, 0, nullptr);
    lv_style_set_transition(&barButton, &releasedButtonTransition);
    lv_style_set_transition(&standardButton, &releasedButtonTransition);

    static const lv_style_prop_t pressedButtonTransitionProperties[] = {LV_STYLE_TEXT_COLOR, LV_STYLE_BG_COLOR, 0};
    static lv_style_transition_dsc_t pressedButtonTransition;
    lv_style_transition_dsc_init(&pressedButtonTransition, buttonTransitionPropertyOverrides, lv_anim_path_linear, 0, 0, nullptr);
    lv_style_set_transition(&pressedButton, &pressedButtonTransition);            

    // Slider
    lv_style_init(&sliderTrack);
    lv_style_set_bg_color(&sliderTrack, globalTheme.amber);

    lv_style_init(&sliderKnob);
    lv_style_set_bg_color(&sliderKnob, globalTheme.amber);
    lv_style_set_pad_all(&sliderKnob, 6);

    lv_style_init(&sliderFocus);
    lv_style_set_outline_color(&sliderFocus, globalTheme.amber);

    // Tab view
    lv_style_init(&tabViewContainerStyle);
    lv_style_set_bg_color(&tabViewContainerStyle, lv_color_black());
    lv_style_set_border_width(&tabViewContainerStyle, 0);
    lv_style_set_outline_width(&tabViewContainerStyle, 0);
    lv_style_set_radius(&tabViewContainerStyle, 0);
    lv_style_set_pad_all(&tabViewContainerStyle, 1);

    lv_style_init(&tabViewStyle);
    lv_style_set_border_width(&tabViewStyle, 0);
    lv_style_set_bg_color(&tabViewStyle, lv_color_black());
    lv_style_set_outline_width(&tabViewStyle, 0);
    lv_style_set_radius(&tabViewStyle, 0);

    lv_style_init(&tabButtonsStyle);
    lv_style_set_bg_color(&tabButtonsStyle, Color::RGB(globalTheme.amber).darkenedTo(0.75).toLV());
    lv_style_set_text_color(&tabButtonsStyle, globalTheme.amber);

    lv_style_init(&tabButtonsSelectedStyle);
    lv_style_set_text_color(&tabButtonsSelectedStyle, globalTheme.amber);    
    lv_style_set_bg_color(&tabButtonsSelectedStyle, globalTheme.amber);
    lv_style_set_border_color(&tabButtonsSelectedStyle, globalTheme.amber);
    lv_style_set_border_side(&tabButtonsSelectedStyle, LV_BORDER_SIDE_RIGHT);        
}