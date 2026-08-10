#include "displayapp/screens/WatchFaceAnalogNumbers.h"
#include <cmath>
#include <limits>
#include <lvgl/lvgl.h>
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/BleIcon.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/NotificationIcon.h"
#include "components/settings/Settings.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  constexpr int16_t HourLength = 70;
  constexpr int16_t MinuteLength = 90;
  // Inside the tick ring: at 100 the wide numerals' corners reached the
  // major ticks' inner ends (~114 from center).
  constexpr int16_t NumeralRadius = 86;

  // LVGL sin isn't constexpr (though it could be if it were C++) so fix size here
  // All the types are hardcoded anyway and would need changing if the size changed
  // sin(90) = 1 so the value of _lv_trigo_sin(90) is the scaling factor
  constexpr int16_t LV_TRIG_SCALE = std::numeric_limits<int16_t>::max(); // = _lv_trigo_sin(90)

  int16_t Cosine(int16_t angle) {
    return _lv_trigo_sin(angle + 90);
  }

  int16_t Sine(int16_t angle) {
    return _lv_trigo_sin(angle);
  }

  int16_t CoordinateXRelocate(int16_t x) {
    return (x + LV_HOR_RES / 2);
  }

  int16_t CoordinateYRelocate(int16_t y) {
    return std::abs(y - LV_HOR_RES / 2);
  }

  lv_point_t CoordinateRelocate(int16_t radius, int16_t angle) {
    return lv_point_t {.x = CoordinateXRelocate(radius * static_cast<int32_t>(Sine(angle)) / LV_TRIG_SCALE),
                       .y = CoordinateYRelocate(radius * static_cast<int32_t>(Cosine(angle)) / LV_TRIG_SCALE)};
  }

  constexpr const char* numeralTexts[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"};
}

WatchFaceAnalogNumbers::WatchFaceAnalogNumbers(Controllers::DateTime& dateTimeController,
                                               const Controllers::Battery& batteryController,
                                               const Controllers::Ble& bleController,
                                               Controllers::NotificationManager& notificationManager,
                                               Controllers::Settings& settingsController)
  : currentDateTime {{}},
    batteryIcon(true),
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController} {

  sHour = 99;
  sMinute = 99;

  // Full 360° scales (the stock Analog spans 300° with the gap at the
  // top, under its lone "12" label). One extra line closes the circle:
  // the first and last coincide.
  minor_scales = lv_linemeter_create(lv_scr_act(), nullptr);
  lv_linemeter_set_scale(minor_scales, 360, 61);
  lv_linemeter_set_angle_offset(minor_scales, 180);
  lv_obj_set_size(minor_scales, 240, 240);
  lv_obj_align(minor_scales, nullptr, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_local_bg_opa(minor_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  lv_obj_set_style_local_scale_width(minor_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_scale_end_line_width(minor_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_scale_end_color(minor_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);

  major_scales = lv_linemeter_create(lv_scr_act(), nullptr);
  lv_linemeter_set_scale(major_scales, 360, 13);
  lv_linemeter_set_angle_offset(major_scales, 180);
  lv_obj_set_size(major_scales, 240, 240);
  lv_obj_align(major_scales, nullptr, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_local_bg_opa(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  lv_obj_set_style_local_scale_width(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 6);
  lv_obj_set_style_local_scale_end_line_width(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_scale_end_color(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);

  for (uint8_t index = 0; index < 12; index++) {
    numerals[index] = lv_label_create(lv_scr_act(), nullptr);
    lv_label_set_align(numerals[index], LV_LABEL_ALIGN_CENTER);
    lv_label_set_text_static(numerals[index], numeralTexts[index]);
    lv_obj_set_style_local_text_color(numerals[index], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_AQUA);
    lv_point_t position = CoordinateRelocate(NumeralRadius, (index + 1) * 30);
    lv_obj_align(numerals[index], nullptr, LV_ALIGN_CENTER, position.x - LV_HOR_RES / 2, position.y - LV_VER_RES / 2);
  }

  batteryIcon.Create(lv_scr_act());
  lv_obj_align(batteryIcon.GetObject(), nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 0);

  plugIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(plugIcon, Symbols::plug);
  lv_obj_align(plugIcon, nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 0);

  bleIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(bleIcon, "");
  lv_obj_align(bleIcon, nullptr, LV_ALIGN_IN_TOP_RIGHT, -30, 0);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  lockIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(lockIcon, "");
  lv_obj_align(lockIcon, nullptr, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

  // Date - Day / Week day

  label_date_day = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date_day, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::orange);
  lv_label_set_text_fmt(label_date_day, "%s\n%02i", dateTimeController.DayOfWeekShortToString(), dateTimeController.Day());
  lv_label_set_align(label_date_day, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(label_date_day, nullptr, LV_ALIGN_CENTER, 50, 0);

  minute_body = lv_line_create(lv_scr_act(), nullptr);
  hour_body = lv_line_create(lv_scr_act(), nullptr);

  lv_style_init(&minute_line_style);
  lv_style_set_line_width(&minute_line_style, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&minute_line_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&minute_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(minute_body, LV_LINE_PART_MAIN, &minute_line_style);

  lv_style_init(&hour_line_style);
  lv_style_set_line_width(&hour_line_style, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&hour_line_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&hour_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(hour_body, LV_LINE_PART_MAIN, &hour_line_style);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);

  Refresh();
}

WatchFaceAnalogNumbers::~WatchFaceAnalogNumbers() {
  lv_task_del(taskRefresh);

  lv_style_reset(&hour_line_style);
  lv_style_reset(&minute_line_style);

  lv_obj_clean(lv_scr_act());
}

void WatchFaceAnalogNumbers::UpdateClock() {
  uint8_t hour = dateTimeController.Hours();
  uint8_t minute = dateTimeController.Minutes();

  if (sMinute != minute) {
    auto const angle = minute * 6;
    minute_point[0] = CoordinateRelocate(5, angle);
    minute_point[1] = CoordinateRelocate(MinuteLength, angle);

    lv_line_set_points(minute_body, minute_point, 2);
  }

  if (sHour != hour || sMinute != minute) {
    sHour = hour;
    sMinute = minute;
    auto const angle = (hour * 30 + minute / 2);

    hour_point[0] = CoordinateRelocate(5, angle);
    hour_point[1] = CoordinateRelocate(HourLength, angle);

    lv_line_set_points(hour_body, hour_point, 2);
  }
}

void WatchFaceAnalogNumbers::SetBatteryIcon() {
  auto batteryPercent = batteryPercentRemaining.Get();
  batteryIcon.SetBatteryPercentage(batteryPercent);
}

void WatchFaceAnalogNumbers::Refresh() {
  isCharging = batteryController.IsCharging();
  if (isCharging.IsUpdated()) {
    if (isCharging.Get()) {
      lv_obj_set_hidden(batteryIcon.GetObject(), true);
      lv_obj_set_hidden(plugIcon, false);
    } else {
      lv_obj_set_hidden(batteryIcon.GetObject(), false);
      lv_obj_set_hidden(plugIcon, true);
      SetBatteryIcon();
    }
  }
  if (!isCharging.Get()) {
    batteryPercentRemaining = batteryController.PercentRemaining();
    if (batteryPercentRemaining.IsUpdated()) {
      SetBatteryIcon();
    }
  }

  bleState = bleController.IsConnected();
  if (bleState.IsUpdated()) {
    if (bleState.Get()) {
      lv_label_set_text_static(bleIcon, Symbols::bluetooth);
    } else {
      lv_label_set_text_static(bleIcon, "");
    }
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();
  notificationsPresent = !notificationManager.IsEmpty();
  if (notificationState.IsUpdated() || notificationsPresent.IsUpdated()) {
    // Three states: new notifications pop in lime, merely stored (read)
    // ones show a dim gray "i", none hides it.
    if (notificationState.Get()) {
      lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
    } else {
      lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::gray);
    }
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get() || notificationsPresent.Get()));
  }

  lockedState = settingsController.IsLocked();
  if (lockedState.IsUpdated()) {
    lv_label_set_text_static(lockIcon, lockedState.Get() ? Symbols::lock : "");
    lv_obj_realign(lockIcon);
  }

  currentDateTime = dateTimeController.CurrentDateTime();
  if (currentDateTime.IsUpdated()) {
    UpdateClock();

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      lv_label_set_text_fmt(label_date_day, "%s\n%02i", dateTimeController.DayOfWeekShortToString(), dateTimeController.Day());
    }
  }
}
