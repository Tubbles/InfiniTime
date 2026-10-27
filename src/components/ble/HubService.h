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

#include <array>
#include <cstdint>
#include <optional>

#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#include <host/ble_uuid.h>
#undef max
#undef min

int HubCallback(uint16_t connHandle, uint16_t attrHandle, struct ble_gatt_access_ctxt* ctxt, void* arg);

namespace Pinetime {
  namespace Controllers {
    class DateTime;

    // Upcoming events pushed by the Hub T phone app (first source: the 3D
    // printer's finish time), shown in the G7710 face's next-event corner.
    // Each event carries its own 1 bpp icon, so a new phone-side source
    // never needs a firmware build. RAM only: the phone re-sends its whole
    // desired state on every connect and poll.
    // Design: pinetime-hacks doc/DESIGN-hub.md.
    class HubService {
    public:
      static constexpr uint8_t maximumIconSize = 24;
      // Rows are padded to whole bytes, as the frame and LV_IMG_CF_INDEXED_1BIT both store them.
      static constexpr uint8_t maximumBitmapSize = (maximumIconSize + 7) / 8 * maximumIconSize;

      static constexpr uint16_t BitmapSize(uint8_t width, uint8_t height) {
        return static_cast<uint16_t>((width + 7) / 8 * height);
      }

      struct Event {
        uint32_t utcSeconds;
        uint8_t sourceId;
        uint8_t width;
        uint8_t height;
        // The service's write counter, bumped by every accepted set of any
        // source, so a re-sent event with the same time but a new icon still
        // reads as changed.
        uint8_t generation;
        std::array<uint8_t, maximumBitmapSize> bitmap;
      };

      explicit HubService(DateTime& dateTimeController);

      void Init();
      int OnWrite(struct ble_gatt_access_ctxt* ctxt);

      // The valid event with the smallest timestamp strictly after
      // nowUtcSeconds, copied out because the BLE task may overwrite a slot
      // at any time. Past events are skipped, not deleted: they expire by
      // themselves and the next set reuses their slot.
      std::optional<Event> Soonest(uint32_t nowUtcSeconds) const;

    private:
      static constexpr uint8_t frameVersion = 1;
      static constexpr uint8_t clearFrameSize = 4;
      static constexpr uint8_t setHeaderSize = 10;
      static constexpr uint8_t maximumFrameSize = setHeaderSize + maximumBitmapSize;
      static constexpr uint8_t slotCount = 4;

      enum class Opcode : uint8_t { Set = 1, ClearSource = 2, ClearAll = 3 };

      // Every byte here is static RAM taken from the heap (pinetime-hacks
      // doc/log/2026-08-10.md), hence 4 slots and no per-slot generation.
      struct EventSlot {
        uint32_t utcSeconds;
        uint8_t sourceId;
        uint8_t width;
        uint8_t height;
        bool valid;
        std::array<uint8_t, maximumBitmapSize> bitmap;
      };

      void Set(const uint8_t* frame, uint8_t width, uint8_t height);
      void ClearSource(uint8_t sourceId);
      void ClearAll();
      uint8_t SlotIndexForSet(uint8_t sourceId, uint32_t nowUtcSeconds) const;
      uint32_t NowUtcSeconds() const;

      // Service 00060000-78fc-48fe-8e23-433b3a1942d0, id 0006 reserved for it
      // by pinetime-hacks doc/DESIGN.md; the two low bytes select the 16-bit
      // id within the group (service 0000, events 0001; 0002 and 0003 are
      // reserved for the remote-app channel).
      static constexpr ble_uuid128_t CharUuid(uint8_t low, uint8_t high) {
        return ble_uuid128_t {.u = {.type = BLE_UUID_TYPE_128},
                              .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 0xfe, 0x48, 0xfc, 0x78, low, high, 0x06, 0x00}};
      }

      ble_uuid128_t hubUuid {CharUuid(0x00, 0x00)};
      ble_uuid128_t eventsCharUuid {CharUuid(0x01, 0x00)};

      // Write only: no notify means no CCCD, and the bond's CCCD store is
      // full (pinetime-hacks doc/log/2026-09-17.md).
      const struct ble_gatt_chr_def characteristicDefinition[2] = {
        {.uuid = &eventsCharUuid.u, .access_cb = HubCallback, .arg = this, .flags = BLE_GATT_CHR_F_WRITE, .val_handle = &eventsHandle},
        {0}};
      const struct ble_gatt_svc_def serviceDefinition[2] = {
        {.type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &hubUuid.u, .characteristics = characteristicDefinition},
        {0}};

      uint16_t eventsHandle {};

      DateTime& dateTimeController;

      std::array<EventSlot, slotCount> slots {};
      uint8_t writeGeneration = 0;
    };
  }
}
