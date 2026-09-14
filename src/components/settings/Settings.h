#pragma once
#include <cstdint>
#include <bitset>
#include <limits>
#include <optional>
#include "components/brightness/BrightnessController.h"
#include "components/fs/FS.h"
#include "components/motion/MotionThresholds.h"
#include "displayapp/apps/Apps.h"
#include <nrf_log.h>

namespace Pinetime {
  namespace Controllers {
    class Settings {
    public:
      enum class ClockType : uint8_t { H24, H12 };
      enum class WeatherFormat : uint8_t { Metric, Imperial };
      enum class Notification : uint8_t { On, Off, Sleep };
      enum class ChimesOption : uint8_t { None, Hours, HalfHours };
      enum class WakeUpMode : uint8_t { SingleTap = 0, DoubleTap = 1, RaiseWrist = 2, Shake = 3, LowerWrist = 4 };
      enum class Colors : uint8_t {
        White,
        Silver,
        Gray,
        Black,
        Red,
        Maroon,
        Yellow,
        Olive,
        Lime,
        Green,
        Cyan,
        Teal,
        Blue,
        Navy,
        Magenta,
        Purple,
        Orange,
        Pink
      };
      enum class PTSGaugeStyle : uint8_t { Full, Half, Numeric };
      enum class PTSWeather : uint8_t { On, Off };
      enum class PrideFlag : uint8_t { Gay, Trans, Bi, Lesbian };
      enum class DfuAndFsMode : uint8_t { Disabled, Enabled, EnabledTillReboot };

      struct PineTimeStyle {
        Colors ColorTime = Colors::Teal;
        Colors ColorBar = Colors::Teal;
        Colors ColorBG = Colors::Black;
        PTSGaugeStyle gaugeStyle = PTSGaugeStyle::Full;
        PTSWeather weatherEnable = PTSWeather::Off;
      };

      struct WatchFaceInfineat {
        bool showSideCover = true;
        int colorIndex = 0;
      };

      Settings(Pinetime::Controllers::FS& fs);

      Settings(const Settings&) = delete;
      Settings& operator=(const Settings&) = delete;
      Settings(Settings&&) = delete;
      Settings& operator=(Settings&&) = delete;

      void Init();
      void SaveSettings();

      void SetWatchFace(Pinetime::Applications::WatchFace face) {
        if (face != settings.watchFace) {
          settingsChanged = true;
        }
        settings.watchFace = face;
      };

      Pinetime::Applications::WatchFace GetWatchFace() const {
        return settings.watchFace;
      };

      void SetChimeOption(ChimesOption chimeOption) {
        if (chimeOption != settings.chimesOption) {
          settingsChanged = true;
        }
        settings.chimesOption = chimeOption;
      };

      ChimesOption GetChimeOption() const {
        return settings.chimesOption;
      };

      void SetPTSColorTime(Colors colorTime) {
        if (colorTime != settings.PTS.ColorTime)
          settingsChanged = true;
        settings.PTS.ColorTime = colorTime;
      };

      Colors GetPTSColorTime() const {
        return settings.PTS.ColorTime;
      };

      void SetPTSColorBar(Colors colorBar) {
        if (colorBar != settings.PTS.ColorBar)
          settingsChanged = true;
        settings.PTS.ColorBar = colorBar;
      };

      Colors GetPTSColorBar() const {
        return settings.PTS.ColorBar;
      };

      void SetPTSColorBG(Colors colorBG) {
        if (colorBG != settings.PTS.ColorBG)
          settingsChanged = true;
        settings.PTS.ColorBG = colorBG;
      };

      Colors GetPTSColorBG() const {
        return settings.PTS.ColorBG;
      };

      void SetInfineatShowSideCover(bool show) {
        if (show != settings.watchFaceInfineat.showSideCover) {
          settings.watchFaceInfineat.showSideCover = show;
          settingsChanged = true;
        }
      };

      bool GetInfineatShowSideCover() const {
        return settings.watchFaceInfineat.showSideCover;
      };

      void SetInfineatColorIndex(int index) {
        if (index != settings.watchFaceInfineat.colorIndex) {
          settings.watchFaceInfineat.colorIndex = index;
          settingsChanged = true;
        }
      };

      int GetInfineatColorIndex() const {
        return settings.watchFaceInfineat.colorIndex;
      };

      void SetPTSGaugeStyle(PTSGaugeStyle gaugeStyle) {
        if (gaugeStyle != settings.PTS.gaugeStyle)
          settingsChanged = true;
        settings.PTS.gaugeStyle = gaugeStyle;
      };

      PTSGaugeStyle GetPTSGaugeStyle() const {
        return settings.PTS.gaugeStyle;
      };

      void SetPTSWeather(PTSWeather weatherEnable) {
        if (weatherEnable != settings.PTS.weatherEnable)
          settingsChanged = true;
        settings.PTS.weatherEnable = weatherEnable;
      };

      PTSWeather GetPTSWeather() const {
        return settings.PTS.weatherEnable;
      };

      void SetPrideFlag(PrideFlag prideFlag) {
        if (prideFlag != settings.prideFlag)
          settingsChanged = true;
        settings.prideFlag = prideFlag;
      };

      PrideFlag GetPrideFlag() const {
        return settings.prideFlag;
      };

      void SetAppMenu(uint8_t menu) {
        appMenu = menu;
      };

      uint8_t GetAppMenu() const {
        return appMenu;
      };

      void SetSettingsMenu(uint8_t menu) {
        settingsMenu = menu;
      };

      uint8_t GetSettingsMenu() const {
        return settingsMenu;
      };

      void SetClockType(ClockType clocktype) {
        if (clocktype != settings.clockType) {
          settingsChanged = true;
        }
        settings.clockType = clocktype;
      };

      ClockType GetClockType() const {
        return settings.clockType;
      };

      void SetWeatherFormat(WeatherFormat weatherFormat) {
        if (weatherFormat != settings.weatherFormat) {
          settingsChanged = true;
        }
        settings.weatherFormat = weatherFormat;
      };

      WeatherFormat GetWeatherFormat() const {
        return settings.weatherFormat;
      };

      void SetNotificationStatus(Notification status) {
        if (status != settings.notificationStatus) {
          settingsChanged = true;
        }
        settings.notificationStatus = status;
      };

      Notification GetNotificationStatus() const {
        return settings.notificationStatus;
      };

      void SetScreenTimeOut(uint32_t timeout) {
        if (timeout != settings.screenTimeOut) {
          settingsChanged = true;
        }
        settings.screenTimeOut = timeout;
      };

      uint32_t GetScreenTimeOut() const {
        return settings.screenTimeOut;
      };

      bool GetAlwaysOnDisplay() const {
        return settings.alwaysOnDisplay && GetNotificationStatus() != Notification::Sleep;
      };

      void SetAlwaysOnDisplaySetting(bool state) {
        if (state != settings.alwaysOnDisplay) {
          settingsChanged = true;
        }
        settings.alwaysOnDisplay = state;
      }

      bool GetAlwaysOnDisplaySetting() const {
        return settings.alwaysOnDisplay;
      }

      void SetShakeThreshold(uint16_t thresh) {
        if (settings.shakeWakeThreshold != thresh) {
          settings.shakeWakeThreshold = thresh;
          settingsChanged = true;
        }
      }

      int16_t GetShakeThreshold() const {
        return settings.shakeWakeThreshold;
      }

      void setWakeUpMode(WakeUpMode wakeUp, bool enabled) {
        if (enabled != isWakeUpModeOn(wakeUp)) {
          settingsChanged = true;
        }
        settings.wakeUpMode.set(static_cast<size_t>(wakeUp), enabled);
        // Handle special behavior
        if (enabled) {
          switch (wakeUp) {
            case WakeUpMode::SingleTap:
              settings.wakeUpMode.set(static_cast<size_t>(WakeUpMode::DoubleTap), false);
              break;
            case WakeUpMode::DoubleTap:
              settings.wakeUpMode.set(static_cast<size_t>(WakeUpMode::SingleTap), false);
              break;
            default:
              break;
          }
        }
      };

      std::bitset<5> getWakeUpModes() const {
        return settings.wakeUpMode;
      }

      bool isWakeUpModeOn(const WakeUpMode mode) const {
        return getWakeUpModes()[static_cast<size_t>(mode)];
      }

      void SetBrightness(Controllers::BrightnessController::Levels level) {
        if (level != settings.brightLevel) {
          settingsChanged = true;
        }
        settings.brightLevel = level;
      };

      Controllers::BrightnessController::Levels GetBrightness() const {
        return settings.brightLevel;
      };

      void SetStepsGoal(uint32_t goal) {
        if (goal != settings.stepsGoal) {
          settingsChanged = true;
        }
        settings.stepsGoal = goal;
      };

      uint32_t GetStepsGoal() const {
        return settings.stepsGoal;
      };

      void SetBleRadioEnabled(bool enabled) {
        bleRadioEnabled = enabled;
      };

      bool GetBleRadioEnabled() const {
        return bleRadioEnabled;
      };

      void SetLocked(bool isLocked) {
        locked = isLocked;
      };

      bool IsLocked() const {
        return locked;
      };

      void SetLockScreenEnabled(bool enabled) {
        if (enabled != settings.lockScreenEnabled) {
          settingsChanged = true;
        }
        settings.lockScreenEnabled = enabled;
        if (!enabled) {
          // Switching the feature off has to release a lock that is live right
          // now, or the user is left staring at a locked screen belonging to a
          // feature they just turned off.
          locked = false;
        }
      };

      bool GetLockScreenEnabled() const {
        return settings.lockScreenEnabled;
      };

      RaiseWakeThresholds GetRaiseWakeThresholds() const {
        return {settings.raiseRollDegrees,
                settings.raiseStillness,
                settings.raiseLevel,
                settings.raiseTilt,
                settings.raiseWindow,
                settings.raiseSettle};
      };

      void SetRaiseWakeThresholds(const RaiseWakeThresholds& thresholds) {
        if (thresholds != GetRaiseWakeThresholds()) {
          settingsChanged = true;
        }
        settings.raiseRollDegrees = thresholds.rollDegrees;
        settings.raiseStillness = thresholds.stillness;
        settings.raiseLevel = thresholds.level;
        settings.raiseTilt = thresholds.tilt;
        settings.raiseWindow = thresholds.window;
        settings.raiseSettle = thresholds.settle;
      };

      LowerSleepThresholds GetLowerSleepThresholds() const {
        return {settings.lowerSideLevel,
                settings.lowerSideRollDegrees,
                settings.lowerFacingLevel,
                settings.lowerRollDegrees,
                settings.lowerHistoryFloor};
      };

      void SetLowerSleepThresholds(const LowerSleepThresholds& thresholds) {
        if (thresholds != GetLowerSleepThresholds()) {
          settingsChanged = true;
        }
        settings.lowerSideLevel = thresholds.sideLevel;
        settings.lowerSideRollDegrees = thresholds.sideRollDegrees;
        settings.lowerFacingLevel = thresholds.facingLevel;
        settings.lowerRollDegrees = thresholds.rollDegrees;
        settings.lowerHistoryFloor = thresholds.historyFloor;
      };

      void SetIntercomKey(char key) {
        if (key != settings.intercomKey) {
          settingsChanged = true;
        }
        settings.intercomKey = key;
      };

      char GetIntercomKey() const {
        return settings.intercomKey;
      };

      void SetDfuAndFsMode(DfuAndFsMode mode) {
        if (mode == GetDfuAndFsMode()) {
          return;
        }
        if (mode == DfuAndFsMode::Enabled || GetDfuAndFsMode() == DfuAndFsMode::Enabled) {
          settingsChanged = true;
        }
        settings.dfuAndFsEnabledOnBoot = (mode == DfuAndFsMode::Enabled);
        dfuAndFsEnabledTillReboot = (mode == DfuAndFsMode::EnabledTillReboot);
      };

      DfuAndFsMode GetDfuAndFsMode() {
        if (dfuAndFsEnabledTillReboot) {
          if (settings.dfuAndFsEnabledOnBoot) { // ensure both variables are in consistent state
            settingsChanged = true;
            settings.dfuAndFsEnabledOnBoot = false;
            NRF_LOG_ERROR("Settings: DfuAndFsMode data corrupted");
          }
          return DfuAndFsMode::EnabledTillReboot;
        }
        return (settings.dfuAndFsEnabledOnBoot ? DfuAndFsMode::Enabled : DfuAndFsMode::Disabled);
      };

      std::optional<uint16_t> GetHeartRateBackgroundMeasurementInterval() const {
        if (settings.heartRateBackgroundPeriod == std::numeric_limits<uint16_t>::max()) {
          return std::nullopt;
        }
        return settings.heartRateBackgroundPeriod;
      }

      void SetHeartRateBackgroundMeasurementInterval(std::optional<uint16_t> newIntervalInSeconds) {
        newIntervalInSeconds = newIntervalInSeconds.value_or(std::numeric_limits<uint16_t>::max());
        if (newIntervalInSeconds != settings.heartRateBackgroundPeriod) {
          settingsChanged = true;
        }
        settings.heartRateBackgroundPeriod = newIntervalInSeconds.value();
      }

    private:
      Pinetime::Controllers::FS& fs;

      static constexpr uint32_t settingsVersion = 0x000b;

      struct SettingsData {
        uint32_t version = settingsVersion;
        uint32_t stepsGoal = 10000;
        uint32_t screenTimeOut = 15000;

        bool alwaysOnDisplay = false;

        ClockType clockType = ClockType::H24;
        WeatherFormat weatherFormat = WeatherFormat::Metric;
        Notification notificationStatus = Notification::On;

        Pinetime::Applications::WatchFace watchFace = Pinetime::Applications::WatchFace::Digital;
        ChimesOption chimesOption = ChimesOption::None;

        PineTimeStyle PTS;

        PrideFlag prideFlag = PrideFlag::Gay;

        WatchFaceInfineat watchFaceInfineat;

        std::bitset<5> wakeUpMode {0};
        uint16_t shakeWakeThreshold = 150;

        Controllers::BrightnessController::Levels brightLevel = Controllers::BrightnessController::Levels::Medium;

        bool dfuAndFsEnabledOnBoot = false;
        uint16_t heartRateBackgroundPeriod = std::numeric_limits<uint16_t>::max(); // Disabled by default

        // In-call intercom button: 0 = off, else the ASCII key ('0'-'9', '*', '#')
        char intercomKey = 0;

        // Appended 2026-09-14 without a settingsVersion bump, because a bump
        // resets every setting on the watch. alignas(4) is what makes that
        // safe: SettingsData is 4-byte aligned, so a 4-aligned member lands
        // exactly at the previous sizeof, past every byte an older
        // settings.dat holds, and LoadSettingsFromFile leaves it at the
        // default below. A bare bool would instead sit in the old layout's
        // trailing padding and load whatever that file carries there.
        alignas(4) bool lockScreenEnabled = true;

        // Wrist gesture tuning, appended under the same rule. Defaults come
        // from components/motion/MotionThresholds.h so the Reset buttons and
        // these initializers cannot drift apart.
        uint8_t raiseRollDegrees = raiseWakeDefaults.rollDegrees;
        uint8_t raiseStillness = raiseWakeDefaults.stillness;
        uint8_t raiseWindow = raiseWakeDefaults.window;
        uint8_t raiseSettle = raiseWakeDefaults.settle;
        uint16_t raiseLevel = raiseWakeDefaults.level;
        uint16_t raiseTilt = raiseWakeDefaults.tilt;

        uint8_t lowerSideRollDegrees = lowerSleepDefaults.sideRollDegrees;
        uint8_t lowerRollDegrees = lowerSleepDefaults.rollDegrees;
        uint16_t lowerSideLevel = lowerSleepDefaults.sideLevel;
        uint16_t lowerFacingLevel = lowerSleepDefaults.facingLevel;
        uint16_t lowerHistoryFloor = lowerSleepDefaults.historyFloor;
      };

      SettingsData settings;
      bool settingsChanged = false;

      uint8_t appMenu = 0;
      uint8_t settingsMenu = 0;
      uint8_t watchFacesMenu = 0;
      /* ble state is intentionally not saved with the other watch settings and initialized
       * to off (false) on every boot because we always want ble to be enabled on startup
       */
      bool bleRadioEnabled = true;
      bool dfuAndFsEnabledTillReboot = false;
      /* wrist-raise lock: runtime-only so the watch always boots unlocked.
       * Set by SystemTask on a raise-wrist wake, cleared on button unlock,
       * sleep, and alarm/timer alerts. */
      bool locked = false;

      void LoadSettingsFromFile();
      void SaveSettingsToFile();
    };
  }
}
