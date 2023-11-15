#include "BLEDevice.h"

// Remote service we wish to connect to, created by VIDI X server
static BLEUUID REMOTE_SERVICE_UUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
// The characteristic of the remote service we are interested in
static BLEUUID REMOTE_CHARACTERISTIC_UUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

// Needs to be sent to VIDI X server, if we want to receive notifications from a desired characteristic
uint8_t activationCode[] = {0x1, 0x0};

static boolean doConnect = false;
static boolean connected = false;
static BLEAdvertisedDevice* myDevice;
BLERemoteCharacteristic* pRemoteCharacteristic;

// Callback function when notifications are received
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  ClearScreen();
  delay(3000);
  PrintOnScreen((char*) pData);
}

// Callack class which handles connections and disconnections from the server
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
  }
};

// Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  // Called for each advertising BLE server.
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(REMOTE_SERVICE_UUID)) {
      // Found VIDI X server
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

// Function to connect client with an advertising device
bool connectToServer() {
    BLEClient* pClient  = BLEDevice::createClient();

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to VIDI X Server.
    pClient->connect(myDevice);

    // Obtain a reference to the service we are after in the VIDI X server.
    BLERemoteService* pRemoteService = pClient->getService(REMOTE_SERVICE_UUID);
    if (pRemoteService == nullptr) {
      pClient->disconnect();
      return false;
    }

    // Obtain a reference to the characteristic in the service of the VIDI X server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(REMOTE_CHARACTERISTIC_UUID);
    if (pRemoteCharacteristic == nullptr) {
      pClient->disconnect();
      return false;
    }

    // Read the message from the VIDI X server.
    if(pRemoteCharacteristic->canRead()) {
      std::string str = pRemoteCharacteristic->readValue();
      const char* value = str.c_str();
      PrintOnScreen(value);
    }

    // Write a new message to the characteristic, to be read by VIDI X server
    if(pRemoteCharacteristic->canWrite()) {
      pRemoteCharacteristic->writeValue("Hello back!");
    }

    // Assign callback function to handle notifications (notifications inform clients when the characteristic' value has been changed)
    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

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
  TFT.setTextSize(3);
  TFT.setTextColor(ILI9341_WHITE);

  TFT.setCursor(120, 108);
}

void PrintOnScreen(const char* value) {
  while(*value) {
    TFT.print(*value);
    value++;
  }
}

void ClearScreen(void) {
  TFT.fillScreen(ILI9341_BLUE);
  TFT.setCursor(120, 108);
}

void setup() {
  // put your setup code here, to run once:
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(3000);
  pBLEScan->setWindow(600);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

  DisplayInit();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (doConnect == true) {
    if (connectToServer()) {
      pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)activationCode, 2, true); // Enable notifications
      doConnect = false;
    }
  }
  delay(1000); // Delay a second between loops.
}
