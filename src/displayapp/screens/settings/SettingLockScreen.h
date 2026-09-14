#pragma once

#include <cstdint>
#include <lvgl/lvgl.h>
#include "components/settings/Settings.h"
#include "displayapp/screens/Screen.h"

namespace Pinetime {

  namespace Applications {
    namespace Screens {

      class SettingLockScreen : public Screen {
      public:
        explicit SettingLockScreen(Pinetime::Controllers::Settings& settingsController);
        ~SettingLockScreen() override;

        void Toggled();

      private:
        Controllers::Settings& settingsController;
        lv_obj_t* checkbox;
      };
    }
  }
}
