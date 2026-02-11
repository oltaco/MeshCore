#include <Arduino.h>
#include "target.h"


BetaFPVSuperXNanoBoard board;

#ifndef LORA_CR
#define LORA_CR 5
#endif

  static SPIClass spi(FSPI);

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
  // rtc_clock.begin(Wire); // no i2c pins available
  // rtc_clock.begin();

#ifdef LR11X0_DIO3_TCXO_VOLTAGE
  float tcxo = LR11X0_DIO3_TCXO_VOLTAGE;
#else
  float tcxo = 0.0f;
#endif


// Manual SPI test
  Serial.println("=== Manual SPI Test ===");
  
  // Initialize SPI ourselves temporarily
  spi.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI, P_LORA_NSS);
  spi.setFrequency(2000000);  // Start slow
  
  // Deselect second radio
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  
  // Manual reset
  pinMode(P_LORA_RESET, OUTPUT);
  digitalWrite(P_LORA_RESET, LOW);
  delay(10);
  digitalWrite(P_LORA_RESET, HIGH);
  delay(300);
  
  // // Check BUSY
  // pinMode(P_LORA_BUSY, INPUT);
  // Serial.print("BUSY after reset: ");
  // Serial.println(digitalRead(P_LORA_BUSY));
  
  // // Try manual SPI transaction - LR1121 GetVersion command
  // pinMode(P_LORA_NSS, OUTPUT);
  // digitalWrite(P_LORA_NSS, LOW);
  // delayMicroseconds(10);
  
  // spi.transfer(0x01);  // GetVersion opcode byte 1
  // spi.transfer(0x01);  // GetVersion opcode byte 2
  // uint8_t hw = spi.transfer(0x00);
  // uint8_t dev = spi.transfer(0x00);
  // uint8_t maj = spi.transfer(0x00);
  // uint8_t min = spi.transfer(0x00);
  
  // digitalWrite(P_LORA_NSS, HIGH);
  
  // Serial.print("Manual SPI read: HW=0x");
  // Serial.print(hw, HEX);
  // Serial.print(" DEV=0x");
  // Serial.print(dev, HEX);
  // Serial.print(" FW=");
  // Serial.print(maj);
  // Serial.print(".");
  // Serial.println(min);

  // Setup both NSS pins
  // pinMode(P_LORA_NSS, OUTPUT);
  // digitalWrite(P_LORA_NSS, HIGH);
  // pinMode(7, OUTPUT);    // NSS_2
  // digitalWrite(7, HIGH);
  
  // // Setup BUSY and DIO1 as inputs
  // pinMode(P_LORA_BUSY, INPUT);
  // pinMode(P_LORA_DIO_1, INPUT);
  // pinMode(8, INPUT);   // BUSY_2
  // pinMode(18, INPUT);  // DIO1_2

  // delay(1000);
  
  // Begin SPI
  // spi.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI);
  // spi.setHwCs(true);
  // gpio_pullup_en((gpio_num_t)P_LORA_MISO);  // Critical!
  // spi.setFrequency(16000000);
  // spi.setDataMode(SPI_MODE0);
  // spi.setBitOrder(MSBFIRST);
  
  // // Reset BOTH radios together
  // pinMode(P_LORA_RESET, OUTPUT);
  // pinMode(10, OUTPUT);  // RESET_2
  // digitalWrite(P_LORA_RESET, LOW);
  // digitalWrite(10, LOW);
  // delay(1);
  // digitalWrite(P_LORA_RESET, HIGH);
  // digitalWrite(10, HIGH);
  // delay(300);  // LR1121 needs 300ms!
  
// Wait for BUSY to go low (with timeout)

// uint32_t start = millis();
// while (digitalRead(P_LORA_BUSY) == HIGH) {
//   if (millis() - start > 5000) {  // 5 second timeout
//     Serial.println("ERROR: BUSY timeout after reset");
//     return false;
//   }
//   Serial.println("Waiting for P_LORA_BUSY to go LOW...");
//   delay(250);  // Check every 250ms
// }

// SPI.begin();
delay(1300);


// Serial.println("Testing NSS toggle...");
// pinMode(P_LORA_NSS, OUTPUT);
// digitalWrite(P_LORA_NSS, HIGH);
// delay(10);
// Serial.print("NSS HIGH: ");
// Serial.println(digitalRead(P_LORA_NSS));
// digitalWrite(P_LORA_NSS, LOW);
// delay(10);
// Serial.print("NSS LOW: ");
// Serial.println(digitalRead(P_LORA_NSS));
// digitalWrite(P_LORA_NSS, HIGH);

  // Serial.println("BUSY signals ready, let's try radio.begin()!");

  // radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE, 10, 16, tcxo);

//   Serial.print("MISO state: ");
// Serial.println(digitalRead(P_LORA_MISO));

//   Serial.println("Testing SPI - reading chip version...");
// uint8_t hw, device, major, minor;
// int16_t state = radio.getVersion(&hw, &device, &major, &minor);

// Serial.print("getVersion returned: ");
// Serial.println(state);
// if (state == RADIOLIB_ERR_NONE) {
//   Serial.print("HW: 0x");
//   Serial.print(hw, HEX);
//   Serial.print(" Device: 0x");
//   Serial.print(device, HEX);
//   Serial.print(" FW: ");
//   Serial.print(major);
//   Serial.print(".");
//   Serial.println(minor);
// } else {
//   Serial.println("ERROR: Cannot read chip version - SPI not working!");
//   return false;
// }


  int status;

  float tcxo_candidates[3] = { tcxo, 1.8f, 3.3f };
  int tcxo_tries = (fabsf(tcxo) <= 0.001f) ? 3 : 1;
  float used_tcxo = tcxo;
  for (int i = 0; i < tcxo_tries; i++) {
    
    used_tcxo = tcxo_candidates[i];
    
    Serial.print("Attempting radio.begin using tcxo=");
    Serial.println(used_tcxo);
    status = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR,
                         RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE, LORA_TX_POWER, 16, used_tcxo);
    if (status == RADIOLIB_ERR_NONE) break;
  }
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
