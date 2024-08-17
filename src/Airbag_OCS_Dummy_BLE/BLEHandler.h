
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "e33691a0-54e0-4a45-9b9a-3804e0e77513"
#define CHARACTERISTIC_UUID "0da21dab-1091-4826-b8e3-183b5e82ca57"

enum BLECommands
{
  NO_OVERRIDE,
  NO_OCCUPIED_OVERRIDE,
  SEMI_OCCUPIED_OVERRIDE,
  OCCUPIED_OVERRIDE,
};

String OCSStatus[4] = {"NO_OVERRIDE", "NO_OCCUPIED_OVERRIDE", "SEMI_OCCUPIED_OVERRIDE", "OCCUPIED_OVERRIDE"};


BLEServer *pServer;


uint8_t overrideState = NO_OVERRIDE;


class BLEHandler : public BLECharacteristicCallbacks // Handler for any BLE input or output
{

  void onWrite(BLECharacteristic *pCharacteristic)
  {
    Serial.println("[BLE] - Written to characteristics");


    Serial.println(*(pCharacteristic->getData()));
    switch(*pCharacteristic->getData())
    {
      case NO_OVERRIDE:
      case NO_OCCUPIED_OVERRIDE:
      case SEMI_OCCUPIED_OVERRIDE:
      case OCCUPIED_OVERRIDE:
        overrideState = *pCharacteristic->getData();
        break;
      default:
        overrideState = NO_OVERRIDE;
        uint8_t value = 0;
        pCharacteristic->setValue(&value, 1);
        break;
    }

    Serial.println((String)"[BLE] - OCS status set to " + OCSStatus[*pCharacteristic->getData()]);
  }
  void onRead(BLECharacteristic *pCharacteristic)
  {
    Serial.println("[BLE] - Read from characteristics");
  }
};


class BLEServerHandler : public BLEServerCallbacks // Handler for client connect and disconnect
{
  void onConnect(BLEServer *pServer)
  {
    Serial.println("[BLE] - Client connected");

  }

  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("[BLE] - Client disconnected");

    // As the device has disconnected, advertise again
    pServer->startAdvertising();
  }
};


void setupBLE(void * parameter) {

  BLEDevice::init("350zOCS");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BLEServerHandler());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  uint8_t value = 0;
  pCharacteristic->setValue(&value, 1);
  pCharacteristic->setCallbacks(new BLEHandler());

  pService->start();  

  pServer->startAdvertising();

  Serial.println("[BLE] - Service started");

  vTaskDelete(NULL);
}
