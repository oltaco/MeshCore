#pragma once

#include "CustomLR2021.h"
#include "RadioLibWrappers.h"

#ifndef USE_LR2021
#define USE_LR2021
#endif

#ifndef LR2021_RX_BOOST_LEVEL
#define LR2021_RX_BOOST_LEVEL 7
#endif

class CustomLR2021Wrapper : public RadioLibWrapper {
public:
  CustomLR2021Wrapper(CustomLR2021& radio, mesh::MainBoard& board) : RadioLibWrapper(radio, board) { }

  void setParams(float freq, float bw, uint8_t sf, uint8_t cr) override {
    ((CustomLR2021 *)_radio)->setFrequency(freq);
    ((CustomLR2021 *)_radio)->setSpreadingFactor(sf);
    ((CustomLR2021 *)_radio)->setBandwidth(bw);
    ((CustomLR2021 *)_radio)->setCodingRate(cr);
    updatePreamble(sf);
  }

  bool setSideDetectors(const uint8_t* sideDetSFs, uint8_t num) override {
    LR2021LoRaSideDetector_t tmp[3];
    uint8_t n;
    uint8_t primarySf = getSpreadingFactor();
    
    for (int i = 0; i < num; i++) {
      tmp[i].sf = sideDetSFs[i];
      tmp[i].ldro = false; // TODO: automatically set ldro true when tSym >=16
      tmp[i].invertIQ = false;
      tmp[i].syncWord = 0x12;
    }
    int16_t status = ((CustomLR2021 *)_radio)->setSideDetector(tmp, num);
    
    MESH_DEBUG_PRINTLN("setSideDetectors() returned %d", status);
    return true == RADIOLIB_ERR_NONE;
  }

  bool isReceivingPacket() override {
    return ((CustomLR2021 *)_radio)->isReceiving();
  }

  float getCurrentRSSI() override {
    float rssi = -110;
    ((CustomLR2021 *)_radio)->getRssiInst(&rssi);
    return rssi;
  }

  void onSendFinished() override {
    RadioLibWrapper::onSendFinished();
    _radio->setPreambleLength(preambleLengthForSF(getSpreadingFactor())); // overcomes weird issues with small and big pkts
  }

  float getLastRSSI() const override { return ((CustomLR2021 *)_radio)->getRSSI(); }
  float getLastSNR() const override { return ((CustomLR2021 *)_radio)->getSNR(); }

  uint8_t getSpreadingFactor() const override { return ((CustomLR2021 *)_radio)->getSpreadingFactor(); }
  
  bool setRxBoostedGainMode(bool en) override {
    ((CustomLR2021 *)_radio)->standby(); // radio must be in standby to accept the setRxBoostedGainMode command, otherwise it returns -707 error.
    int16_t status = ((CustomLR2021 *)_radio)->setRxBoostedGainMode(en ? LR2021_RX_BOOST_LEVEL: 0);
    RadioLibWrapper::idle(); // trigger startReceive()
    return status == RADIOLIB_ERR_NONE;
  }

  bool getRxBoostedGainMode() const override {
    return ((CustomLR2021 *)_radio)->getRxBoostedGainMode();
  }

  protected:
    LR2021LoRaSideDetector_t _sideDet[3];
    size_t _numSideDet = 0;


};
