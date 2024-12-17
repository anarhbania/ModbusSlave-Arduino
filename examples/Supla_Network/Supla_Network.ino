#include <SuplaDevice.h>
#include <supla/control/button.h>
#include <supla/device/status_led.h>
#include <supla/device/supla_ca_cert.h>
#include <supla/network/esp_web_server.h>
#include <supla/network/esp_wifi.h>
#include <supla/network/html/device_info.h>
#include <supla/network/html/protocol_parameters.h>
#include <supla/network/html/wifi_parameters.h>
#include <supla/storage/eeprom.h>
#include <supla/storage/littlefs_config.h>

#include <ModbusSlave.h>

Supla::Device::StatusLed statusLed(13, true);
Supla::Eeprom eeprom;
Supla::ESPWifi wifi;
Supla::EspWebServer suplaServer;
Supla::LittleFsConfig configSupla;

#define SLAVE_BAUD       9600
#define SLAVE_ID            1
#define SLAVE_ADDRESS_START 0
#define SLAVE_ADDRESS_SIZE  1

#define NETWORK_READY       0

uint16_t slaveTable[SLAVE_ADDRESS_SIZE];

ModbusSlave Slave(&Serial, SLAVE_BAUD, SLAVE_ID, SLAVE_ADDRESS_START, slaveTable, SLAVE_ADDRESS_SIZE, 1000);

void setup() 
{
  new Supla::Html::DeviceInfo(&SuplaDevice);
  new Supla::Html::WifiParameters;
  new Supla::Html::ProtocolParameters;

  auto suplaButtonCfg = new Supla::Control::Button(0, true, true);
  suplaButtonCfg->configureAsConfigButton(&SuplaDevice);

  SuplaDevice.setSuplaCACert(suplaCACert);
  SuplaDevice.setSupla3rdPartyCACert(supla3rdCACert);

  SuplaDevice.begin();
}

void loop()
{
  SuplaDevice.iterate();

  Slave.Update();
  
  static uint32_t lastTime = 0;
  if(millis() - lastTime > 1000) 
  {
    lastTime = millis();

    slaveTable[NETWORK_READY] = Supla::Network::IsReady();
  }
}