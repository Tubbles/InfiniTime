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
#include "components/trace/Trace.h"
#include "components/fs/FS.h"

#include <FreeRTOS.h>
#include <task.h>

#include <cstring>

using namespace Pinetime::Controllers;

namespace {
  constexpr uint16_t ringSize = 256;
  Trace::Record ring[ringSize];
  uint16_t ringHead = 0;   // next slot to write
  uint32_t totalEvents = 0;

  Trace::Record snapshotBuffer[ringSize];
  uint16_t snapshotCount = 0;
  uint32_t readCursor = 0; // byte offset into header + records
  constexpr uint8_t headerSize = 8;
}

void Trace::Event(uint8_t type, uint8_t a, uint16_t b, uint16_t c, uint16_t d) {
  taskENTER_CRITICAL();
  Record& record = ring[ringHead];
  record.tick = xTaskGetTickCount();
  record.type = type;
  record.a = a;
  record.b = b;
  record.c = c;
  record.d = d;
  ringHead = (ringHead + 1) % ringSize;
  totalEvents++;
  taskEXIT_CRITICAL();
}

uint16_t Trace::Snapshot() {
  taskENTER_CRITICAL();
  const uint16_t count = totalEvents < ringSize ? ringHead : ringSize;
  const uint16_t oldest = totalEvents < ringSize ? 0 : ringHead;
  for (uint16_t index = 0; index < count; index++) {
    snapshotBuffer[index] = ring[(oldest + index) % ringSize];
  }
  snapshotCount = count;
  readCursor = 0;
  taskEXIT_CRITICAL();
  return count;
}

uint16_t Trace::ReadChunk(uint8_t* buffer, uint16_t maxLength) {
  uint8_t header[headerSize] = {'I', 'T', 'R', 'C', 0, 0, 0, 0};
  header[4] = snapshotCount & 0xff;
  header[5] = snapshotCount >> 8;
  header[6] = sizeof(Record) & 0xff;
  header[7] = sizeof(Record) >> 8;

  const uint32_t totalLength = headerSize + snapshotCount * sizeof(Record);
  uint16_t written = 0;
  while (written < maxLength && readCursor < totalLength) {
    if (readCursor < headerSize) {
      buffer[written] = header[readCursor];
    } else {
      buffer[written] = reinterpret_cast<const uint8_t*>(snapshotBuffer)[readCursor - headerSize];
    }
    readCursor++;
    written++;
  }
  return written;
}

int Trace::FlushToFile(Pinetime::Controllers::FS& fs) {
  lfs_file_t file;
  int result = fs.FileOpen(&file, "/trace.bin", LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
  if (result != 0) {
    return result;
  }
  uint8_t header[headerSize] = {'I', 'T', 'R', 'C', 0, 0, 0, 0};
  header[4] = snapshotCount & 0xff;
  header[5] = snapshotCount >> 8;
  header[6] = sizeof(Record) & 0xff;
  header[7] = sizeof(Record) >> 8;
  fs.FileWrite(&file, header, headerSize);
  fs.FileWrite(&file, reinterpret_cast<const uint8_t*>(snapshotBuffer), snapshotCount * sizeof(Record));
  fs.FileClose(&file);
  return 0;
}

extern "C" void infinitime_trace_event(uint8_t type, uint8_t a, uint16_t b, uint16_t c, uint16_t d) {
  Trace::Event(type, a, b, c, d);
}
