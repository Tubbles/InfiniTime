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

#include "components/ble/KeyTonesService.h"
#include "components/ble/NimbleController.h"

using namespace Pinetime::Controllers;

int KeyTonesCallback(uint16_t /*connHandle*/, uint16_t /*attrHandle*/, struct ble_gatt_access_ctxt* /*ctxt*/, void* /*arg*/) {
  // Notify-only characteristic: nothing to read or write.
  return 0;
}

KeyTonesService::KeyTonesService(NimbleController& nimble) : nimble {nimble} {
}

void KeyTonesService::Init() {
  ble_gatts_count_cfg(serviceDefinition);
  ble_gatts_add_svcs(serviceDefinition);
}

void KeyTonesService::NotifyKey(char key) {
  uint16_t connectionHandle = nimble.connHandle();
  if (connectionHandle == 0 || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
    return;
  }
  auto payload = static_cast<uint8_t>(key);
  auto* om = ble_hs_mbuf_from_flat(&payload, 1);
  ble_gattc_notify_custom(connectionHandle, keyHandle, om);
}
