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

#include "components/ble/HubService.h"
#include "components/datetime/DateTimeController.h"

#include <chrono>
#include <cstring>
#include <FreeRTOS.h>
#include <task.h>

using namespace Pinetime::Controllers;

namespace {
  uint32_t ReadUInt32(const uint8_t* data) {
    return data[0] + (data[1] << 8) + (static_cast<uint32_t>(data[2]) << 16) + (static_cast<uint32_t>(data[3]) << 24);
  }

  bool IsValidIconSize(uint8_t size) {
    return size >= 1 && size <= HubService::maximumIconSize;
  }
}

int HubCallback(uint16_t /*connHandle*/, uint16_t /*attrHandle*/, struct ble_gatt_access_ctxt* ctxt, void* arg) {
  return static_cast<Pinetime::Controllers::HubService*>(arg)->OnWrite(ctxt);
}

HubService::HubService(DateTime& dateTimeController) : dateTimeController {dateTimeController} {
}

void HubService::Init() {
  ble_gatts_count_cfg(serviceDefinition);
  ble_gatts_add_svcs(serviceDefinition);
}

// Anything but a fully valid frame is ignored with a success status, like
// the other phone-written services, and leaves the table untouched.
int HubService::OnWrite(struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
    return 0;
  }
  // The packet length, not om_len: a long (prepared) write arrives as an mbuf chain.
  const uint16_t length = OS_MBUF_PKTLEN(ctxt->om);
  if (length < clearFrameSize || length > maximumFrameSize) {
    return 0;
  }
  uint8_t frame[maximumFrameSize];
  if (os_mbuf_copydata(ctxt->om, 0, length, frame) != 0) {
    return 0;
  }
  if (frame[0] != frameVersion) {
    return 0;
  }
  // The source byte means nothing to clear-all, so only the opcodes that use
  // it reject the reserved value 0.
  const uint8_t sourceId = frame[2];
  const bool isClearAll = static_cast<Opcode>(frame[1]) == Opcode::ClearAll;
  if (sourceId == 0 && !isClearAll) {
    return 0;
  }

  switch (static_cast<Opcode>(frame[1])) {
    case Opcode::Set: {
      if (length < setHeaderSize) {
        return 0;
      }
      const uint8_t width = frame[8];
      const uint8_t height = frame[9];
      if (!IsValidIconSize(width) || !IsValidIconSize(height) || length != setHeaderSize + BitmapSize(width, height)) {
        return 0;
      }
      Set(frame, width, height);
      break;
    }
    case Opcode::ClearSource:
      if (length == clearFrameSize) {
        ClearSource(sourceId);
      }
      break;
    case Opcode::ClearAll:
      if (length == clearFrameSize) {
        ClearAll();
      }
      break;
    default:
      break;
  }
  return 0;
}

// Called with a fully validated frame. The critical section keeps the
// display task from copying a half-written slot.
void HubService::Set(const uint8_t* frame, uint8_t width, uint8_t height) {
  const uint8_t sourceId = frame[2];
  const uint32_t nowUtcSeconds = NowUtcSeconds();
  taskENTER_CRITICAL();
  EventSlot& slot = slots[SlotIndexForSet(sourceId, nowUtcSeconds)];
  slot.utcSeconds = ReadUInt32(&frame[4]);
  slot.sourceId = sourceId;
  slot.width = width;
  slot.height = height;
  slot.valid = true;
  std::memcpy(slot.bitmap.data(), &frame[setHeaderSize], BitmapSize(width, height));
  writeGeneration++;
  taskEXIT_CRITICAL();
}

void HubService::ClearSource(uint8_t sourceId) {
  taskENTER_CRITICAL();
  for (auto& slot : slots) {
    if (slot.valid && slot.sourceId == sourceId) {
      slot.valid = false;
    }
  }
  taskEXIT_CRITICAL();
}

void HubService::ClearAll() {
  taskENTER_CRITICAL();
  for (auto& slot : slots) {
    slot.valid = false;
  }
  taskEXIT_CRITICAL();
}

// One slot per source. With all four taken by other sources, an expired
// event is the cheapest loss; failing that, the farthest one, since the
// corner shows only the soonest.
uint8_t HubService::SlotIndexForSet(uint8_t sourceId, uint32_t nowUtcSeconds) const {
  for (uint8_t index = 0; index < slotCount; index++) {
    if (slots[index].valid && slots[index].sourceId == sourceId) {
      return index;
    }
  }
  for (uint8_t index = 0; index < slotCount; index++) {
    if (!slots[index].valid) {
      return index;
    }
  }
  for (uint8_t index = 0; index < slotCount; index++) {
    if (slots[index].utcSeconds <= nowUtcSeconds) {
      return index;
    }
  }
  uint8_t farthestIndex = 0;
  for (uint8_t index = 1; index < slotCount; index++) {
    if (slots[index].utcSeconds > slots[farthestIndex].utcSeconds) {
      farthestIndex = index;
    }
  }
  return farthestIndex;
}

std::optional<HubService::Event> HubService::Soonest(uint32_t nowUtcSeconds) const {
  std::optional<Event> soonest;
  taskENTER_CRITICAL();
  const EventSlot* soonestSlot = nullptr;
  for (const auto& slot : slots) {
    if (slot.valid && slot.utcSeconds > nowUtcSeconds && (soonestSlot == nullptr || slot.utcSeconds < soonestSlot->utcSeconds)) {
      soonestSlot = &slot;
    }
  }
  if (soonestSlot != nullptr) {
    soonest =
      Event {soonestSlot->utcSeconds, soonestSlot->sourceId, soonestSlot->width, soonestSlot->height, writeGeneration, soonestSlot->bitmap};
  }
  taskEXIT_CRITICAL();
  return soonest;
}

uint32_t HubService::NowUtcSeconds() const {
  return static_cast<uint32_t>(
    std::chrono::duration_cast<std::chrono::seconds>(dateTimeController.UTCDateTime().time_since_epoch()).count());
}
