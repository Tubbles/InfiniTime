/*  Copyright (C) 2026 InfiniTime contributors

    This file is part of InfiniTime.

    InfiniTime is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    InfiniTime is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "components/ble/ClockSyncService.h"
#include "components/ble/NimbleController.h"

#include <chrono>
#include <cstring>
#include <FreeRTOS.h>

using namespace Pinetime::Controllers;

namespace {
  uint32_t ReadUInt32(const uint8_t* data) {
    return data[0] + (data[1] << 8) + (static_cast<uint32_t>(data[2]) << 16) + (static_cast<uint32_t>(data[3]) << 24);
  }

  int64_t ReadInt64(const uint8_t* data) {
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
      value |= static_cast<uint64_t>(data[i]) << (8 * i);
    }
    return static_cast<int64_t>(value);
  }

  void WriteUInt32(uint8_t* data, uint32_t value) {
    for (int i = 0; i < 4; i++) {
      data[i] = (value >> (8 * i)) & 0xff;
    }
  }

  void WriteInt64(uint8_t* data, int64_t value) {
    auto unsignedValue = static_cast<uint64_t>(value);
    for (int i = 0; i < 8; i++) {
      data[i] = (unsignedValue >> (8 * i)) & 0xff;
    }
  }

  int64_t NowMs(Pinetime::Controllers::DateTime& dateTimeController) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(dateTimeController.CurrentDateTime().time_since_epoch()).count();
  }

  TickType_t MsToTicks(uint32_t milliseconds) {
    return static_cast<TickType_t>((static_cast<uint64_t>(milliseconds) * configTICK_RATE_HZ) / 1000);
  }

  uint32_t TicksToMs(TickType_t ticks) {
    return static_cast<uint32_t>((static_cast<uint64_t>(ticks) * 1000) / configTICK_RATE_HZ);
  }
}

int ClockSyncCallback(uint16_t /*connHandle*/, uint16_t /*attrHandle*/, struct ble_gatt_access_ctxt* ctxt, void* arg) {
  return static_cast<Pinetime::Controllers::ClockSyncService*>(arg)->OnCommand(ctxt);
}

ClockSyncService::ClockSyncService(NimbleController& nimble,
                                   DateTime& dateTimeController,
                                   StopWatchController& stopWatchController,
                                   Timer& timerController)
  : nimble {nimble}, dateTimeController {dateTimeController}, stopWatchController {stopWatchController}, timerController {timerController} {
}

void ClockSyncService::Init() {
  ble_gatts_count_cfg(serviceDefinition);
  ble_gatts_add_svcs(serviceDefinition);
}

int ClockSyncService::OnCommand(struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
    return 0;
  }
  const auto* buffer = ctxt->om;
  if (buffer->om_len < frameSize) {
    return 0;
  }
  const uint8_t* frame = buffer->om_data;
  if (frame[0] != frameVersion) {
    return 0;
  }

  const auto domain = static_cast<Domain>(frame[1]);
  const uint8_t state = frame[2];
  const uint32_t valueMs = ReadUInt32(&frame[4]);
  const int64_t referenceMs = ReadInt64(&frame[8]);

  if (domain == Domain::StopWatch) {
    switch (state) {
      case 1: { // running
        int64_t elapsedMs = NowMs(dateTimeController) - referenceMs;
        if (elapsedMs < 0) {
          elapsedMs = 0;
        }
        stopWatchController.SetState(true, MsToTicks(static_cast<uint32_t>(elapsedMs)));
        break;
      }
      case 2: // paused
        stopWatchController.SetState(false, MsToTicks(valueMs));
        break;
      case 0: // cleared
      default:
        stopWatchController.Clear();
        break;
    }
  } else if (domain == Domain::Timer) {
    switch (state) {
      case 1: { // running
        int64_t remainingMs = referenceMs - NowMs(dateTimeController);
        if (remainingMs > 0) {
          timerController.StartTimer(std::chrono::milliseconds {remainingMs});
        } else {
          timerController.StopTimer();
        }
        break;
      }
      case 0: // stopped
      case 2: // expired
      default:
        timerController.StopTimer();
        break;
    }
  }

  return 0;
}

void ClockSyncService::BuildStopWatchFrame(uint8_t* frame) const {
  std::memset(frame, 0, frameSize);
  frame[0] = frameVersion;
  frame[1] = static_cast<uint8_t>(Domain::StopWatch);
  if (stopWatchController.IsRunning()) {
    frame[2] = 1;
    uint32_t elapsedMs = TicksToMs(stopWatchController.GetElapsedTime());
    WriteInt64(&frame[8], NowMs(dateTimeController) - elapsedMs);
  } else if (stopWatchController.IsPaused()) {
    frame[2] = 2;
    WriteUInt32(&frame[4], TicksToMs(stopWatchController.GetElapsedTime()));
  } else {
    frame[2] = 0;
  }
}

void ClockSyncService::BuildTimerFrame(uint8_t* frame) {
  std::memset(frame, 0, frameSize);
  frame[0] = frameVersion;
  frame[1] = static_cast<uint8_t>(Domain::Timer);
  auto timerState = timerController.GetTimerState();
  if (timerState && !timerState->expired && timerController.IsRunning()) {
    frame[2] = 1;
    auto remainingMs = std::chrono::duration_cast<std::chrono::milliseconds>(timerState->distanceToExpiry).count();
    WriteUInt32(&frame[4], static_cast<uint32_t>(remainingMs));
    WriteInt64(&frame[8], NowMs(dateTimeController) + remainingMs);
  } else if (timerState && timerState->expired) {
    frame[2] = 2;
  } else {
    frame[2] = 0;
  }
}

void ClockSyncService::Notify(const uint8_t* frame) {
  uint16_t connectionHandle = nimble.connHandle();
  if (connectionHandle == 0 || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
    return;
  }
  auto* om = ble_hs_mbuf_from_flat(frame, frameSize);
  ble_gattc_notify_custom(connectionHandle, stateHandle, om);
}

void ClockSyncService::NotifyStopWatch() {
  uint8_t frame[frameSize];
  BuildStopWatchFrame(frame);
  Notify(frame);
}

void ClockSyncService::NotifyTimer() {
  uint8_t frame[frameSize];
  BuildTimerFrame(frame);
  Notify(frame);
}
