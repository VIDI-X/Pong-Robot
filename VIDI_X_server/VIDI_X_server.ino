/*********************************************
 *    BLE functions and variables
 *********************************************/
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID              "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define MOTOR_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BALL_CHARACTERISTIC_UUID  "88ae53aa-72ad-44e4-ac95-8e5f7a4a0bd2"

BLEServer *pServer = NULL;
BLECharacteristic *pMotorCharacteristic = NULL;
BLECharacteristic *pBallCharacteristic = NULL;

bool RPiConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
  public:
  
    uint16_t id;
    
    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
      RPiConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      RPiConnected = false;

      pServer->startAdvertising(); // Must start advertising again for new connections (not intuitive)
    }
};

void BLEInit(void) {
  BLEDevice::init("ESP32 server");

  // Start server and set up callback for new connections
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create service and characteristics
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  pMotorCharacteristic = pService->createCharacteristic(MOTOR_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);  
  pMotorCharacteristic->addDescriptor(new BLE2902());

  pBallCharacteristic = pService->createCharacteristic(BALL_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  pBallCharacteristic->addDescriptor(new BLE2902());

  // Start service
  pService->start();

  // Setup advertising to other devices
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  
  BLEDevice::startAdvertising();
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_ADV);
}


/*********************************************
 *    VIDI X button structure and functions
 *********************************************/

struct Button {
  const uint8_t pin;

  uint16_t current_state;
  uint16_t last_state;
  
  uint16_t last_button_time;

  bool changed;
};

Button BTN_A = {32, HIGH, HIGH, 0, false};
Button BTN_LR = {34, 0, 0, 0, false};

void IRAM_ATTR BTN_A_ISR(){
  if (BTN_A.changed == false)
  {
    BTN_A.last_button_time = millis();
    BTN_A.changed = true;
  }
}

void check_BTN_A(){
  // Wait for debounce interval to expire
  if (millis() - BTN_A.last_button_time > 60)
  {
    // Take current state and compare to last state
    BTN_A.current_state = digitalRead(BTN_A.pin);
    if (BTN_A.current_state != BTN_A.last_state)
    {
      BTN_A.last_state = BTN_A.current_state;

      // Send to Pi
      pBallCharacteristic->setValue(BTN_A.last_state);
      pBallCharacteristic->notify();
    }
    BTN_A.changed = false;
  }
}

void check_BTN_LR(){
  uint16_t state = analogRead(BTN_LR.pin);
  
  if (state > 4000){
    BTN_LR.current_state = 1;
  } else if (state < 10) {
    BTN_LR.current_state = 0;
  } else if (state > 1500 && state < 2500){
    BTN_LR.current_state = 2;
  }

  // Check if the state has changed
  if (BTN_LR.last_state != BTN_LR.current_state){
    BTN_LR.last_state = BTN_LR.current_state;

    // Send to Pi
    pMotorCharacteristic->setValue(BTN_LR.last_state);
    pMotorCharacteristic->notify();
  }
}

/*********************************************
 *    Main setup and program loop
 *********************************************/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Button setup code
  pinMode(BTN_A.pin, INPUT_PULLUP);
  pinMode(BTN_LR.pin, INPUT_PULLUP);

  // IRQ initialisation
  attachInterrupt(BTN_A.pin, BTN_A_ISR, CHANGE);

  // Custom function to set up Bluetooth
  BLEInit();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (RPiConnected)
  {
    check_BTN_LR();
    if (BTN_A.changed) {
      check_BTN_A();
    }
  }
}
