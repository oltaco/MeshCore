#include <Arduino.h>
#include "target.h"


BetaFPVSuperXNanoBoard board;

#ifndef LORA_CR
#define LORA_CR 5
#endif

  static SPIClass spi(FSPI); // ESP32-C3 only has FSPI.

  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
SensorManager sensors;


#ifdef RF_SWITCH_TABLE
static const uint32_t rfswitch_dios[Module::RFSWITCH_MAX_PINS] = {
  RADIOLIB_LR11X0_DIO5, // RFSW0
  RADIOLIB_LR11X0_DIO6, // RFSW1
  RADIOLIB_LR11X0_DIO7, // RFSW2
  RADIOLIB_LR11X0_DIO8, // RFSW3
  RADIOLIB_NC
};
                    
// ExpressLRS "radio_rfsw_ctrl": [15, 0, 4, 8, 8, 14, 0, 13]
static const Module::RfSwitchMode_t rfswitch_table[] = {
  // mode             DIO 5   6   7   8
  { LR11x0::MODE_STBY,   {0,  0,  0,  0  }},
  { LR11x0::MODE_RX,     {0,  0,  1,  0  }},
  { LR11x0::MODE_TX,     {0,  0,  0,  1  }},
  { LR11x0::MODE_TX_HP,  {0,  0,  0,  1  }},
  { LR11x0::MODE_TX_HF,  {0,  1,  1,  1  }},
  END_OF_MODE_TABLE,
};
#endif


bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);

#ifdef LR11X0_DIO3_TCXO_VOLTAGE
  float tcxo = LR11X0_DIO3_TCXO_VOLTAGE;
#else
  float tcxo = 1.8f;
#endif

  spi.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI, P_LORA_NSS);
  
  // Deselect second radio
  pinMode(P_LORA_NSS_2, OUTPUT);
  digitalWrite(P_LORA_NSS_2, HIGH);
  
  int status = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE, LORA_TX_POWER, 16, tcxo);
  if (status != RADIOLIB_ERR_NONE) {
    Serial.print("ERROR: radio init failed: ");
    Serial.println(status);
    return false;  // fail
  }
  
  radio.setCRC(2);
  radio.explicitHeader();
  
  radio.setRfSwitchTable(rfswitch_dios, rfswitch_table);

#ifdef RX_BOOSTED_GAIN
  radio.setRxBoostedGainMode(RX_BOOSTED_GAIN);
#endif

  return true;  // success
}


uint32_t radio_get_rng_seed() {
  return radio.random(0x7FFFFFFF);
}

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  radio.setFrequency(freq);
  radio.setSpreadingFactor(sf);
  radio.setBandwidth(bw);
  radio.setCodingRate(cr);
}

void radio_set_tx_power(uint8_t dbm) {
  radio.setOutputPower(dbm);
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}
