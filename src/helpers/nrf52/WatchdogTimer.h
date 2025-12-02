#pragma once

#include <nrf_wdt.h>

#ifndef WATCHDOG_TIMEOUT_SECS
#define WATCHDOG_TIMEOUT_SECS 60
#endif

namespace Watchdog {
    inline void begin() {
        nrf_wdt_behaviour_set(NRF_WDT, NRF_WDT_BEHAVIOUR_RUN_SLEEP); // Run in sleep, pause in halt
        nrf_wdt_reload_value_set(NRF_WDT, WATCHDOG_TIMEOUT_SECS * 32768UL);
        nrf_wdt_reload_request_enable(NRF_WDT, NRF_WDT_RR0);
        nrf_wdt_task_trigger(NRF_WDT, NRF_WDT_TASK_START);
    }

    inline void feed() {
      if (nrf_wdt_started(NRF_WDT)) {
        nrf_wdt_reload_request_set(NRF_WDT, NRF_WDT_RR0);
      }
    }
}
