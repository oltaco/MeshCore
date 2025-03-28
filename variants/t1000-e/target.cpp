#include <Arduino.h>
#include "target.h"

T1000eBoard board;

RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI);

#ifndef LORA_CR
  #define LORA_CR      5
#endif

#ifdef RF_SWITCH_TABLE
static const uint32_t rfswitch_dios[Module::RFSWITCH_MAX_PINS] = {
  RADIOLIB_LR11X0_DIO5,
  RADIOLIB_LR11X0_DIO6,
  RADIOLIB_LR11X0_DIO7,
  RADIOLIB_LR11X0_DIO8, 
  RADIOLIB_NC
};

static const Module::RfSwitchMode_t rfswitch_table[] = {
  // mode                 DIO5  DIO6  DIO7  DIO8
  { LR11x0::MODE_STBY,   {LOW,  LOW,  LOW,  LOW  }},  
  { LR11x0::MODE_RX,     {HIGH, LOW,  LOW,  HIGH }},
  { LR11x0::MODE_TX,     {HIGH, HIGH, LOW,  HIGH }},
  { LR11x0::MODE_TX_HP,  {LOW,  HIGH, LOW,  HIGH }},
  { LR11x0::MODE_TX_HF,  {LOW,  LOW,  LOW,  LOW  }}, 
  { LR11x0::MODE_GNSS,   {LOW,  LOW,  HIGH, LOW  }},
  { LR11x0::MODE_WIFI,   {LOW,  LOW,  LOW,  LOW  }},  
  END_OF_MODE_TABLE,
};
#endif

bool radio_init() {
#ifdef LR11X0_DIO3_TCXO_VOLTAGE
  float tcxo = LR11X0_DIO3_TCXO_VOLTAGE;
#else
  float tcxo = 1.6f;
#endif

  SPI.setPins(P_LORA_MISO, P_LORA_SCLK, P_LORA_MOSI);
  SPI.begin();
  int status = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE, LORA_TX_POWER, 8, tcxo);
  if (status != RADIOLIB_ERR_NONE) {
    Serial.print("ERROR: radio init failed: ");
    Serial.println(status);
    return false;  // fail
  }
  
  radio.setCRC(1);

#ifdef RF_SWITCH_TABLE
  radio.setRfSwitchTable(rfswitch_dios, rfswitch_table);
#endif
#ifdef RX_BOOSTED_GAIN
  radio.setRxBoostedGainMode(RX_BOOSTED_GAIN);
#endif

  return true;  // success
}
