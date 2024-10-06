#pragma once

#include <Arduino.h>
#include <BBQ10Keyboard.h>
#include "Device.h"

class SceneManager;

class Scene {
public:
    Scene(Device& d) : device(d) {}
    virtual ~Scene() = default;

    // Scene can now add child views to its 'screen' property.
    // However, final layout has not yet been performed.
    virtual void willLoadScreen() {}

    // Screen is loaded and final layout has been performed.
    virtual void didLoadScreen() {}

    // Screen is about to be unloaded
    virtual void willUnloadScreen() {}

    // Called once every device update loop.
    virtual void update(uint32_t dt) {}

    // Called whenever the device receives a text mesage.
    virtual void receivedMessage(const char* message) {}

protected:
    Device& device;
    lv_obj_t* screen = nullptr;

    // So the scene manager can set the scene's screen.
    friend SceneManager;
};