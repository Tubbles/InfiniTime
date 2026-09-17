#include "displayapp/screens/settings/SettingEventLog.h"
#include <algorithm>
#include <cstring>
#include <lvgl/lvgl.h>
#include "components/eventlog/EventLog.h"
#include "components/fs/FS.h"
#include "displayapp/DisplayApp.h"
#include "displayapp/screens/Label.h"

using namespace Pinetime::Applications::Screens;

namespace {
  // The log rotates at 4 KiB, and a kibibyte of its tail holds around twenty
  // lines: more than the five pages can show, and small enough that the raw
  // tail and the page texts can sit on the heap at the same time while the
  // pages are built.
  constexpr uint32_t tailSize = 1024;

  constexpr size_t stampLength = 19;       // YYYY-MM-DD HH:MM:SS
  constexpr size_t yearLength = 5;         // YYYY-
  constexpr size_t dateAndTimeLength = 11; // MM-DD HH:MM
  constexpr size_t secondsLength = 3;      // :SS

  struct LogLine {
    const char* text;
    size_t length;
  };

  // A stamped line reads as "MM-DD HH:MM text" on the watch: the year and the
  // seconds cost screen width without telling the reader anything it did not
  // already know. A line too short to carry the stamp is copied as it stands.
  // Returns the number of bytes written, which is never more than lineLength.
  size_t CopyDisplayLine(char* destination, const char* line, size_t lineLength) {
    if (lineLength <= stampLength) {
      std::memcpy(destination, line, lineLength);
      return lineLength;
    }

    const size_t restLength = lineLength - stampLength;
    std::memcpy(destination, line + yearLength, dateAndTimeLength);
    std::memcpy(destination + dateAndTimeLength, line + yearLength + dateAndTimeLength + secondsLength, restLength);
    return dateAndTimeLength + restLength;
  }

  // Walks the buffer backwards so the newest line comes out first, which is
  // the order the pages want. Returns how many lines were found, at most
  // maximumLines.
  size_t CollectLines(const char* content, size_t contentLength, LogLine* lines, size_t maximumLines) {
    while (contentLength > 0 && content[contentLength - 1] == '\n') {
      contentLength--;
    }

    size_t lineCount = 0;
    size_t lineEnd = contentLength;
    while (lineCount < maximumLines && lineEnd > 0) {
      size_t lineStart = lineEnd;
      while (lineStart > 0 && content[lineStart - 1] != '\n') {
        lineStart--;
      }
      if (lineEnd > lineStart) {
        lines[lineCount] = {content + lineStart, lineEnd - lineStart};
        lineCount++;
      }
      lineEnd = lineStart > 0 ? lineStart - 1 : 0;
    }
    return lineCount;
  }
}

SettingEventLog::LogPages SettingEventLog::ReadPages(Pinetime::Controllers::FS& fileSystem) {
  LogPages pages = {};

  lfs_info info;
  if (fileSystem.Stat(Pinetime::Controllers::EventLog::LogPath(), &info) != 0 || info.size == 0) {
    return pages;
  }

  lfs_file_t file;
  if (fileSystem.FileOpen(&file, Pinetime::Controllers::EventLog::LogPath(), LFS_O_RDONLY) != 0) {
    return pages;
  }

  // Only the tail is read. What came before it stays in the file for the
  // laptop side (pinetime-hacks tools/pull_watch_file.py).
  const uint32_t startOffset = info.size > tailSize ? info.size - tailSize : 0;
  auto tail = std::make_unique<char[]>(tailSize + 1);
  fileSystem.FileSeek(&file, startOffset);
  const int readCount = fileSystem.FileRead(&file, reinterpret_cast<uint8_t*>(tail.get()), tailSize);
  fileSystem.FileClose(&file);
  if (readCount <= 0) {
    return pages;
  }
  tail[readCount] = '\0';

  const char* content = tail.get();
  if (startOffset > 0) {
    // The seek landed in the middle of a line, and that fragment is not a
    // record. When the whole read is one such fragment, nothing is left.
    const char* firstNewline = std::strchr(content, '\n');
    content = firstNewline != nullptr ? firstNewline + 1 : content + readCount;
  }

  std::array<LogLine, maximumLines> lines = {};
  const size_t lineCount = CollectLines(content, std::strlen(content), lines.data(), lines.size());
  if (lineCount == 0) {
    return pages;
  }

  // Every display line is at most as long as the file line it came from, and
  // takes one separator byte after it ('\n' between the lines of a page, '\0'
  // at the end of one), which is the byte the newline it replaces took in the
  // file. The tail size plus one for a file whose last line has no newline is
  // therefore always enough.
  pages.buffer = std::make_unique<char[]>(tailSize + 2);
  char* write = pages.buffer.get();
  for (uint8_t pageIndex = 0; pageIndex < pageCount; pageIndex++) {
    const size_t firstLine = static_cast<size_t>(pageIndex) * linesPerPage;
    if (firstLine >= lineCount) {
      break;
    }

    pages.texts[pageIndex] = write;
    const size_t endLine = std::min(firstLine + linesPerPage, lineCount);
    for (size_t index = firstLine; index < endLine; index++) {
      if (index > firstLine) {
        *write = '\n';
        write++;
      }
      write += CopyDisplayLine(write, lines[index].text, lines[index].length);
    }
    *write = '\0';
    write++;
  }

  return pages;
}

SettingEventLog::SettingEventLog(Pinetime::Applications::DisplayApp* app, Pinetime::Controllers::FS& fileSystem)
  : pages(ReadPages(fileSystem)),
    screens {app,
             0,
             {[this]() -> std::unique_ptr<Screen> {
                return CreatePage(0);
              },
              [this]() -> std::unique_ptr<Screen> {
                return CreatePage(1);
              },
              [this]() -> std::unique_ptr<Screen> {
                return CreatePage(2);
              },
              [this]() -> std::unique_ptr<Screen> {
                return CreatePage(3);
              },
              [this]() -> std::unique_ptr<Screen> {
                return CreatePage(4);
              }},
             Screens::ScreenListModes::UpDown} {
}

SettingEventLog::~SettingEventLog() {
  lv_obj_clean(lv_scr_act());
}

bool SettingEventLog::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  return screens.OnTouchEvent(event);
}

std::unique_ptr<Screen> SettingEventLog::CreatePage(uint8_t pageIndex) {
  lv_obj_t* label = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_long_mode(label, LV_LABEL_LONG_BREAK);
  lv_obj_set_width(label, LV_HOR_RES);
  if (pages.texts[pageIndex] != nullptr) {
    // A line wraps to about three rows in the 20 px font, and the screen fits
    // ten, so three lines per page. The newest is at the top either way.
    lv_label_set_text(label, pages.texts[pageIndex]);
  } else if (pageIndex == 0) {
    lv_label_set_text_static(label, "No events");
  } else {
    lv_label_set_text_static(label, "(no more)");
  }
  lv_obj_align(label, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 10);
  return std::make_unique<Screens::Label>(pageIndex, pageCount, label);
}
