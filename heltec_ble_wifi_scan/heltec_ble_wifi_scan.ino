/**
 * Heltec ESP32: Alternating passive BLE & Wi-Fi scans every 30 seconds.
 *
 * - BLE uses NimBLE stack (built into ESP32 Arduino core) with passive window
 * - Wi-Fi uses passive scan via esp-idf parameters
 * - Each scan cycle reports the number of unique signals discovered
 *
 * NOTE: Requires ESP32 Arduino core 2.0.5+ for WiFi.scanNetworks passive args.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <esp_bt.h>

namespace {
constexpr uint32_t kScanIntervalMs = 30'000;
constexpr uint32_t kBleScanDurationSeconds = 10;   // Per pass scan window
constexpr uint32_t kWifiScanDurationMs = 120;      // Per-channel passive dwell

enum class ScanMode { kBLE, kWiFi };

BLEScan *g_bleScan = nullptr;
ScanMode g_nextMode = ScanMode::kBLE;
uint32_t g_lastScanStamp = 0;

bool g_bleReady = false;
bool g_btClassicReleased = false;

void ensureBleReady() {
  if (g_bleReady) {
    return;
  }
  if (!g_btClassicReleased) {
    // Release classic BT memory for additional RAM headroom (only once).
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    g_btClassicReleased = true;
  }
  BLEDevice::init("RoomGuard-Scanner");
  BLEDevice::setPower(ESP_PWR_LVL_P6);  // Moderate TX power

  g_bleScan = BLEDevice::getScan();
  g_bleScan->setActiveScan(false);  // Passive scan to reduce airtime
  g_bleScan->setInterval(100);      // 100 ms interval, 50% duty
  g_bleScan->setWindow(50);

  g_bleReady = true;
}

void ensureWifiSta() {
  if (WiFi.getMode() != WIFI_MODE_STA) {
    WiFi.mode(WIFI_STA);
  }
  WiFi.disconnect(true, true);
  esp_wifi_set_promiscuous(false);
}

void disableWifi() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();
}

uint16_t runBleScan() {
  ensureBleReady();
  Serial.println(F("[BLE] Passive scan started"));
  BLEScanResults *results = g_bleScan->start(kBleScanDurationSeconds, false);
  const uint16_t uniqueDevices = results ? results->getCount() : 0;
  Serial.printf("[BLE] Found %u unique advertisers\n", uniqueDevices);
  g_bleScan->clearResults();
  return uniqueDevices;
}

uint16_t runWifiScan() {
  ensureWifiSta();
  Serial.println(F("[WiFi] Passive scan started"));

  // Passive scan across full band (duration is per-channel dwell time).
  int16_t found = WiFi.scanNetworks(
      /*async=*/false,
      /*show_hidden=*/true,
      /*passive=*/true,
      /*max_ms_per_chan=*/kWifiScanDurationMs,
      /*channel=*/0,
      /*ssid=*/nullptr,
      /*bssid=*/nullptr);

  if (found < 0) {
    Serial.printf("[WiFi] Scan failed (%d)\n", found);
    return 0;
  }

  Serial.printf("[WiFi] Found %d unique access points\n", found);
  WiFi.scanDelete();
  disableWifi();
  return static_cast<uint16_t>(found);
}

void performScanCycle() {
  switch (g_nextMode) {
    case ScanMode::kBLE: {
      runBleScan();
      g_nextMode = ScanMode::kWiFi;
      break;
    }
    case ScanMode::kWiFi: {
      runWifiScan();
      g_nextMode = ScanMode::kBLE;
      break;
    }
  }
}
}  // namespace

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    delay(10);
  }
  Serial.println(F("\nRoomGuard Heltec ESP32 multi-radio scanner"));
  g_lastScanStamp = millis() - kScanIntervalMs;  // Trigger immediately
}

void loop() {
  const uint32_t now = millis();
  if (now - g_lastScanStamp >= kScanIntervalMs) {
    performScanCycle();
    g_lastScanStamp = now;
  }
  delay(50);
}

