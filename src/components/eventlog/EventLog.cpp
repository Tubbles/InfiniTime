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
#include "components/eventlog/EventLog.h"
#include "components/ble/NotificationManager.h"
#include "components/datetime/DateTimeController.h"
#include "components/fs/FS.h"
#include "systemtask/SystemTask.h"

#include <cstdio>
#include <cstring>
#include <utility>

using namespace Pinetime::Controllers;

namespace {
  // Pointers only: the heap is what is left after static data
  // (heap_4_infinitime.c), so every line is formatted on the caller's stack
  // instead of in a buffer that lives here (doc/log/2026-08-10).
  Pinetime::Controllers::FS* fileSystem = nullptr;
  Pinetime::Controllers::DateTime* dateTimeController = nullptr;
  Pinetime::System::SystemTask* systemTaskController = nullptr;

  constexpr const char* logPath = "/events.log";
  constexpr const char* previousLogPath = "/events.old";
  constexpr uint32_t maximumLogSize = 4096;
  constexpr size_t lineBufferSize = 96;

  // Kept out of Log so that the file handle and the lfs_info of the rotation
  // check never sit on the stack at the same time: both are large enough to
  // matter on the system task.
  void AppendLine(const char* line, size_t length) {
    lfs_file_t file;
    if (fileSystem->FileOpen(&file, logPath, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND) != 0) {
      return;
    }
    fileSystem->FileWrite(&file, reinterpret_cast<const uint8_t*>(line), length);
    fileSystem->FileClose(&file);
  }

  void RotateIfFull() {
    lfs_info info;
    if (fileSystem->Stat(logPath, &info) != 0 || info.size <= maximumLogSize) {
      return;
    }
    // There is no previous generation on the first rotation, and a missing
    // file is the expected error then.
    fileSystem->FileDelete(previousLogPath);
    fileSystem->Rename(logPath, previousLogPath);
  }
}

void EventLog::Init(Pinetime::Controllers::FS& fs, Pinetime::Controllers::DateTime& dateTime, Pinetime::System::SystemTask& systemTask) {
  fileSystem = &fs;
  dateTimeController = &dateTime;
  systemTaskController = &systemTask;
}

const char* EventLog::LogPath() {
  return logPath;
}

void EventLog::Log(const char* text) {
  if (fileSystem == nullptr || dateTimeController == nullptr) {
    return;
  }

  char line[lineBufferSize];
  // Integer directives only: even their widest possible expansion fits the
  // buffer, so the format itself can never truncate. The text is appended by
  // hand below because the compiler cannot bound its length, and a %s it
  // cannot bound is what -Wformat-truncation=2 rejects.
  const int prefixLength = snprintf(line,
                                    sizeof(line),
                                    "%04u-%02u-%02u %02u:%02u:%02u ",
                                    static_cast<unsigned int>(dateTimeController->Year()),
                                    static_cast<unsigned int>(dateTimeController->Month()),
                                    static_cast<unsigned int>(dateTimeController->Day()),
                                    static_cast<unsigned int>(dateTimeController->Hours()),
                                    static_cast<unsigned int>(dateTimeController->Minutes()),
                                    static_cast<unsigned int>(dateTimeController->Seconds()));
  if (prefixLength <= 0 || static_cast<size_t>(prefixLength) >= sizeof(line)) {
    return;
  }

  size_t length = static_cast<size_t>(prefixLength);
  size_t textIndex = 0;
  // One byte stays reserved for the newline, so a long text loses its tail
  // rather than the line separator.
  while (text[textIndex] != '\0' && length + 1 < sizeof(line)) {
    line[length] = text[textIndex];
    length++;
    textIndex++;
  }
  line[length] = '\n';
  length++;

  AppendLine(line, length);
  RotateIfFull();
}

void EventLog::LogAndNotify(const char* title, const char* text) {
  Log(text);
  if (systemTaskController == nullptr) {
    return;
  }

  // Same shape as the file service's access denied alert: title, NUL, body,
  // NUL, with the size counting both terminators.
  constexpr size_t maximumSize = Pinetime::Controllers::NotificationManager::MaximumMessageSize();
  size_t titleLength = std::strlen(title);
  if (titleLength > maximumSize - 2) {
    titleLength = maximumSize - 2;
  }
  size_t textLength = std::strlen(text);
  if (titleLength + textLength + 2 > maximumSize) {
    textLength = maximumSize - titleLength - 2;
  }

  Pinetime::Controllers::NotificationManager::Notification notification;
  char* message = notification.message.data();
  std::memcpy(message, title, titleLength);
  message[titleLength] = '\0';
  std::memcpy(message + titleLength + 1, text, textLength);
  message[titleLength + 1 + textLength] = '\0';
  notification.size = static_cast<uint8_t>(titleLength + textLength + 2);
  notification.category = Pinetime::Controllers::NotificationManager::Categories::SimpleAlert;

  systemTaskController->GetNotificationManager().Push(std::move(notification));
  systemTaskController->PushMessage(Pinetime::System::Messages::OnNewNotification);
}

extern "C" void infinitime_event_cccd_persist_failed(uint16_t chr_val_handle, int status, uint16_t conn_handle) {
  char text[80];
  snprintf(text,
           sizeof(text),
           "CCCD 0x%04x not saved: store status %d (conn %u)",
           static_cast<unsigned int>(chr_val_handle),
           status,
           static_cast<unsigned int>(conn_handle));
  EventLog::LogAndNotify("BLE", text);
}
