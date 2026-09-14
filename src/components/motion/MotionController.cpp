#include "components/motion/MotionController.h"

#include <task.h>

#include "utility/Math.h"

using namespace Pinetime::Controllers;

namespace {
  constexpr inline int32_t Clamp(int32_t val, int32_t min, int32_t max) {
    return val < min ? min : (val > max ? max : val);
  }

  // only returns meaningful values if inputs are acceleration due to gravity
  int16_t DegreesRolled(int16_t y, int16_t z, int16_t prevY, int16_t prevZ) {
    int16_t prevYAngle = Pinetime::Utility::Asin(Clamp(prevY * 32, -32767, 32767));
    int16_t yAngle = Pinetime::Utility::Asin(Clamp(y * 32, -32767, 32767));

    if (z < 0 && prevZ < 0) {
      return yAngle - prevYAngle;
    }
    if (prevZ < 0) {
      if (y < 0) {
        return -prevYAngle - yAngle - 180;
      }
      return -prevYAngle - yAngle + 180;
    }
    if (z < 0) {
      if (y < 0) {
        return prevYAngle + yAngle + 180;
      }
      return prevYAngle + yAngle - 180;
    }
    return prevYAngle - yAngle;
  }
}

void MotionController::AdvanceDay() {
  --nbSteps; // Higher index = further in the past
  SetSteps(Days::Today, 0);
  if (service != nullptr) {
    service->OnNewStepCountValue(NbSteps(Days::Today));
  }
}

void MotionController::Update(int16_t x, int16_t y, int16_t z, uint32_t nbSteps) {
  uint32_t oldSteps = NbSteps(Days::Today);
  if (oldSteps != nbSteps && service != nullptr) {
    service->OnNewStepCountValue(nbSteps);
  }

  if (service != nullptr && (xHistory[0] != x || yHistory[0] != y || zHistory[0] != z)) {
    service->OnNewMotionValues(x, y, z);
  }

  lastTime = time;
  time = xTaskGetTickCount();

  xHistory++;
  xHistory[0] = x;
  yHistory++;
  yHistory[0] = y;
  zHistory++;
  zHistory[0] = z;

  // Update accumulated speed
  // Currently polling at 10Hz, if this ever goes faster scalar and EMA might need adjusting
  // Index [size - 1] is the sample from the previous tick whatever the buffer
  // size is, so resizing the ring for the raise-wake window leaves the shake
  // speed untouched.
  int32_t speed = std::abs(zHistory[0] - zHistory[historySize - 1] + ((yHistory[0] - yHistory[historySize - 1]) / 2) +
                           ((xHistory[0] - xHistory[historySize - 1]) / 4)) *
                  100 / (time - lastTime);
  // integer version of (.2 * speed) + ((1 - .2) * accumulatedSpeed);
  accumulatedSpeed = speed / 5 + accumulatedSpeed * 4 / 5;

  int32_t deltaSteps = nbSteps - oldSteps;
  if (deltaSteps > 0) {
    currentTripSteps += deltaSteps;
  }
  SetSteps(Days::Today, nbSteps);
}

MotionController::AccelStats MotionController::GetAccelStats(uint8_t window, uint8_t settle) const {
  AccelStats stats;

  // Index 0 is the newest sample and index [size - k] is k ticks ago, so the
  // "now" group counts back from the newest and the "prev" group counts
  // forward from window-1 ticks ago.
  const uint8_t prevStart = historySize - window + 1;

  for (uint8_t i = 0; i < settle; i++) {
    stats.xMean += xHistory[historySize - i];
    stats.yMean += yHistory[historySize - i];
    stats.zMean += zHistory[historySize - i];
    stats.prevXMean += xHistory[prevStart + i];
    stats.prevYMean += yHistory[prevStart + i];
    stats.prevZMean += zHistory[prevStart + i];
  }
  stats.xMean /= settle;
  stats.yMean /= settle;
  stats.zMean /= settle;
  stats.prevXMean /= settle;
  stats.prevYMean /= settle;
  stats.prevZMean /= settle;

  for (uint8_t i = 0; i < settle; i++) {
    stats.xVariance += (xHistory[historySize - i] - stats.xMean) * (xHistory[historySize - i] - stats.xMean);
    stats.yVariance += (yHistory[historySize - i] - stats.yMean) * (yHistory[historySize - i] - stats.yMean);
    stats.zVariance += (zHistory[historySize - i] - stats.zMean) * (zHistory[historySize - i] - stats.zMean);
  }
  stats.xVariance /= settle;
  stats.yVariance /= settle;
  stats.zVariance /= settle;

  return stats;
}

bool MotionController::ShouldRaiseWake(const RaiseWakeThresholds& thresholds) const {
  // Two independent sliders set the window and the settle count, so the
  // relationship between them is enforced here rather than in the UI.
  const uint8_t window = Clamp(thresholds.window, 2, historySize);
  const uint8_t settle = Clamp(thresholds.settle, 1, window - 1);
  const AccelStats stats = GetAccelStats(window, settle);

  const uint32_t varianceThresh = static_cast<uint32_t>(thresholds.stillness) * thresholds.stillness;
  const int16_t xThresh = thresholds.level;
  const int16_t yThresh = -static_cast<int16_t>(thresholds.tilt);
  const int16_t rollDegreesThresh = -static_cast<int16_t>(thresholds.rollDegrees);

  if (std::abs(stats.xMean) > xThresh) {
    return false;
  }

  // if the variance is below the threshold, the accelerometer values can be considered to be from acceleration due to gravity
  if (stats.yVariance > varianceThresh || (stats.yMean < -724 && stats.zVariance > varianceThresh) || stats.yMean > yThresh) {
    return false;
  }

  return DegreesRolled(stats.yMean, stats.zMean, stats.prevYMean, stats.prevZMean) < rollDegreesThresh;
}

bool MotionController::ShouldLowerSleep(const LowerSleepThresholds& thresholds) const {
  const AccelStats stats = GetAccelStats(lowerSleepWindow, lowerSleepSettle);

  const int16_t sideLevel = thresholds.sideLevel;
  const int16_t sideRollDegrees = thresholds.sideRollDegrees;

  if ((stats.xMean > sideLevel && DegreesRolled(stats.xMean, stats.zMean, stats.prevXMean, stats.prevZMean) > sideRollDegrees) ||
      (stats.xMean < -sideLevel && DegreesRolled(stats.xMean, stats.zMean, stats.prevXMean, stats.prevZMean) < -sideRollDegrees)) {
    return true;
  }

  if (stats.yMean < static_cast<int16_t>(thresholds.facingLevel) ||
      DegreesRolled(stats.yMean, stats.zMean, stats.prevYMean, stats.prevZMean) < thresholds.rollDegrees) {
    return false;
  }

  // Every sample between the two groups must also be raised: indices below
  // this belong to the "prev" group, index 0 to the "now" group.
  for (uint8_t i = historySize - lowerSleepWindow + lowerSleepSettle + 1; i < historySize; i++) {
    if (yHistory[i] < static_cast<int16_t>(thresholds.historyFloor)) {
      return false;
    }
  }

  return true;
}

void MotionController::Init(Pinetime::Drivers::Bma421::DeviceTypes types) {
  switch (types) {
    case Drivers::Bma421::DeviceTypes::BMA421:
      this->deviceType = DeviceTypes::BMA421;
      break;
    case Drivers::Bma421::DeviceTypes::BMA425:
      this->deviceType = DeviceTypes::BMA425;
      break;
    default:
      this->deviceType = DeviceTypes::Unknown;
      break;
  }
}
