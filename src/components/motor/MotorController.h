#pragma once

#include <FreeRTOS.h>
#include <timers.h>
#include <cstdint>

namespace Pinetime {
  namespace Controllers {

    class MotorController {
    public:
      MotorController() = default;

      void Init();
      void RunForDuration(uint8_t motorDuration);
      // Two 100 ms buzzes with a 150 ms gap: the connection-lost warning,
      // distinct from the single short notification buzz and the alarm ring.
      void RunDoubleBuzz();
      void StartRinging();
      void StopRinging();
      bool IsRinging();

    private:
      static void Ring(TimerHandle_t xTimer);
      static void RunSecondBuzz(TimerHandle_t xTimer);
      static void StopMotor(TimerHandle_t xTimer);
      TimerHandle_t shortVib;
      TimerHandle_t longVib;
      TimerHandle_t doubleVib;
    };
  }
}
