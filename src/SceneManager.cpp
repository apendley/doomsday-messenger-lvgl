
#include "SceneManager.h"
#include "Scene.h"
#include <lvgl.h>

SceneManager::~SceneManager() {
    if (currentScene != nullptr) {
        delete currentScene;
        currentScene = nullptr;
    }
}

void SceneManager::update(uint32_t dt) {
    if (currentScene != nullptr) {
        currentScene->update(dt);
    }
}

void SceneManager::gotoScene(Scene* s) {
    if (currentScene != nullptr) {
        currentScene->willUnloadScreen();
        delete currentScene;
    }

    // The next scene is now the current scene.
    currentScene = s;

    // Gonna need this in a sec
    lv_obj_t* previousScreen = lv_screen_active();

    // Create screne and provide default setup.
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, Device::displayWidth, Device::displayHeight);
    lv_obj_set_pos(screen, 0, 0);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    if (currentScene == nullptr) {
        // Load default empty screen
        lv_screen_load(screen);
    }
    else {
        // Our scene now can access the screen.
        currentScene->screen = screen;

        // Let the current scene set up the screen and then load it.
        currentScene->willLoadScreen();
        lv_screen_load(screen);

        // Lay the new screen out
        lv_obj_update_layout(screen);

        // And let the scene know we finished loading and laying out the screen.
        currentScene->didLoadScreen();
    }    

    // Delete the previous screen
    if (previousScreen != nullptr) {
        lv_obj_del(previousScreen);
    }
}

void SceneManager::receivedMessage(const char* message) {
    if (currentScene != nullptr) {
        currentScene->receivedMessage(message);
    }
}