#pragma once

#include <RadioLib.h>
#include "MeshCore.h"


class CustomLR2021 : public LR2021 {
  bool _rx_boosted = false;

  public:
    CustomLR2021(Module *mod) : LR2021(mod) { irqDioNum = LR2021_IRQ_DIO; }

    bool std_init(SPIClass* spi = NULL)
    {
      
  #ifdef LR2021_TCXO_VOLTAGE
      float tcxo = LR2021_TCXO_VOLTAGE;
  #else
      float tcxo = 1.6f;
  #endif

  #ifdef LORA_CR
      uint8_t cr = LORA_CR;
  #else
      uint8_t cr = 5;
  #endif

  #if defined(P_LORA_SCLK)
    #ifdef NRF52_PLATFORM
      if (spi) { spi->setPins(P_LORA_MISO, P_LORA_SCLK, P_LORA_MOSI); spi->begin(); }
    #elif defined(RP2040_PLATFORM)
      if (spi) {
        spi->setMISO(P_LORA_MISO);
        //spi->setCS(P_LORA_NSS); // Setting CS results in freeze
        spi->setSCK(P_LORA_SCLK);
        spi->setMOSI(P_LORA_MOSI);
        spi->begin();
      }
    #else
      if (spi) spi->begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI);
    #endif
  #endif
      int status = begin(LORA_FREQ, LORA_BW, LORA_SF, cr, RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE, LORA_TX_POWER, 16, tcxo);
      // if radio init fails with -707/-706, try again with tcxo voltage set to 0.0f
      if (status == RADIOLIB_ERR_SPI_CMD_FAILED || status == RADIOLIB_ERR_SPI_CMD_INVALID) {
        tcxo = 0.0f;
        status = begin(LORA_FREQ, LORA_BW, LORA_SF, cr, RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE, LORA_TX_POWER, 16, tcxo);
      }
      if (status != RADIOLIB_ERR_NONE) {
        Serial.print("ERROR: radio init failed: ");
        Serial.println(status);
        return false;  // fail
      }
    
      setCRC(2);
      explicitHeader();

      
    #ifdef LR2021_RX_BOOSTED_GAIN
      // setRxBoostedGainMode(); TODO: LR2021 takes an int for boosted gain, not a bool like sx1262
    #endif

      return true;  // success
    }
    
    float getFreqMHz() const { return freqMHz; }

    // TODO: LR2021 has levels of boosted gain?
    // see https://github.com/jgromes/RadioLib/blob/217f8cf4bd8f7bc803ba5e9f6db0235fa37f0f9b/src/modules/LR2021/LR2021.h#L672
    // int16_t setRxBoostedGainMode(bool en) {
    //   _rx_boosted = en;
    //   return LR2021::setRxBoostedGainMode(en);
    // }
    //
    // bool getRxBoostedGainMode() const { return _rx_boosted; }

    bool isReceiving() {
      uint32_t irq = getIrqStatus();
      bool detected = ((irq & RADIOLIB_LR2021_IRQ_SYNCWORD_VALID) || (irq & RADIOLIB_LR2021_IRQ_PREAMBLE_DETECTED));
      return detected;
    }

    uint8_t getSpreadingFactor() const { return spreadingFactor; }
};