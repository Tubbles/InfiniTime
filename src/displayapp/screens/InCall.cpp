#include "displayapp/screens/InCall.h"

#include "components/ble/AlertNotificationService.h"
#include "components/ble/KeyTonesService.h"
#include "components/motor/MotorController.h"
#include "components/settings/Settings.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  void MainEventHandler(lv_obj_t* obj, lv_event_t event) {
    auto* screen = static_cast<InCall*>(obj->user_data);
    screen->OnMainButtonEvent(obj, event);
  }

  void KeypadEventHandler(lv_obj_t* obj, lv_event_t event) {
    auto* screen = static_cast<InCall*>(obj->user_data);
    screen->OnKeypadEvent(obj, event);
  }

  constexpr const char* keypadMap[] = {"1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n", "*", "0", "#", ""};
}

InCall::InCall(Controllers::AlertNotificationService& alertService,
               Controllers::KeyTonesService* keyTonesService,
               Controllers::MotorController& motorController,
               Controllers::Settings& settingsController,
               System::SystemTask& systemTask)
  : alertService {alertService},
    keyTonesService {keyTonesService},
    motorController {motorController},
    settingsController {settingsController},
    wakeLock(systemTask) {
  // A call is an interactive situation; keep the screen awake while open.
  wakeLock.Lock();
  ShowMainView();
}

InCall::~InCall() {
  lv_obj_clean(lv_scr_act());
}

void InCall::ShowMainView() {
  lv_obj_clean(lv_scr_act());
  currentView = View::Main;
  keypadMatrix = nullptr;

  lv_obj_t* title = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(title, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
  lv_label_set_text_static(title, "In call");
  lv_obj_align(title, nullptr, LV_ALIGN_IN_TOP_MID, 0, 10);

  const char intercomKey = settingsController.GetIntercomKey();
  if (intercomKey != 0) {
    btnIntercom = lv_btn_create(lv_scr_act(), nullptr);
    btnIntercom->user_data = this;
    lv_obj_set_event_cb(btnIntercom, MainEventHandler);
    lv_obj_set_size(btnIntercom, 228, 90);
    lv_obj_align(btnIntercom, nullptr, LV_ALIGN_IN_TOP_MID, 0, 45);
    lv_obj_set_style_local_bg_color(btnIntercom, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, Colors::highlight);
    lv_obj_t* label = lv_label_create(btnIntercom, nullptr);
    intercomLabel[0] = intercomKey;
    lv_label_set_text_static(label, intercomLabel);
  } else {
    btnIntercom = nullptr;
  }

  btnHangUp = lv_btn_create(lv_scr_act(), nullptr);
  btnHangUp->user_data = this;
  lv_obj_set_event_cb(btnHangUp, MainEventHandler);
  lv_obj_set_size(btnHangUp, 114, 80);
  lv_obj_align(btnHangUp, nullptr, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_local_bg_color(btnHangUp, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
  lv_obj_t* hangUpLabel = lv_label_create(btnHangUp, nullptr);
  lv_label_set_text_static(hangUpLabel, Symbols::phoneSlash);

  btnNumberpad = lv_btn_create(lv_scr_act(), nullptr);
  btnNumberpad->user_data = this;
  lv_obj_set_event_cb(btnNumberpad, MainEventHandler);
  lv_obj_set_size(btnNumberpad, 114, 80);
  lv_obj_align(btnNumberpad, nullptr, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
  lv_obj_set_style_local_bg_color(btnNumberpad, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, Colors::bgAlt);
  lv_obj_t* numberpadLabel = lv_label_create(btnNumberpad, nullptr);
  lv_label_set_text_static(numberpadLabel, "123");
}

void InCall::ShowKeypadView() {
  lv_obj_clean(lv_scr_act());
  currentView = View::Keypad;
  btnHangUp = nullptr;
  btnNumberpad = nullptr;
  btnIntercom = nullptr;

  keypadMatrix = lv_btnmatrix_create(lv_scr_act(), nullptr);
  keypadMatrix->user_data = this;
  lv_obj_set_event_cb(keypadMatrix, KeypadEventHandler);
  lv_btnmatrix_set_map(keypadMatrix, const_cast<const char**>(keypadMap));
  lv_obj_set_size(keypadMatrix, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_style_local_bg_color(keypadMatrix, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, Colors::bgAlt);
  lv_obj_set_style_local_pad_inner(keypadMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_pad_top(keypadMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_pad_bottom(keypadMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_pad_left(keypadMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_pad_right(keypadMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_align(keypadMatrix, nullptr, LV_ALIGN_CENTER, 0, 0);
}

void InCall::SendKey(char key) {
  if (keyTonesService != nullptr) {
    keyTonesService->NotifyKey(key);
  }
  motorController.RunForDuration(20);
}

void InCall::OnMainButtonEvent(lv_obj_t* obj, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) {
    return;
  }
  if (obj == btnHangUp) {
    // Two paths, because each covers what the other cannot: the companion
    // maps the alert-service reject to TelecomManager.endCall (deprecated,
    // silently refused for some ongoing calls), while 'E' reaches the dialer
    // app, whose InCallService can always end the call it owns.
    alertService.RejectIncomingCall();
    SendKey('E');
    running = false;
  } else if (obj == btnNumberpad) {
    ShowKeypadView();
  } else if (obj == btnIntercom) {
    SendKey(settingsController.GetIntercomKey());
  }
}

void InCall::OnKeypadEvent(lv_obj_t* obj, lv_event_t event) {
  if (obj != keypadMatrix || event != LV_EVENT_PRESSED) {
    return;
  }
  const char* buttonText = lv_btnmatrix_get_active_btn_text(keypadMatrix);
  if (buttonText == nullptr) {
    return;
  }
  SendKey(buttonText[0]);
}

bool InCall::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  // In the keypad view a right swipe goes back to the main view instead of
  // leaving the app.
  if (currentView == View::Keypad && event == TouchEvents::SwipeRight) {
    ShowMainView();
    return true;
  }
  return false;
}
