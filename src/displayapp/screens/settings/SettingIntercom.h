#pragma once

#include <lvgl/lvgl.h>

#include "displayapp/screens/Screen.h"
#include "components/settings/Settings.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      // Picks the key shown as the direct "intercom button" on the InCall
      // screen: one of 0-9, *, #, or Off (see doc/DESIGN-intercom-keytones.md).
      class SettingIntercom : public Screen {
      public:
        explicit SettingIntercom(Pinetime::Controllers::Settings& settingsController);
        ~SettingIntercom() override;

        void OnKeyEvent(lv_obj_t* obj, lv_event_t event);

      private:
        Controllers::Settings& settingsController;
        lv_obj_t* keypadMatrix = nullptr;
      };
    }
  }
}
