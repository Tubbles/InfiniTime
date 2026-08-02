#include "displayapp/screens/settings/SettingIntercom.h"

#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  void KeyEventHandler(lv_obj_t* obj, lv_event_t event) {
    auto* screen = static_cast<SettingIntercom*>(obj->user_data);
    screen->OnKeyEvent(obj, event);
  }

  constexpr const char* keypadMap[] =
    {"1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n", "*", "0", "#", "\n", "Off", ""};

  // Button ids follow the map order, newlines excluded.
  constexpr uint16_t offButtonId = 12;

  uint16_t ButtonIdForKey(char key) {
    switch (key) {
      case '1':
        return 0;
      case '2':
        return 1;
      case '3':
        return 2;
      case '4':
        return 3;
      case '5':
        return 4;
      case '6':
        return 5;
      case '7':
        return 6;
      case '8':
        return 7;
      case '9':
        return 8;
      case '*':
        return 9;
      case '0':
        return 10;
      case '#':
        return 11;
      default:
        return offButtonId;
    }
  }
}

SettingIntercom::SettingIntercom(Pinetime::Controllers::Settings& settingsController) : settingsController {settingsController} {
  lv_obj_t* title = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(title, "Intercom button");
  lv_label_set_align(title, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(title, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 8);

  keypadMatrix = lv_btnmatrix_create(lv_scr_act(), nullptr);
  keypadMatrix->user_data = this;
  lv_obj_set_event_cb(keypadMatrix, KeyEventHandler);
  lv_btnmatrix_set_map(keypadMatrix, const_cast<const char**>(keypadMap));
  lv_btnmatrix_set_btn_ctrl_all(keypadMatrix, LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_check(keypadMatrix, true);
  lv_obj_set_size(keypadMatrix, LV_HOR_RES, LV_VER_RES - 35);
  lv_obj_set_style_local_bg_color(keypadMatrix, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, Colors::bgAlt);
  lv_obj_set_style_local_bg_color(keypadMatrix, LV_BTNMATRIX_PART_BTN, LV_STATE_CHECKED, Colors::highlight);
  lv_obj_set_style_local_pad_inner(keypadMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_pad_top(keypadMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_pad_bottom(keypadMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_pad_left(keypadMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_pad_right(keypadMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_align(keypadMatrix, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

  lv_btnmatrix_set_btn_ctrl(keypadMatrix, ButtonIdForKey(settingsController.GetIntercomKey()), LV_BTNMATRIX_CTRL_CHECK_STATE);
}

SettingIntercom::~SettingIntercom() {
  lv_obj_clean(lv_scr_act());
  settingsController.SaveSettings();
}

void SettingIntercom::OnKeyEvent(lv_obj_t* obj, lv_event_t event) {
  if (obj != keypadMatrix || event != LV_EVENT_VALUE_CHANGED) {
    return;
  }
  const char* buttonText = lv_btnmatrix_get_active_btn_text(keypadMatrix);
  if (buttonText == nullptr) {
    return;
  }
  if (buttonText[0] == 'O') { // "Off"
    settingsController.SetIntercomKey(0);
  } else {
    settingsController.SetIntercomKey(buttonText[0]);
  }
  // Radio semantics: clicking the already-checked key must not leave the
  // matrix with nothing checked while the setting keeps its value.
  lv_btnmatrix_set_btn_ctrl(keypadMatrix, lv_btnmatrix_get_active_btn(keypadMatrix), LV_BTNMATRIX_CTRL_CHECK_STATE);
}
