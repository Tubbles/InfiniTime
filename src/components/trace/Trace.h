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

namespace Pinetime {
  namespace Controllers {
    class FS;

    // In-RAM BLE event trace for field debugging (doc/LOG.md 2026-08-03: the
    // DFU failures could not be attributed from phone-side logs alone). A
    // fixed ring of 12-byte records; readout is paged through the KeyTones
    // diagnostic characteristic, optionally persisted to /trace.bin.
    namespace Trace {
      enum EventType : uint8_t {
        AttErrorTx = 1,   // a = ATT opcode, b = attribute handle, c = error code
        Subscribe = 2,    // a = reason, b = attribute handle, c = cur_notify | cur_indicate << 1
        Announce = 3,     // a = 1 boot, 2 subscribe-triggered
        Gap = 4,          // a = 1 connect, 2 disconnect, 3 enc change; b = status/reason
        Bond = 5,         // a = 1 persist, 2 restore; b = CCCD count
        Dfu = 6,          // a = GATT op, b = attribute handle
        RevisionRead = 7, // b = revision value served
      };

      struct Record {
        uint32_t tick;
        uint8_t type;
        uint8_t a;
        uint16_t b;
        uint16_t c;
        uint16_t d;
      };

      void Event(uint8_t type, uint8_t a, uint16_t b, uint16_t c, uint16_t d);

      // Copies the ring (oldest first) into the readout buffer and rewinds
      // the read cursor. Returns the number of records captured.
      uint16_t Snapshot();

      // Pages the snapshot out: fills up to maxLength bytes, advances the
      // cursor, returns the number of bytes written (0 = done). The first
      // chunk starts with an 8-byte header: 'I','T','R','C', uint16 record
      // count, uint16 record size.
      uint16_t ReadChunk(uint8_t* buffer, uint16_t maxLength);

      // Persists the current snapshot to /trace.bin (header + records).
      int FlushToFile(Pinetime::Controllers::FS& fs);
    }
  }
}

extern "C" void infinitime_trace_event(uint8_t type, uint8_t a, uint16_t b, uint16_t c, uint16_t d);
