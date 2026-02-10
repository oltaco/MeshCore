#pragma once

#include <Arduino.h>
#include <helpers/ESP32Board.h>

class BetaFPVSuperXNanoBoard : public ESP32Board {

public:
  void begin();
  // void onBeforeTransmit(void) override;
  // void onAfterTransmit(void) override;
  // void enterDeepSleep(uint32_t secs, int pin_wake_btn = -1);
  // void powerOff() override;
  // uint16_t getBattMilliVolts() override;
  const char* getManufacturerName() const override ;

};




