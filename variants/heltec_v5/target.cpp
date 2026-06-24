#include <Arduino.h>
#include "target.h"

HeltecV5Board board;

#if defined(P_LORA_SCLK)
  static SPIClass spi;
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);

#if ENV_INCLUDE_GPS
  #include <helpers/sensors/MicroNMEALocationProvider.h>
  MicroNMEALocationProvider nmea = MicroNMEALocationProvider(Serial1, &rtc_clock);
  EnvironmentSensorManager sensors = EnvironmentSensorManager(nmea);
#else
  EnvironmentSensorManager sensors;
#endif

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display(NULL);
  MomentaryButton user_btn(PIN_USER_BTN, 1000, true);
#endif

// leaving this for when RadioLib bug is fixed.
// https://wiki.meshnology.com/assets/files/W12-MB-V0.2(1)-55ab22db55cdecf178c62a595fa66bea.pdf
const uint32_t rfswitch_dios[] = {
  RADIOLIB_LR2021_DIO5,   // RFX2402E 2.4G_TX_EN
  RADIOLIB_LR2021_DIO6,   // RFX2402E 2.4G_RX_EN
  RADIOLIB_LR2021_DIO9,   // GC1109 CTX (Transmit mode)
  RADIOLIB_LR2021_DIO10,  // GC1109 CPS (Bypass mode)
  RADIOLIB_LR2021_DIO11,  // GC1109 CSD (Shutdown)
};

// GC1109 datasheet https://www.geochipinc.com/uploads/23940615_1756826539.pdf
static const Module::RfSwitchMode_t rfswitch_table[] = {
  //                    DIO5 DIO6  DIO9   DIO10   DIO11
  { LR2021::MODE_STBY,  { LOW,  LOW,  LOW,  LOW,  LOW  } },  // everything off
  { LR2021::MODE_RX,    { LOW,  LOW,  LOW,  LOW,  HIGH } },  // rx_lf: GC1109 LNA
  { LR2021::MODE_TX,    { LOW,  LOW,  HIGH, HIGH, HIGH } },  // tx_lf: GC1109 full PA
  { LR2021::MODE_RX_HF, { LOW,  HIGH, LOW,  LOW,  LOW  } },  // rx_hf: 2G4 LNA, GC1109 off
  { LR2021::MODE_TX_HF, { HIGH, LOW,  LOW,  LOW,  LOW  } },  // tx_hf: 2G4 PA,  GC1109 off
  END_OF_MODE_TABLE,
};


bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);

  pinMode(P_LORA_LF_PA_POWER, OUTPUT);
  digitalWrite(P_LORA_LF_PA_POWER, HIGH);   // PA_EN_M — power the GC1109 FEM

  delay(100); // give the GC1109 some time to start

#if defined(P_LORA_SCLK)
  int err = radio.std_init(&spi);
  if (err != 1) return err;
#else
  int err = radio.std_init();
  if (err != 1) return err;
#endif

// for now this sets the LR2021 DIO pins to be used for switching the LF LORA FEM states
// using setRfSwitchTable would be better but we have to wait for RadioLib PR
// CTX (DIO9)
int f9 = radio.setDioFunction(9, RADIOLIB_LR2021_DIO_FUNCTION_RF_SWITCH, RADIOLIB_LR2021_DIO_SLEEP_PULL_AUTO);
MESH_DEBUG_PRINTLN("fn9=%d cfg9=%d", f9, radio.setDioRfSwitchConfig(9, 0x04));
// CPS (DIO10)
int f10 = radio.setDioFunction(10, RADIOLIB_LR2021_DIO_FUNCTION_RF_SWITCH, RADIOLIB_LR2021_DIO_SLEEP_PULL_AUTO);
MESH_DEBUG_PRINTLN("fn10=%d cfg10=%d", f10, radio.setDioRfSwitchConfig(10, 0x04));
// CSD (DIO11)
int f11 = radio.setDioFunction(11, RADIOLIB_LR2021_DIO_FUNCTION_RF_SWITCH, RADIOLIB_LR2021_DIO_SLEEP_PULL_AUTO);
MESH_DEBUG_PRINTLN("fn11=%d cfg11=%d", f11, radio.setDioRfSwitchConfig(11, 0x06));

// this attempts to make up for the lack of tcxo (almost works)
// radio.setXoscCpTrim(47, 47, 150);

// commented out until RadioLib can address non-contiguous DIO pins in setRfSwitchTable
// #ifdef RF_SWITCH_TABLE 
//   radio.setRfSwitchTable(rfswitch_dios, rfswitch_table);
// #endif

 return true;
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}

