#pragma once

#include <displayapp/screens/BatteryIcon.h>
#include <lvgl/src/lv_core/lv_obj.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <displayapp/Controllers.h>
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/ble/BleController.h"
#include "components/ble/HubService.h"
#include "utility/DirtyValue.h"
#include "displayapp/apps/Apps.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class AlarmController;
    class NotificationManager;
    class HeartRateController;
    class MotionController;
  }

  namespace Applications {
    namespace Screens {

      class WatchFaceCasioStyleG7710 : public Screen {
      public:
        WatchFaceCasioStyleG7710(Controllers::DateTime& dateTimeController,
                                 const Controllers::Battery& batteryController,
                                 const Controllers::Ble& bleController,
                                 const Controllers::AlarmController& alarmController,
                                 Controllers::HubService* hubService,
                                 Controllers::NotificationManager& notificatioManager,
                                 Controllers::Settings& settingsController,
                                 Controllers::HeartRateController& heartRateController,
                                 Controllers::MotionController& motionController,
                                 Controllers::FS& filesystem);
        ~WatchFaceCasioStyleG7710() override;

        void Refresh() override;

        static bool IsAvailable(Pinetime::Controllers::FS& filesystem);

      private:
        enum class CornerKind : uint8_t { None, Alarm, Event };

        // What the next-event corner shows; a change repaints it. The
        // source and generation catch a new icon at an unchanged minute.
        // Value-initialized, it reads None.
        struct CornerContent {
          CornerKind kind;
          uint8_t sourceId;
          uint16_t minutesOfDay;
          uint8_t generation;

          bool operator==(const CornerContent&) const = default;
        };

        CornerContent NextCornerContent(const std::optional<Controllers::HubService::Event>& event, uint32_t nowUtcSeconds) const;
        void PaintCorner(const CornerContent& content, const std::optional<Controllers::HubService::Event>& event);
        void ShowEventIcon(const Controllers::HubService::Event& event);
        uint16_t LocalMinutesOfDay(uint32_t utcSeconds) const;
        uint32_t NowUtcSeconds() const;

        Utility::DirtyValue<uint8_t> batteryPercentRemaining {};
        Utility::DirtyValue<bool> powerPresent {};
        Utility::DirtyValue<bool> bleState {};
        Utility::DirtyValue<bool> bleRadioEnabled {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentDateTime {};
        Utility::DirtyValue<uint32_t> stepCount {};
        Utility::DirtyValue<uint8_t> heartbeat {};
        Utility::DirtyValue<bool> heartbeatRunning {};
        Utility::DirtyValue<bool> lockedState {};
        Utility::DirtyValue<CornerContent> cornerContent {};
        Utility::DirtyValue<bool> notificationsPresent {};
        Utility::DirtyValue<bool> notificationState {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::days>> currentDate;

        lv_point_t line_icons_points[3] {{0, 5}, {117, 5}, {122, 0}};
        lv_point_t line_day_of_week_number_points[4] {{0, 0}, {100, 0}, {95, 95}, {0, 95}};
        lv_point_t line_day_of_year_points[3] {{0, 5}, {130, 5}, {135, 0}};
        lv_point_t line_date_points[3] {{0, 5}, {135, 5}, {140, 0}};
        lv_point_t line_time_points[3] {{0, 0}, {230, 0}, {235, 5}};

        lv_color_t color_text = lv_color_hex(0x98B69A);

        lv_style_t style_line;
        lv_style_t style_border;

        lv_obj_t* label_time;
        lv_obj_t* line_time;
        lv_obj_t* label_time_ampm;
        lv_obj_t* label_date;
        lv_obj_t* line_date;
        lv_obj_t* label_day_of_week;
        lv_obj_t* label_week_number;
        lv_obj_t* line_day_of_week_number;
        lv_obj_t* label_day_of_year;
        lv_obj_t* line_day_of_year;
        lv_obj_t* backgroundLabel;
        lv_obj_t* bleIcon;
        lv_obj_t* batteryPlug;
        lv_obj_t* label_battery_value;
        lv_obj_t* heartbeatIcon;
        lv_obj_t* heartbeatValue;
        lv_obj_t* alarmIcon;
        lv_obj_t* eventIcon;
        lv_obj_t* alarmValue;
        lv_obj_t* stepIcon;
        lv_obj_t* stepValue;
        lv_obj_t* notificationIcon;
        lv_obj_t* line_icons;

        BatteryIcon batteryIcon;

        // The face's own copy of the shown event icon: the BLE task may
        // rewrite the service's slots at any time, and LVGL reads the
        // pixels on every redraw. Palette first (index 0 transparent,
        // index 1 color_text), then the rows.
        static constexpr uint8_t eventIconPaletteSize = 2 * sizeof(lv_color32_t);
        alignas(lv_color32_t) std::array<uint8_t, eventIconPaletteSize + Controllers::HubService::maximumBitmapSize> eventIconData {};
        lv_img_dsc_t eventIconDescriptor {};

        Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        const Controllers::Ble& bleController;
        const Controllers::AlarmController& alarmController;
        Controllers::HubService* hubService;
        Controllers::NotificationManager& notificatioManager;
        Controllers::Settings& settingsController;
        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;

        lv_task_t* taskRefresh;
        lv_font_t* font_dot40 = nullptr;
        lv_font_t* font_segment40 = nullptr;
        lv_font_t* font_segment115 = nullptr;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::CasioStyleG7710> {
      static constexpr WatchFace watchFace = WatchFace::CasioStyleG7710;
      static constexpr const char* name = "Casio G7710";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceCasioStyleG7710(controllers.dateTimeController,
                                                     controllers.batteryController,
                                                     controllers.bleController,
                                                     controllers.alarmController,
                                                     controllers.hubService,
                                                     controllers.notificationManager,
                                                     controllers.settingsController,
                                                     controllers.heartRateController,
                                                     controllers.motionController,
                                                     controllers.filesystem);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& filesystem) {
        return Screens::WatchFaceCasioStyleG7710::IsAvailable(filesystem);
      }
    };
  }
}
