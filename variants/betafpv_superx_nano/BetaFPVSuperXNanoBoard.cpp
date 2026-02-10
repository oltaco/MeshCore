#include <Arduino.h>
#include <BetaFPVSuperXNanoBoard.h>

void BetaFPVSuperXNanoBoard::begin() {
    ESP32Board::begin();

    

}


//   void BetaFPVSuperXNanoBoard::powerOff()  {
//     enterDeepSleep(0);
//   }

const char* BetaFPVSuperXNanoBoard::getManufacturerName() const {
    return "BetaFPV SuperX Nano";
  }

