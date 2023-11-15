#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID          "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define GREET   "Hello!"

BLECharacteristic *pCharacteristic =  NULL;

/*********************************************
 *    TFT display functions and variables
 *********************************************/

#include "Adafruit_ILI9341.h"
#include "Adafruit_GFX.h"

#define TFT_CS   5
#define TFT_DC   21

// Create an object representing the screen
Adafruit_ILI9341 TFT = Adafruit_ILI9341(TFT_CS, TFT_DC);

void DisplayInit(void) {
  TFT.begin();                      // Initialize screen
  TFT.setRotation(3);               // Set screen orientation
  
  TFT.fillScreen(ILI9341_BLUE);     // Screen background
  TFT.setTextSize(6);
  TFT.setTextColor(ILI9341_WHITE);

  TFT.setCursor(60, 108);
}

void PrintOnScreen(const char* value) {
  while(*value) {
    TFT.print(*value);
    value++;
  }
}

void ClearScreen(void) {
  TFT.fillScreen(ILI9341_BLUE);
  TFT.setCursor(60, 108);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  BLEDevice::init("VIDI-X Server");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );
  pCharacteristic->setValue(GREET); // Message to be received by clients
  pCharacteristic->addDescriptor(new BLE2902());
  
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  DisplayInit();
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(3000);
  std::string value = pCharacteristic->getValue();
  if (value != std::string(GREET)) {
    ClearScreen();
    delay(3000);
    PrintOnScreen(value.c_str());
  }
}
