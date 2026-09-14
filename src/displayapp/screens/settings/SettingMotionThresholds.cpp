#include "displayapp/screens/settings/SettingMotionThresholds.h"
#include <iterator>
#include <lvgl/lvgl.h>
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;
using Gesture = SettingMotionThresholds::Gesture;

namespace {
  struct Row {
    const char* name;
    uint16_t min;
    uint16_t max;
  };

  // Names are kept short on purpose: the label and its value share a 122 px
  // column at 12 px per character, so ten characters is the whole budget.
  constexpr Row raiseRows[] = {
    {"Roll", 10, 90},
    {"Still", 8, 200},
    {"Level", 64, 1024},
    {"Tilt", 0, 256},
    {"Win", 3, 16},
    {"Set", 1, 4},
  };

  constexpr Row lowerRows[] = {
    {"SideL", 512, 1024},
    {"SideR", 5, 90},
    {"Face", 256, 1024},
    {"Roll", 5, 90},
    {"Floor", 0, 1024},
  };

  const Row* RowsFor(Gesture gesture) {
    return gesture == Gesture::RaiseWrist ? raiseRows : lowerRows;
  }

  uint8_t RowCountFor(Gesture gesture) {
    return gesture == Gesture::RaiseWrist ? std::size(raiseRows) : std::size(lowerRows);
  }

  const char* TitleFor(Gesture gesture) {
    return gesture == Gesture::RaiseWrist ? "Raise wrist" : "Lower wrist";
  }

  void SpreadRaise(const Pinetime::Controllers::RaiseWakeThresholds& thresholds, uint16_t* values) {
    values[0] = thresholds.rollDegrees;
    values[1] = thresholds.stillness;
    values[2] = thresholds.level;
    values[3] = thresholds.tilt;
    values[4] = thresholds.window;
    values[5] = thresholds.settle;
  }

  Pinetime::Controllers::RaiseWakeThresholds GatherRaise(const uint16_t* values) {
    return {static_cast<uint8_t>(values[0]),
            static_cast<uint8_t>(values[1]),
            values[2],
            values[3],
            static_cast<uint8_t>(values[4]),
            static_cast<uint8_t>(values[5])};
  }

  void SpreadLower(const Pinetime::Controllers::LowerSleepThresholds& thresholds, uint16_t* values) {
    values[0] = thresholds.sideLevel;
    values[1] = thresholds.sideRollDegrees;
    values[2] = thresholds.facingLevel;
    values[3] = thresholds.rollDegrees;
    values[4] = thresholds.historyFloor;
  }

  Pinetime::Controllers::LowerSleepThresholds GatherLower(const uint16_t* values) {
    return {values[0], static_cast<uint8_t>(values[1]), values[2], static_cast<uint8_t>(values[3]), values[4]};
  }

  void SliderEventHandler(lv_obj_t* obj, lv_event_t event) {
    auto* screen = static_cast<SettingMotionThresholds*>(obj->user_data);
    if (event == LV_EVENT_VALUE_CHANGED) {
      screen->OnSliderChanged(obj);
    }
  }

  void ResetEventHandler(lv_obj_t* obj, lv_event_t event) {
    auto* screen = static_cast<SettingMotionThresholds*>(obj->user_data);
    if (event == LV_EVENT_CLICKED) {
      screen->OnReset();
    }
  }

  constexpr lv_coord_t labelWidth = 122;
  constexpr lv_coord_t sliderWidth = 102;
  constexpr lv_coord_t firstRowTop = 26;
  constexpr lv_coord_t rowHeight = 29;
}

SettingMotionThresholds::SettingMotionThresholds(Pinetime::Controllers::Settings& settingsController, Gesture gesture)
  : settingsController {settingsController}, gesture {gesture}, rowCount {RowCountFor(gesture)} {

  if (gesture == Gesture::RaiseWrist) {
    SpreadRaise(settingsController.GetRaiseWakeThresholds(), values);
  } else {
    SpreadLower(settingsController.GetLowerSleepThresholds(), values);
  }

  lv_obj_t* title = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(title, TitleFor(gesture));
  lv_label_set_align(title, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(title, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 0);

  const Row* rows = RowsFor(gesture);
  for (uint8_t index = 0; index < rowCount; index++) {
    const lv_coord_t rowTop = firstRowTop + index * rowHeight;

    labels[index] = lv_label_create(lv_scr_act(), nullptr);
    lv_label_set_long_mode(labels[index], LV_LABEL_LONG_CROP);
    lv_obj_set_width(labels[index], labelWidth);
    lv_obj_set_pos(labels[index], 2, rowTop);

    sliders[index] = lv_slider_create(lv_scr_act(), nullptr);
    sliders[index]->user_data = this;
    lv_obj_set_event_cb(sliders[index], SliderEventHandler);
    lv_slider_set_range(sliders[index], rows[index].min, rows[index].max);
    lv_slider_set_value(sliders[index], values[index], LV_ANIM_OFF);
    lv_obj_set_size(sliders[index], sliderWidth, 12);
    lv_obj_set_pos(sliders[index], LV_HOR_RES - sliderWidth - 4, rowTop + 5);

    UpdateRow(index);
  }

  resetButton = lv_btn_create(lv_scr_act(), nullptr);
  resetButton->user_data = this;
  lv_obj_set_event_cb(resetButton, ResetEventHandler);
  lv_obj_set_size(resetButton, 110, 36);
  lv_obj_align(resetButton, lv_scr_act(), LV_ALIGN_IN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_local_bg_color(resetButton, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, Colors::bgAlt);

  lv_obj_t* resetLabel = lv_label_create(resetButton, nullptr);
  lv_label_set_text_static(resetLabel, "Reset");
}

SettingMotionThresholds::~SettingMotionThresholds() {
  lv_obj_clean(lv_scr_act());
  settingsController.SaveSettings();
}

void SettingMotionThresholds::UpdateRow(uint8_t index) {
  lv_label_set_text_fmt(labels[index], "%s %d", RowsFor(gesture)[index].name, values[index]);
}

void SettingMotionThresholds::StoreToSettings() {
  if (gesture == Gesture::RaiseWrist) {
    settingsController.SetRaiseWakeThresholds(GatherRaise(values));
  } else {
    settingsController.SetLowerSleepThresholds(GatherLower(values));
  }
}

void SettingMotionThresholds::OnSliderChanged(lv_obj_t* object) {
  for (uint8_t index = 0; index < rowCount; index++) {
    if (sliders[index] == object) {
      values[index] = lv_slider_get_value(object);
      UpdateRow(index);
      StoreToSettings();
      return;
    }
  }
}

void SettingMotionThresholds::OnReset() {
  if (gesture == Gesture::RaiseWrist) {
    SpreadRaise(Pinetime::Controllers::raiseWakeDefaults, values);
  } else {
    SpreadLower(Pinetime::Controllers::lowerSleepDefaults, values);
  }
  for (uint8_t index = 0; index < rowCount; index++) {
    lv_slider_set_value(sliders[index], values[index], LV_ANIM_OFF);
    UpdateRow(index);
  }
  StoreToSettings();
}
