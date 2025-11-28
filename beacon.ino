#include <Arduino.h>
#include "heltec.h"          // Heltec 官方库，用于 OLED
#include <WiFi.h>            // 用来读取 WiFi MAC
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>       // 显式包含（虽然只是广播）

const char* BEACON_NAME = "RG_BEACON";

// 板载 LED（用来确认程序在跑）
const int LED_PIN = LED_BUILTIN;

void setup() {
  // -------- 串口 --------
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("===== RG_BEACON Booting =====");

  // -------- Heltec 初始化（带 OLED）--------
  // 参数顺序：DisplayEnable, LoRaEnable, SerialEnable, PABOOST, BAND
  Heltec.begin(
    true,   // 打开 OLED 显示
    false,  // 不用 LoRa
    true,   // 打开 Serial
    false,  // 关闭 PA_BOOST
    470E6   // 频点随便给一个（LoRa 关着其实无所谓）
  );

#ifdef Vext
  // 给外设（包括 OLED）供电
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);     // 对大多数 Heltec：LOW = 打开 Vext
  delay(100);
#endif

  // -------- OLED 基本提示 --------
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 0, "RG_BEACON booting...");
  Heltec.display->drawString(0, 12, "Init WiFi & BLE...");
  Heltec.display->display();

  // -------- LED --------
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // -------- 先初始化 WiFi（为了读 WiFi MAC） --------
  WiFi.mode(WIFI_STA);       // 不连网，只是让 WiFi 模块起来
  String wifiMac = WiFi.macAddress();
  Serial.print("[WiFi] MAC: ");
  Serial.println(wifiMac);

  // -------- 初始化 BLE --------
  Serial.println("[BLE] init ...");
  BLEDevice::init(BEACON_NAME);      // 设置设备名（GAP 名）

  // 建一个空 server（即使不用，也保持标准用法）
  BLEServer *pServer = BLEDevice::createServer();

  // 读取 BLE MAC 地址
  BLEAddress bleAddr = BLEDevice::getAddress();
  String bleMac = String(bleAddr.toString().c_str());   // 注意：toString() 是 std::string，要转成 Arduino String

  Serial.print("[BLE] MAC: ");
  Serial.println(bleMac);

  // -------- 配置广播 --------
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

  // 广播数据里显式放名字，确保手机上能看到 RG_BEACON
  BLEAdvertisementData advData;
  advData.setName(BEACON_NAME);
  pAdvertising->setAdvertisementData(advData);

  pAdvertising->setScanResponse(true);   // 可以让扫描响应里多带一些信息
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);

  // 启动广播
  BLEDevice::startAdvertising();
  Serial.println("[BLE] Advertising started as RG_BEACON");

  // -------- OLED 上显示 MAC & 状态 --------
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);

  Heltec.display->drawString(0, 0,  "RG_BEACON ready");
  Heltec.display->drawString(0, 12, "BLE MAC:");
  Heltec.display->drawString(0, 24, bleMac);    // 显示 BLE MAC
  Heltec.display->drawString(0, 36, "Adv: ON");
  Heltec.display->display();

  Serial.println("===== Setup done =====");
}

void loop() {
  // 每秒闪一下 LED，说明程序在正常跑
  static unsigned long lastToggle = 0;
  static bool ledState = false;

  if (millis() - lastToggle > 1000) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastToggle = millis();

    // 每 1 秒在串口打一行，方便看有没有重启
    Serial.println("[Loop] still running...");
  }

  // BLE 广播由底层自动跑，这里不用写别的
  
}
