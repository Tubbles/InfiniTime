#pragma once

#include <lvgl/lvgl.h>

#include "displayapp/apps/Apps.h"
#include "displayapp/screens/Screen.h"
#include "displayapp/Controllers.h"
#include "systemtask/SystemTask.h"
#include "systemtask/WakeLock.h"
#include "Symbols.h"

namespace Pinetime {
  namespace Controllers {
    class AlertNotificationService;
    class KeyTonesService;
    class MotorController;
    class Settings;
  }

  namespace Applications {
    namespace Screens {

      // In-call screen: hang up, a DTMF numberpad, and the configured intercom
      // key as a direct button. The watch never learns call start/end (see
      // doc/DESIGN-intercom-keytones.md), so this app is opened and closed by
      // the user; key presses are fire-and-forget notifies to the phone.
      class InCall : public Screen {
      public:
        InCall(Controllers::AlertNotificationService& alertService,
               Controllers::KeyTonesService* keyTonesService,
               Controllers::MotorController& motorController,
               Controllers::Settings& settingsController,
               System::SystemTask& systemTask);
        ~InCall() override;

        bool OnTouchEvent(TouchEvents event) override;
        bool OnButtonPushed() override;

        void OnMainButtonEvent(lv_obj_t* obj, lv_event_t event);
        void OnKeypadEvent(lv_obj_t* obj, lv_event_t event);

      private:
        enum class View { Main, Keypad };

        void ShowMainView();
        void ShowKeypadView();
        void SendKey(char key);

        Controllers::AlertNotificationService& alertService;
        Controllers::KeyTonesService* keyTonesService;
        Controllers::MotorController& motorController;
        Controllers::Settings& settingsController;
        System::WakeLock wakeLock;

        View currentView = View::Main;
        lv_obj_t* btnHangUp = nullptr;
        lv_obj_t* btnNumberpad = nullptr;
        lv_obj_t* btnIntercom = nullptr;
        lv_obj_t* keypadMatrix = nullptr;
        char intercomLabel[2] = {0, 0};
      };
    }

    template <>
    struct AppTraits<Apps::InCall> {
      static constexpr Apps app = Apps::InCall;
      static constexpr const char* icon = Screens::Symbols::phone;

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::InCall(*controllers.alertService,
                                   controllers.keyTonesService,
                                   controllers.motorController,
                                   controllers.settingsController,
                                   *controllers.systemTask);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
