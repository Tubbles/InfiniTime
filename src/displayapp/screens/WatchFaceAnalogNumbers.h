#pragma once

#include <lvgl/src/lv_core/lv_obj.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "displayapp/screens/BatteryIcon.h"
#include "utility/DirtyValue.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class NotificationManager;
  }

  namespace Applications {
    namespace Screens {

      class WatchFaceAnalogNumbers : public Screen {
      public:
        WatchFaceAnalogNumbers(Controllers::DateTime& dateTimeController,
                               const Controllers::Battery& batteryController,
                               const Controllers::Ble& bleController,
                               Controllers::NotificationManager& notificationManager,
                               Controllers::Settings& settingsController);

        ~WatchFaceAnalogNumbers() override;

        void Refresh() override;

      private:
        uint8_t sHour, sMinute;

        Utility::DirtyValue<uint8_t> batteryPercentRemaining {0};
        Utility::DirtyValue<bool> isCharging {};
        Utility::DirtyValue<bool> bleState {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>> currentDateTime;
        Utility::DirtyValue<bool> notificationState {false};
        Utility::DirtyValue<bool> notificationsPresent {};
        Utility::DirtyValue<bool> lockedState {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::days>> currentDate;

        lv_obj_t* minor_scales;
        lv_obj_t* major_scales;
        lv_obj_t* numerals[12];

        lv_obj_t* hour_body;
        lv_obj_t* minute_body;

        lv_point_t hour_point[2];
        lv_point_t minute_point[2];

        lv_style_t hour_line_style;
        lv_style_t minute_line_style;

        lv_obj_t* label_date_day;
        lv_obj_t* plugIcon;
        lv_obj_t* notificationIcon;
        lv_obj_t* bleIcon;
        lv_obj_t* lockIcon;

        BatteryIcon batteryIcon;

        Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        const Controllers::Ble& bleController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;

        void UpdateClock();
        void SetBatteryIcon();

        lv_task_t* taskRefresh;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::AnalogNumbers> {
      static constexpr WatchFace watchFace = WatchFace::AnalogNumbers;
      static constexpr const char* name = "Analog 12";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceAnalogNumbers(controllers.dateTimeController,
                                                   controllers.batteryController,
                                                   controllers.bleController,
                                                   controllers.notificationManager,
                                                   controllers.settingsController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
