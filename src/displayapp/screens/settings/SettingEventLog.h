#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/ScreenList.h"

namespace Pinetime {
  namespace Controllers {
    class FS;
  }

  namespace Applications {
    class DisplayApp;

    namespace Screens {
      /**
       * Reads the tail of the event log written by components/eventlog and
       * shows it as pages of lines, newest first, so the wearer can see what
       * the watch complained about without a laptop in the loop.
       * Design: pinetime-hacks doc/DESIGN-error-visibility.md.
       *
       * The file is read once, when the screen is created, into the heap:
       * nothing here is static, because every static byte is a heap byte lost
       * on this platform (doc/log/2026-08-10.md).
       */
      class SettingEventLog : public Screen {
      public:
        SettingEventLog(DisplayApp* app, Pinetime::Controllers::FS& fileSystem);
        ~SettingEventLog() override;

        bool OnTouchEvent(TouchEvents event) override;

      private:
        static constexpr uint8_t pageCount = 5;
        static constexpr uint8_t linesPerPage = 3;
        static constexpr size_t maximumLines = static_cast<size_t>(pageCount) * linesPerPage;

        // The page texts, laid out one after another in a single heap block,
        // with a pointer to the start of each. A page that has no lines left
        // to show keeps its null pointer.
        struct LogPages {
          std::unique_ptr<char[]> buffer;
          std::array<const char*, pageCount> texts {};
        };

        static LogPages ReadPages(Pinetime::Controllers::FS& fileSystem);

        std::unique_ptr<Screen> CreatePage(uint8_t pageIndex);

        // Declared before screens because the ScreenList constructor builds
        // the first page right away, and that page reads these texts.
        LogPages pages;
        ScreenList<pageCount> screens;
      };
    }
  }
}
