#include <Arduino.h>
#include <BetaFPVSuperXNanoBoard.h>

void BetaFPVSuperXNanoBoard::begin() {
    ESP32Board::begin();
    Serial.end();

    Serial.begin(115200, SERIAL_8N1, SERIAL_RX, SERIAL_TX);

}


//   void BetaFPVSuperXNanoBoard::powerOff()  {
//     enterDeepSleep(0);
//   }

const char* BetaFPVSuperXNanoBoard::getManufacturerName() const {
    return "BetaFPV SuperX Nano";
  }

