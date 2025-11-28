#include <WiFi.h>          // Wi-Fi 扫描
#include <BLEDevice.h>     // BLE 扫描
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// 扫描时间（秒）——之后你可以改成 30 / 60 秒
const int BLE_SCAN_TIME = 10;

// 为了去重，最多记录多少个设备
const int MAX_DEVICES = 100;

// 存放已发现的 BLE / WiFi 设备地址，用于去重
String bleAddrs[MAX_DEVICES];
String wifiBssids[MAX_DEVICES];

// 简单的“去重”检查函数
bool isInList(String arr[], int size, const String &item) {
  for (int i = 0; i < size; i++) {
    if (arr[i] == item) return true;
  }
  return false;
}

BLEScan *pBLEScan;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println();
  Serial.println("RoomGuard - BLE & WiFi Scanner");

  // ---------- Wi-Fi 只用于扫描，不连接 ----------
  WiFi.mode(WIFI_STA);   // Station 模式
  WiFi.disconnect(true); // 不连接任何 AP，仅扫描
  delay(100);

  // ---------- 初始化 BLE ----------
  BLEDevice::init("RoomGuard_Lighthouse");  // 设备名随便写
  pBLEScan = BLEDevice::getScan();          // 创建扫描对象
  pBLEScan->setActiveScan(true);            // active scan，拿到更多信息
  pBLEScan->setInterval(100);               // 扫描间隔（ms）
  pBLEScan->setWindow(80);                  // 每次扫描窗口（ms），<= interval

  Serial.println("Init done. Start scanning...");
}

void loop() {
  // ===== 1. 扫描 BLE 设备 =====
  Serial.println("----- BLE SCAN START -----");

  // 清空 BLE 去重列表
  int bleCountUnique = 0;

  BLEScanResults foundDevices = pBLEScan->start(BLE_SCAN_TIME, false);
  int rawCount = foundDevices.getCount();  // 原始发现数量（未去重）

  for (int i = 0; i < rawCount && bleCountUnique < MAX_DEVICES; i++) {
    BLEAdvertisedDevice dev = foundDevices.getDevice(i);
    String addr = dev.getAddress().toString().c_str();  // MAC 地址
    int rssi = dev.getRSSI();

    if (!isInList(bleAddrs, bleCountUnique, addr)) {
      bleAddrs[bleCountUnique] = addr;
      bleCountUnique++;
      Serial.print("BLE Device ");
      Serial.print(bleCountUnique);
      Serial.print(" : ");
      Serial.print(addr);
      Serial.print("  RSSI=");
      Serial.println(rssi);
    }
  }

  Serial.print("BLE devices (raw count)     = ");
  Serial.println(rawCount);
  Serial.print("BLE devices (unique count)  = ");
  Serial.println(bleCountUnique);
  Serial.println("----- BLE SCAN END -----");
  Serial.println();

  // ===== 2. 扫描 Wi-Fi AP =====
  Serial.println("----- WiFi SCAN START -----");

  // 清空 WiFi 去重列表
  int wifiCountUnique = 0;

  // 第一个参数：异步/同步，这里用同步扫描
  // 第二个参数：是否扫描隐藏网络
  int n = WiFi.scanNetworks(false, true);
  Serial.print("WiFi networks (raw count)   = ");
  Serial.println(n);

  for (int i = 0; i < n && wifiCountUnique < MAX_DEVICES; i++) {
    String ssid  = WiFi.SSID(i);      // 网络名
    String bssid = WiFi.BSSIDstr(i);  // AP 的 MAC 地址
    int rssi     = WiFi.RSSI(i);

    if (!isInList(wifiBssids, wifiCountUnique, bssid)) {
      wifiBssids[wifiCountUnique] = bssid;
      wifiCountUnique++;

      Serial.print("WiFi AP ");
      Serial.print(wifiCountUnique);
      Serial.print(" : SSID=");
      Serial.print(ssid);
      Serial.print("  BSSID=");
      Serial.print(bssid);
      Serial.print("  RSSI=");
      Serial.println(rssi);
    }
  }

  WiFi.scanDelete(); // 清除扫描结果，释放内存

  Serial.print("WiFi AP (unique BSSID)      = ");
  Serial.println(wifiCountUnique);
  Serial.println("----- WiFi SCAN END -----");
  Serial.println();

  // ===== 3. 等待再来一次 =====
  Serial.println("Sleep 10s then rescan...\n");
  delay(10000);  // 10 秒后重新扫描（之后你可以改）
}
