#pragma once

#include <cstdint>

namespace Pinetime {
  namespace Controllers {

    // Plain data carriers for the two wrist gestures' tuning knobs, in their
    // own header so that Settings can own the persisted values without
    // MotionController growing a dependency on Settings, and so each default
    // has exactly one definition shared by the settings defaults and by the
    // Reset buttons.

    struct RaiseWakeThresholds {
      uint8_t rollDegrees; // applied negated: the face must roll at least this far toward the viewer
      uint8_t stillness;   // applied squared as the variance limit
      uint16_t level;      // |xMean| limit, 1024 is 1 g
      uint16_t tilt;       // applied negated as the yMean limit
      uint8_t window;      // ring samples spanned between the "prev" and "now" groups
      uint8_t settle;      // newest samples averaged into each group

      bool operator==(const RaiseWakeThresholds&) const = default;
    };

    struct LowerSleepThresholds {
      uint16_t sideLevel;      // |xMean| above which the sideways test applies
      uint8_t sideRollDegrees; // roll around the forearm needed with it
      uint16_t facingLevel;    // yMean below this rejects
      uint8_t rollDegrees;     // roll toward the ground below this rejects
      uint16_t historyFloor;   // any sample in the window below this rejects

      bool operator==(const LowerSleepThresholds&) const = default;
    };

    inline constexpr RaiseWakeThresholds raiseWakeDefaults = {45, 56, 384, 64, 8, 2};
    inline constexpr LowerSleepThresholds lowerSleepDefaults = {887, 30, 724, 30, 265};
  }
}
