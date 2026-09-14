#pragma once

#include <cstdint>
#include <lvgl/lvgl.h>
#include "components/settings/Settings.h"
#include "displayapp/screens/Screen.h"

namespace Pinetime {

  namespace Applications {
    namespace Screens {

      // One page per wrist gesture. The layout, the sliders and the Reset
      // button are identical for both, so the gesture only selects the row
      // table and which pair of Settings accessors is used.
      class SettingMotionThresholds : public Screen {
      public:
        enum class Gesture : uint8_t { RaiseWrist, LowerWrist };

        SettingMotionThresholds(Pinetime::Controllers::Settings& settingsController, Gesture gesture);
        ~SettingMotionThresholds() override;

        void OnSliderChanged(lv_obj_t* object);
        void OnReset();

        static constexpr uint8_t maxRows = 6;

      private:
        void UpdateRow(uint8_t index);
        void StoreToSettings();

        Controllers::Settings& settingsController;
        Gesture gesture;
        uint8_t rowCount;
        uint16_t values[maxRows] = {};
        lv_obj_t* sliders[maxRows] = {};
        lv_obj_t* labels[maxRows] = {};
        lv_obj_t* resetButton;
      };
    }
  }
}
