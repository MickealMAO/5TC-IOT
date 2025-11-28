#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// BLE scan duration in seconds
int BLE_SCAN_TIME = 5;

// Pointer to the BLE scanner object
BLEScan *pBLEScan;

// Simple thresholds for crowd level (you can tune these later)
int THRESHOLD_CALM    = 5;   // totalSignals < 5  -> CALM
int THRESHOLD_CROWDED = 20;  // totalSignals >= 20 -> CROWDED

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("RoomGuard - Combined BLE + WiFi Scanner");

  // ---- WiFi initialization (scan only, no connection) ----
  WiFi.mode(WIFI_STA);        // Station mode only (no AP mode)
  WiFi.disconnect(true);      // Ensure not connected to any AP
  delay(100);

  // ---- BLE initialization ----
  BLEDevice::init("RoomGuard_Lighthouse");  // BLE device name (can be "" if you want)
  pBLEScan = BLEDevice::getScan();          // Create BLE scanner instance
  pBLEScan->setActiveScan(true);            // Active scan = more data, more power
  pBLEScan->setInterval(100);               // Scan interval in ms
  pBLEScan->setWindow(80);                  // Scan window in ms (must be <= interval)

  Serial.println("Setup done, starting scan loop...");
}

void loop() {
  Serial.println("======================================");
  Serial.println("New scan window started");

  // ---------- 1. BLE scan ----------
  Serial.println("[BLE] Scanning...");
  // start(scanTime, is_continue): is_continue = false means blocking scan
  BLEScanResults *bleResults = pBLEScan->start(BLE_SCAN_TIME, false);

  // getCount() returns the number of BLEAdvertisedDevice objects,
  // i.e., the number of unique BLE devices detected in this scan window.
  int bleCount = bleResults->getCount();

  Serial.print("[BLE] Unique BLE devices found = ");
  Serial.println(bleCount);

  // Show up to first 5 BLE device MAC addresses (for debugging / analysis)
  int maxBleToShow = (bleCount < 5) ? bleCount : 5;
  Serial.println("[BLE] Top BLE devices (MAC, RSSI):");
  for (int i = 0; i < maxBleToShow; i++) {
    BLEAdvertisedDevice dev = bleResults->getDevice(i);
    String addr = dev.getAddress().toString().c_str();
    int rssi = dev.getRSSI();

    Serial.print("  #");
    Serial.print(i + 1);
    Serial.print(" -> ");
    Serial.print(addr);
    Serial.print("  RSSI=");
    Serial.println(rssi);
  }

  // Clear BLE results to free memory
  pBLEScan->clearResults();

  // ---------- 2. WiFi scan ----------
  Serial.println("[WiFi] Scanning...");
  // WiFi.scanNetworks() returns the number of WiFi access points found
  int wifiCount = WiFi.scanNetworks();

  if (wifiCount == 0) {
    Serial.println("[WiFi] No networks found");
  } else {
    Serial.print("[WiFi] Networks found: ");
    Serial.println(wifiCount);

    Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
    for (int i = 0; i < wifiCount; ++i) {
      // Print WiFi AP information
      Serial.printf("%2d", i + 1);
      Serial.print(" | ");
      Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
      Serial.print(" | ");
      Serial.printf("%4ld", WiFi.RSSI(i));
      Serial.print(" | ");
      Serial.printf("%2ld", WiFi.channel(i));
      Serial.print(" | ");
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN:            Serial.print("open"); break;
        case WIFI_AUTH_WEP:             Serial.print("WEP"); break;
        case WIFI_AUTH_WPA_PSK:         Serial.print("WPA"); break;
        case WIFI_AUTH_WPA2_PSK:        Serial.print("WPA2"); break;
        case WIFI_AUTH_WPA_WPA2_PSK:    Serial.print("WPA+WPA2"); break;
        case WIFI_AUTH_WPA2_ENTERPRISE: Serial.print("WPA2-EAP"); break;
        case WIFI_AUTH_WPA3_PSK:        Serial.print("WPA3"); break;
        case WIFI_AUTH_WPA2_WPA3_PSK:   Serial.print("WPA2+WPA3"); break;
        case WIFI_AUTH_WAPI_PSK:        Serial.print("WAPI"); break;
        default:                        Serial.print("unknown");
      }
      Serial.println();
      delay(5);   // Small delay to keep serial output readable
    }
  }
  Serial.println();

  // Delete WiFi scan results to free memory
  WiFi.scanDelete();

  // ---------- 3. Compute simple crowd level ----------
  int totalSignals = bleCount + wifiCount;

  Serial.print("[SUMMARY] BLE unique count  = ");
  Serial.println(bleCount);
  Serial.print("[SUMMARY] WiFi count        = ");
  Serial.println(wifiCount);
  Serial.print("[SUMMARY] Total signals     = ");
  Serial.println(totalSignals);

  // Map totalSignals to a crowd level
  String crowdText;
  int crowdLevel = 0;  // 0 = CALM, 1 = MODERATE, 2 = CROWDED

  if (totalSignals < THRESHOLD_CALM) {
    crowdLevel = 0;
    crowdText = "CALM";
  } else if (totalSignals < THRESHOLD_CROWDED) {
    crowdLevel = 1;
    crowdText = "MODERATE";
  } else {
    crowdLevel = 2;
    crowdText = "CROWDED";
  }

  Serial.print("[SUMMARY] Crowd level = ");
  Serial.print(crowdLevel);
  Serial.print(" (");
  Serial.print(crowdText);
  Serial.println(")");
  Serial.println("======================================\n");

  // Wait some time before next combined scan window
  delay(5000);
}
