
#include <M5Stack.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <deque>
#include <string>


#define DEVICE_NAME         "M5Stack"
#define SERVICE_UUID        "5147b804-4b5b-429d-b6d2-0f4b8187a4ea"
#define CHARACTERISTIC_UUID "a851d6b3-6720-41e7-a9d4-81dcec2fd861"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
int start_advertise_timer = 0;
int buttons = 0;

std::vector<char> ble_input;
std::deque<std::string> requests;

enum {
  S_WAITING,
  S_READY,
} status = S_WAITING;

void printStatus(char const *text)
{
  M5.Lcd.fillRect(0, 0, 320, 40, M5.Lcd.color565(64, 64, 80));
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setCursor(4, 4);
  M5.Lcd.printf(text);
}

void printWatingStatus()
{
  printStatus("Waiting...");  
}

void printConnectedStatus()
{
  printStatus("OK, Connected");  
}

void printMessage(char const *text)
{
  const int y = 40;
  M5.Lcd.fillRect(0, y, 320, 150, M5.Lcd.color565(0, 0, 0));
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setCursor(4, 4 + y);
  M5.Lcd.printf(text);
}

void drawButtons()
{
  for (int i = 0; i < 3; i++) {
    int w = 320 / 3;
    int x = w * i;
    int h = 48;
    int y = 240 - h - 2;
    int color = ((buttons >> i) & 1) ? M5.Lcd.color565(0, 255, 0) : M5.Lcd.color565(64, 64, 64);
    M5.Lcd.drawRect(x, y, w, h, WHITE);
    M5.Lcd.fillRect(x + 2, y + 2, w - 4, h - 4, color);
  }
}

void startAdvertise()
{
  status = S_WAITING;
  printWatingStatus();
  start_advertise_timer = 5;  
}

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };
  
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
    for (int i = 0; i < value.length(); i++) {
      char c = value[i];
      if (c == 0) {
        if (!ble_input.empty()) {
          std::string s(ble_input.data(), ble_input.size());
          requests.push_back(s);
        }
        ble_input.clear();
        break;
      }
      ble_input.push_back(c);
    }
  }
};

void setup()
{
  setCpuFrequencyMhz(80);
  M5.begin();
  M5.Lcd.fillScreen(BLACK);
  printStatus("Starting...");
  drawButtons();

  status = S_WAITING;

  // Create the BLE Device
  BLEDevice::init(DEVICE_NAME);
  
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ   |
        BLECharacteristic::PROPERTY_WRITE  |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
        );
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  
  // Start the service
  pService->start();
  
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  startAdvertise();
}

void loop()
{
  if (deviceConnected) {
    if (!oldDeviceConnected) {
      oldDeviceConnected = true;
      printConnectedStatus();
    }
    M5.update();
    uint8_t value = 0;
    if (M5.BtnA.isPressed()) value |= 1;
    if (M5.BtnB.isPressed()) value |= 2;
    if (M5.BtnC.isPressed()) value |= 4;
    if (buttons != value) {
      buttons = value;
      pCharacteristic->setValue((uint8_t *)&value, 1);
      pCharacteristic->notify();
      drawButtons();
    }
  } else {
    if (oldDeviceConnected) {
      oldDeviceConnected = false;
      startAdvertise();
    } else if (start_advertise_timer > 0) {
      start_advertise_timer--;
      if (start_advertise_timer == 0) {
        pServer->startAdvertising(); // restart advertising
      }
    }
  }

  while (!requests.empty()) {
    std::string s = requests.front();
    requests.pop_front();
    printMessage(s.c_str());
  }

}
