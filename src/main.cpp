
#include <M5Stack.h>
#include "BLEDevice.h"

uint8_t seq; // remember number of boots in RTC Memory
#define MyManufacturerId 0xffff  // test manufacturer ID
#define S_PERIOD     1  // Silent period

static BLEUUID service_uuid("");
static BLEUUID char_uuid("");

static BLEAddress* p_server_address;
static boolean do_connect = false;
static boolean connected = false;
static BLERemoteCharacteristic* p_remote_charastic;
BLEScan* p_ble_scan;
hw_timer_t * timer = NULL;
portMUX_TYPE timer_mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR on_timer(){
       // esp_deep_sleep(SLEEP_MSEC(1000));              // S_PERIOD秒Deep Sleepする
    p_ble_scan->stop();

}
void write_sd_card(float input){
   File file = SD.open("/log.csv",FILE_APPEND);
    file.printf("%f\n",input);
    file.close();
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertised_device) {
    if (!advertised_device.haveServiceUUID()) return;
    if (!advertised_device.getServiceUUID().equals(BLEUUID(service_uuid))) return;
    
    advertised_device.getScan()->stop();
    p_server_address = new BLEAddress(advertised_device.getAddress());
    do_connect = true;
  }
};

void setup() {
    M5.begin();

    M5.Lcd.setTextSize(3);

    BLEDevice::init("");  

    p_ble_scan = BLEDevice::getScan();
    p_ble_scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    p_ble_scan->setActiveScan(false);

    // timer = timerBegin(0,80,true);
    // timerAttachInterrupt(timer,&on_timer,true);
    // timerAlarmWrite(timer,100000, true);
    // timerAlarmEnable(timer);
    
}

void loop() {
    float temp, humid, press, vbat;

    BLEScanResults foundDevices = p_ble_scan->start(1);  // スキャンする
    int count = foundDevices.getCount();
    for (int i = 0; i < count; i++) {
        BLEAdvertisedDevice d = foundDevices.getDevice(i);
        if(!d.haveManufacturerData()){
            continue;
        }
    
        std::string data = d.getManufacturerData();
        int manu = data[1] << 8 | data[0];

        if (manu != MyManufacturerId || seq == data[2]) {  // カンパニーIDが0xFFFFで、
            continue;
        }             // シーケンス番号が新しいものを探す

        seq = data[2];
        temp = (float)(data[4] << 8 | data[3]) / 100.0;  // パケットから温度を取得
        humid = (float)(data[6] << 8 | data[5]) / 100.0;
        press = (float)(data[8] << 8 | data[7]) * 10.0 / 100.0;
        vbat = (float)(data[10] << 8 | data[9]) / 100.0;

        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(20,  20); M5.Lcd.printf("seq: %d\r\n", seq);
        M5.Lcd.setCursor(20,  60); M5.Lcd.printf("temp: %4.1f'C\r\n", temp);
        M5.Lcd.setCursor(20, 100); M5.Lcd.printf("humid:%4.1f%%\r\n", humid);
        M5.Lcd.setCursor(20, 140); M5.Lcd.printf("press:%4.0fhPa\r\n", press);
        M5.Lcd.setCursor(20, 180); M5.Lcd.printf("vbat: %4.2fV\r\n", vbat);
        write_sd_card(vbat);
        delay(1000);
    }
    esp_deep_sleep(SLEEP_MSEC(1000)); 
}
