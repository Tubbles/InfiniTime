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

int KeyTonesCallback(uint16_t connHandle, uint16_t attrHandle, struct ble_gatt_access_ctxt* ctxt, void* arg);

namespace Pinetime {
  namespace System {
    class SystemTask;
  }

  namespace Controllers {
    class NimbleController;

    // In-call service: sends DTMF key presses to the companion app (one ASCII
    // byte per press, '0'-'9'/'*'/'#', plus 'E' to end and 'A' to answer the
    // call, notify-only, fire-and-forget) and
    // receives call state from a Gadgetbridge fork (1 byte: 0 = call ended,
    // 1 = call active) to open/close the InCall screen. Stock Gadgetbridge
    // never writes the call-state characteristic; everything still works, the
    // screen is just opened and closed by hand.
    // Design: doc/DESIGN-intercom-keytones.md (pinetime-hacks repo).
    class KeyTonesService {
    public:
      KeyTonesService(NimbleController& nimble, Pinetime::System::SystemTask& systemTask);

      void Init();
      void NotifyKey(char key);
      int OnCallState(uint16_t attributeHandle, struct ble_gatt_access_ctxt* ctxt);

    private:
      // Service 00080000-78fc-48fe-8e23-433b3a1942d0; the two low bytes select
      // the 16-bit id within the group (service 0000, key 0001, call state
      // 0002, CCCD diagnostic 0003).
      static constexpr ble_uuid128_t CharUuid(uint8_t low, uint8_t high) {
        return ble_uuid128_t {.u = {.type = BLE_UUID_TYPE_128},
                              .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 0xfe, 0x48, 0xfc, 0x78, low, high, 0x08, 0x00}};
      }

      ble_uuid128_t keyTonesUuid {CharUuid(0x00, 0x00)};
      ble_uuid128_t keyCharUuid {CharUuid(0x01, 0x00)};
      ble_uuid128_t callStateCharUuid {CharUuid(0x02, 0x00)};
      // Diagnostics (nRF Connect friendly). WRITE 0x01: snapshot the BLE
      // trace ring (components/trace) and rewind the read cursor; WRITE 0x02:
      // also persist it to /trace.bin. READ: pages the snapshot out (~200 B
      // per read, empty read = done); with no snapshot taken it returns the
      // legacy ble_gatts_diag struct (host/ble_gatt.h), the CCCD permission
      // table captured at the last refused subscription.
      ble_uuid128_t diagCharUuid {CharUuid(0x03, 0x00)};

      const struct ble_gatt_chr_def characteristicDefinition[4] = {
        {.uuid = &keyCharUuid.u, .access_cb = KeyTonesCallback, .arg = this, .flags = BLE_GATT_CHR_F_NOTIFY, .val_handle = &keyHandle},
        {.uuid = &callStateCharUuid.u,
         .access_cb = KeyTonesCallback,
         .arg = this,
         .flags = BLE_GATT_CHR_F_WRITE,
         .val_handle = &callStateHandle},
        {.uuid = &diagCharUuid.u,
         .access_cb = KeyTonesCallback,
         .arg = this,
         .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
         .val_handle = &diagHandle},
        {0}};
      const struct ble_gatt_svc_def serviceDefinition[2] = {
        {.type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &keyTonesUuid.u, .characteristics = characteristicDefinition},
        {0}};

      uint16_t keyHandle {};
      uint16_t callStateHandle {};
      uint16_t diagHandle {};

      NimbleController& nimble;
      Pinetime::System::SystemTask& systemTask;
    };
  }
}
