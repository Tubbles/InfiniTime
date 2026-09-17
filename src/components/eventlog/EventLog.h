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
  namespace System {
    class SystemTask;
  }

  namespace Controllers {
    class DateTime;
    class FS;

    /**
     * Human readable record of the rare and bad things that happen on the
     * watch, so the wearer learns about them without a laptop in the loop.
     * Design: pinetime-hacks doc/DESIGN-error-visibility.md.
     *
     * Each entry is one line, "YYYY-MM-DD HH:MM:SS text", appended to
     * /events.log. Once that file passes 4 KiB it is renamed to /events.old
     * and a fresh one starts, so the flash cost stays bounded at 8 KiB while
     * the previous generation survives.
     *
     * The timestamp comes from DateTime, which starts at its default epoch:
     * lines written before the phone sets the clock carry that epoch rather
     * than a real date. They are still in order, which is what a reader needs
     * from a boot line.
     *
     * Writes happen in the calling task (the BLE host task for DFU and CCCD
     * events, the system task at boot), like the bond persistence already
     * does, and there is no lock. The events are rare and the reads are user
     * initiated. If that ever bites, the fix belongs in FS, not here.
     *
     * The counterpart is the Trace ring, which stays the fine-grained
     * instrument: 64 records in RAM for forensics against a handful of
     * readable lines per week here.
     */
    namespace EventLog {
      // Stores the collaborators the logger needs. Call once, from
      // SystemTask::Work right after FS::Init. Log and LogAndNotify do
      // nothing until it has run.
      void Init(Pinetime::Controllers::FS& fs, Pinetime::Controllers::DateTime& dateTime, Pinetime::System::SystemTask& systemTask);

      // The file the lines land in, so that a reader (the Event log screen)
      // does not have to repeat the path.
      const char* LogPath();

      // Appends one timestamped line. Text that does not fit the line buffer
      // is truncated.
      void Log(const char* text);

      // Appends the line and raises a watch notification carrying title and
      // text, for the events the wearer must not miss.
      void LogAndNotify(const char* title, const char* text);
    }
  }
}

// Hook for the NimBLE host, which cannot see the C++ interface. Called from
// ble_gatts_clt_cfg_access when a subscription cannot be persisted.
extern "C" void infinitime_event_cccd_persist_failed(uint16_t chr_val_handle, int status, uint16_t conn_handle);
