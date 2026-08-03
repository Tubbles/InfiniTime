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
#include "systemtask/SystemTask.h"

using namespace Pinetime::Controllers;

int KeyTonesCallback(uint16_t /*connHandle*/, uint16_t /*attrHandle*/, struct ble_gatt_access_ctxt* ctxt, void* arg) {
  return static_cast<Pinetime::Controllers::KeyTonesService*>(arg)->OnCallState(ctxt);
}

KeyTonesService::KeyTonesService(NimbleController& nimble, Pinetime::System::SystemTask& systemTask)
  : nimble {nimble}, systemTask {systemTask} {
}

int KeyTonesService::OnCallState(struct ble_gatt_access_ctxt* ctxt) {
  // The only readable characteristic in this service is the CCCD diagnostic;
  // return the raw ble_gatts_diag snapshot (host/ble_gatt.h).
  if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
    int res = os_mbuf_append(ctxt->om, &ble_gatts_diag, sizeof(ble_gatts_diag));
    return res == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }
  // Otherwise only the call-state characteristic is writable; the key
  // characteristic never produces an access op.
  if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
    return 0;
  }
  if (ctxt->om->om_len < 1) {
    return 0;
  }
  switch (ctxt->om->om_data[0]) {
    case 1:
      systemTask.PushMessage(System::Messages::CallStarted);
      break;
    case 0:
      systemTask.PushMessage(System::Messages::CallEnded);
      break;
    default:
      break;
  }
  return 0;
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
