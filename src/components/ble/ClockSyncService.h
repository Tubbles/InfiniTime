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
#pragma once

#include <cstdint>

#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#include <host/ble_uuid.h>
#undef max
#undef min

#include "components/datetime/DateTimeController.h"
#include "components/stopwatch/StopWatchController.h"
#include "components/timer/Timer.h"

int ClockSyncCallback(uint16_t connHandle, uint16_t attrHandle, struct ble_gatt_access_ctxt* ctxt, void* arg);

namespace Pinetime {
  namespace Controllers {
    class NimbleController;

    // Bidirectional stopwatch/timer sync with a companion app.
    // Wire protocol v1 is documented in doc/DESIGN-clock-sync.md.
    class ClockSyncService {
    public:
      ClockSyncService(NimbleController& nimble,
                       DateTime& dateTimeController,
                       StopWatchController& stopWatchController,
                       Timer& timerController);

      void Init();
      int OnCommand(struct ble_gatt_access_ctxt* ctxt);

      // Push the current state to the companion app (watch -> phone).
      void NotifyStopWatch();
      void NotifyTimer();

    private:
      static constexpr uint8_t frameVersion = 1;
      static constexpr uint8_t frameSize = 16;
      enum class Domain : uint8_t { StopWatch = 0, Timer = 1 };

      void BuildStopWatchFrame(uint8_t* frame) const;
      void BuildTimerFrame(uint8_t* frame);
      void Notify(const uint8_t* frame);

      // Service 00070000-78fc-48fe-8e23-433b3a1942d0; the two low bytes select
      // the 16-bit id within the group (service 0000, control 0001, state 0002).
      static constexpr ble_uuid128_t CharUuid(uint8_t low, uint8_t high) {
        return ble_uuid128_t {.u = {.type = BLE_UUID_TYPE_128},
                              .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 0xfe, 0x48, 0xfc, 0x78, low, high, 0x07, 0x00}};
      }

      ble_uuid128_t clockSyncUuid {CharUuid(0x00, 0x00)};
      ble_uuid128_t controlCharUuid {CharUuid(0x01, 0x00)};
      ble_uuid128_t stateCharUuid {CharUuid(0x02, 0x00)};

      const struct ble_gatt_chr_def characteristicDefinition[3] = {{.uuid = &controlCharUuid.u,
                                                                    .access_cb = ClockSyncCallback,
                                                                    .arg = this,
                                                                    .flags = BLE_GATT_CHR_F_WRITE,
                                                                    .val_handle = &controlHandle},
                                                                   {.uuid = &stateCharUuid.u,
                                                                    .access_cb = ClockSyncCallback,
                                                                    .arg = this,
                                                                    .flags = BLE_GATT_CHR_F_NOTIFY,
                                                                    .val_handle = &stateHandle},
                                                                   {0}};
      const struct ble_gatt_svc_def serviceDefinition[2] = {
        {.type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &clockSyncUuid.u, .characteristics = characteristicDefinition},
        {0}};

      uint16_t controlHandle {};
      uint16_t stateHandle {};

      NimbleController& nimble;
      DateTime& dateTimeController;
      StopWatchController& stopWatchController;
      Timer& timerController;
    };
  }
}
