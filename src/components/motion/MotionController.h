#pragma once

#include <cstdint>

#include <FreeRTOS.h>

#include "drivers/Bma421.h"
#include "components/ble/MotionService.h"
#include "components/motion/MotionThresholds.h"
#include "utility/CircularBuffer.h"

namespace Pinetime {
  namespace Controllers {
    class MotionController {
    public:
      enum class DeviceTypes {
        Unknown,
        BMA421,
        BMA425,
      };

      enum class Days : uint8_t {
        Today = 0,
        Yesterday,
      };

      static constexpr size_t stepHistorySize = 2; // Store this many day's step counter

      void AdvanceDay();

      void Update(int16_t x, int16_t y, int16_t z, uint32_t nbSteps);

      int16_t X() const {
        return xHistory[0];
      }

      int16_t Y() const {
        return yHistory[0];
      }

      int16_t Z() const {
        return zHistory[0];
      }

      uint32_t NbSteps(Days day = Days::Today) const {
        return nbSteps[static_cast<std::underlying_type_t<Days>>(day)];
      }

      void ResetTrip() {
        currentTripSteps = 0;
      }

      uint32_t GetTripSteps() const {
        return currentTripSteps;
      }

      bool ShouldRaiseWake(const RaiseWakeThresholds& thresholds) const;
      bool ShouldLowerSleep(const LowerSleepThresholds& thresholds) const;

      int32_t CurrentShakeSpeed() const {
        return accumulatedSpeed;
      }

      DeviceTypes DeviceType() const {
        return deviceType;
      }

      void Init(Pinetime::Drivers::Bma421::DeviceTypes types);

      void SetService(Pinetime::Controllers::MotionService* service) {
        this->service = service;
      }

      Pinetime::Controllers::MotionService* GetService() const {
        return service;
      }

    private:
      Utility::CircularBuffer<uint32_t, stepHistorySize> nbSteps = {0};
      uint32_t currentTripSteps = 0;

      void SetSteps(Days day, uint32_t steps) {
        nbSteps[static_cast<std::underlying_type_t<Days>>(day)] = steps;
      }

      TickType_t lastTime = 0;
      TickType_t time = 0;

      struct AccelStats {
        int16_t xMean = 0;
        int16_t yMean = 0;
        int16_t zMean = 0;
        int16_t prevXMean = 0;
        int16_t prevYMean = 0;
        int16_t prevZMean = 0;

        uint32_t xVariance = 0;
        uint32_t yVariance = 0;
        uint32_t zVariance = 0;
      };

      // window = ring samples spanned between the two groups, settle = how
      // many newest samples each group averages.
      AccelStats GetAccelStats(uint8_t window, uint8_t settle) const;

      // The buffer is sized for the largest window the raise-wake setting
      // offers; the default window of 8 uses only the newest half of it.
      static constexpr uint8_t historySize = 16;
      // Lower wrist keeps the timing the algorithm always had. Only raise
      // wake exposes its window, because that is the gesture people retune.
      static constexpr uint8_t lowerSleepWindow = 8;
      static constexpr uint8_t lowerSleepSettle = 2;
      Utility::CircularBuffer<int16_t, historySize> xHistory = {};
      Utility::CircularBuffer<int16_t, historySize> yHistory = {};
      Utility::CircularBuffer<int16_t, historySize> zHistory = {};
      int32_t accumulatedSpeed = 0;

      DeviceTypes deviceType = DeviceTypes::Unknown;
      Pinetime::Controllers::MotionService* service = nullptr;
    };
  }
}
