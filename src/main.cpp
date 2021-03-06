
#include <M5Stack.h>
#include "BLEDevice.h"

uint8_t seq;                     // remember number of boots in RTC Memory
#define MyManufacturerId 0xffff  // test manufacturer ID
#define S_PERIOD 1               // Silent period

static BLEUUID service_uuid("");
static BLEUUID char_uuid("");

static BLEAddress* p_server_address;
static boolean do_connect = false;
static boolean connected = false;
static BLERemoteCharacteristic* p_remote_charastic;

BLEScan* p_ble_scan;
hw_timer_t* timer = NULL;
portMUX_TYPE timer_mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR on_timer() {
  // esp_deep_sleep(SLEEP_MSEC(1000));              // S_PERIOD秒Deep Sleepする
  p_ble_scan->stop();
}
void write_sd_card(time_t time) {
  File file = SD.open("/log.csv", FILE_APPEND);
  file.printf("%d\n", time);
  file.close();
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertised_device) {
    if (!advertised_device.haveServiceUUID()) return;
    if (!advertised_device.getServiceUUID().equals(BLEUUID(service_uuid)))
      return;

    advertised_device.getScan()->stop();
    p_server_address = new BLEAddress(advertised_device.getAddress());
    do_connect = true;
  }
};

void setup_ble(){
  M5.Lcd.setTextSize(3);

  BLEDevice::init("");

  p_ble_scan = BLEDevice::getScan();
  p_ble_scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  p_ble_scan->setActiveScan(false);
}
void task_ble(){
  BLEScanResults foundDevices = p_ble_scan->start(1);  // スキャンする
  int count = foundDevices.getCount();
  for (int i = 0; i < count; i++) {
    BLEAdvertisedDevice d = foundDevices.getDevice(i);
    if (!d.haveManufacturerData()) {
      continue;
    }

    std::string data = d.getManufacturerData();
    int manu = data[1] << 8 | data[0];

    if (manu != MyManufacturerId ||
        seq == data[2]) {  // カンパニーIDが0xFFFFで、
      continue;
    }  // シーケンス番号が新しいものを探す

    seq = data[2];
    time_t time =
        (time_t)(data[6] << 24 | data[5] << 16 | data[4] << 8 | data[3]);
    struct tm* now = localtime(&time);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("seq: %d\r\n", seq);
    M5.Lcd.printf("tm: %d:%d:%d'\r\n", now->tm_hour, now->tm_min, now->tm_sec);
    write_sd_card(time);
    // delay(1000);
  }
}
void setup() {
  setup_ble();
  M5.begin();
}


void loop() {
  task_ble();
  // esp_deep_sleep(SLEEP_MSEC(1000));
}
