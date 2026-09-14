#include "displayapp/screens/settings/SettingLockScreen.h"
#include <lvgl/lvgl.h>
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/Symbols.h"

using namespace Pinetime::Applications::Screens;

namespace {
  void EventHandler(lv_obj_t* obj, lv_event_t event) {
    auto* screen = static_cast<SettingLockScreen*>(obj->user_data);
    if (event == LV_EVENT_VALUE_CHANGED) {
      screen->Toggled();
    }
  }
}

SettingLockScreen::SettingLockScreen(Pinetime::Controllers::Settings& settingsController) : settingsController {settingsController} {
  lv_obj_t* title = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(title, "Lock screen");
  lv_label_set_align(title, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(title, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 15, 15);

  lv_obj_t* icon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_ORANGE);
  lv_label_set_text_static(icon, Symbols::lock);
  lv_label_set_align(icon, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(icon, title, LV_ALIGN_OUT_LEFT_MID, -10, 0);

  checkbox = lv_checkbox_create(lv_scr_act(), nullptr);
  lv_checkbox_set_text_static(checkbox, "Use lock screen");
  lv_checkbox_set_checked(checkbox, settingsController.GetLockScreenEnabled());
  checkbox->user_data = this;
  lv_obj_set_event_cb(checkbox, EventHandler);
  lv_obj_align(checkbox, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 10, 60);
}

SettingLockScreen::~SettingLockScreen() {
  lv_obj_clean(lv_scr_act());
  settingsController.SaveSettings();
}

void SettingLockScreen::Toggled() {
  settingsController.SetLockScreenEnabled(lv_checkbox_is_checked(checkbox));
}
