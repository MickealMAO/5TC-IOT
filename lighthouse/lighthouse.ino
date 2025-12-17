#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "LoRaWan_APP.h"

/* ABP para*/
uint8_t devEui[] = { 0x14, 0x19, 0x0b, 0x1d, 0x14, 0x19, 0x0b, 0x1d};
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t appKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appSKey[] = {0xAE,0x8F,0x48,0xA0,0x5F,0xA1,0x91,0xCF,0xAB,0x5F,0xCD,0xE4,0xB2,0xE7,0x16,0x3F};
uint8_t nwkSKey[] = {0x98,0x0C,0x7F,0xB8,0x2B,0xAD,0x47,0xFE,0x5D,0x53,0xF2,0xF0,0x66,0x30,0x72,0x9C};
uint32_t devAddr = (uint32_t)0x260B818D;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

/*OTAA or ABP*/
bool overTheAirActivation = false;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = false;

/* Application port */
uint8_t appPort = 2;

uint8_t confirmedNbTrials = 4;


// BLE scan duration in seconds
int BLE_SCAN_TIME = 5;

// Target Beacon Name to track
const char* TARGET_BEACON_NAME = "RG_BEACON";

// Pointer to the BLE scanner object
BLEScan *pBLEScan;

// Simple thresholds for crowd level
int THRESHOLD_CALM    = 20;   // BLEcount < 20  -> CALM
int THRESHOLD_CROWDED = 80;  // BLEcount >= 80 -> CROWDED


// Global variables to store latest counts
uint16_t latestWifiCount = 0;
uint16_t latestBleCount = 0;

// Global variable to store previous WiFi SSIDs for comparison
#include <vector>
std::vector<String> lastSsids;

// Function to prepare and send LoRaWAN payload
// Payload format (6 bytes total):
// Byte 0-1: WiFi count (uint16_t, big-endian)
// Byte 2-3: BLE count (uint16_t, big-endian)
// Byte 4:   Beacon RSSI (uint8_t: 0=Not Found, >0 = abs(RSSI))
// Byte 5:   Environment State (uint8_t: 0=Static, 1=Mobile)
void sendLoRaWANData(uint16_t wifiCount, uint16_t bleCount, int beaconRssi, bool isMobile, uint8_t port) {
    
    // Queue the transmission
    appDataSize = 6;
    appData[0] = (wifiCount >> 8) & 0xFF;
    appData[1] = wifiCount & 0xFF;
    appData[2] = (bleCount >> 8) & 0xFF;
    appData[3] = bleCount & 0xFF;
    
    // Encode Beacon RSSI
    if (beaconRssi == 0) {
        appData[4] = 0;
    } else {
        appData[4] = (uint8_t)abs(beaconRssi);
    }

    // Encode Environment State
    appData[5] = isMobile ? 1 : 0;
    
    Serial.println(F("[LoRaWAN] Preparing to send:"));
    Serial.print(F("  WiFi count: "));
    Serial.println(wifiCount);
    Serial.print(F("  BLE count: "));
    Serial.println(bleCount);
    Serial.print(F("  Beacon RSSI: "));
    if (beaconRssi == 0) {
      Serial.println("Not Found");
    } else {
      Serial.println(beaconRssi);
    }
    Serial.print(F("  Environment: "));
    Serial.println(isMobile ? "MOBILE" : "STATIC");
}

void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
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

  {
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
#if(LORAWAN_DEVEUI_AUTO)
      LoRaWAN.generateDeveuiByChipID();
#endif
      LoRaWAN.init(loraWanClass,loraWanRegion);
      //both set join DR and DR when ADR off 
      LoRaWAN.setDefaultDR(3);
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
        Serial.println("======================================");
  Serial.println("New scan window started");

  // ---------- 1. BLE scan ----------
  Serial.println("[BLE] Scanning...");
  // start(scanTime, is_continue): is_continue = false means blocking scan
  BLEScanResults *bleResults = pBLEScan->start(BLE_SCAN_TIME, false);

  // Initialize beacon tracking variable
  int targetRssi = 0; // 0 means not found

  // getCount() returns the number of BLEAdvertisedDevice objects,
  // i.e., the number of unique BLE devices detected in this scan window.
  int bleCount = bleResults->getCount();

  Serial.print("[BLE] Unique BLE devices found = ");
  Serial.println(bleCount);

  // Show up to first 5 BLE device MAC addresses (for debugging / analysis)
  int maxBleToShow = (bleCount < 5) ? bleCount : 5;
  Serial.println("[BLE] Top BLE devices (MAC, RSSI):");
  for (int i = 0; i < bleCount; i++) {
    BLEAdvertisedDevice dev = bleResults->getDevice(i);
    String addr = dev.getAddress().toString().c_str();
    int rssi = dev.getRSSI();
    String name = dev.getName().c_str();

    // Check if this is our target beacon
    if (name == TARGET_BEACON_NAME) {
        targetRssi = rssi;
        Serial.print("  >>> FOUND TARGET BEACON: ");
        Serial.print(name);
        Serial.print(" RSSI: ");
        Serial.println(rssi);
    }

    // Only print details for the first 5 devices to keep logs clean
    if (i < 5) {
        Serial.print("  #");
        Serial.print(i + 1);
        Serial.print(" -> ");
        Serial.print(addr);
        Serial.print("  RSSI=");
        Serial.print(rssi);
        if (name.length() > 0) {
            Serial.print(" Name=");
            Serial.print(name);
        }
        Serial.println();
    }
  }

  // Clear BLE results to free memory
  pBLEScan->clearResults();

  // ---------- 2. WiFi scan ----------
  Serial.println("[WiFi] Scanning...");
  // WiFi.scanNetworks() returns the number of WiFi access points found
  int wifiCount = WiFi.scanNetworks();
  
  // Analyze Environment Stability (Static vs Mobile)
  bool isMobile = false;
  std::vector<String> currentSsids;
  
  if (wifiCount > 0) {
      for (int i = 0; i < wifiCount; ++i) {
          currentSsids.push_back(WiFi.SSID(i));
      }
  }

  // Compare with last scan if we have history
  if (!lastSsids.empty() && !currentSsids.empty()) {
      int matchCount = 0;
      for (const auto& ssid : currentSsids) {
          for (const auto& lastSsid : lastSsids) {
              if (ssid == lastSsid) {
                  matchCount++;
                  break;
              }
          }
      }

      // Calculate similarity based on the larger of the two sets
      // This is a conservative approach: if the set size changes drastically, it's likely mobile
      int maxSize = max(lastSsids.size(), currentSsids.size());
      float similarity = (float)matchCount / maxSize;

      Serial.print("[WiFi] Similarity with last scan: ");
      Serial.print(similarity * 100);
      Serial.println("%");

      // Threshold: If less than 50% overlap, assume we moved
      if (similarity < 0.5) {
          isMobile = true;
      }
  } else if (lastSsids.empty() && !currentSsids.empty()) {
      // First scan, assume Static (or unknown)
      Serial.println("[WiFi] First scan, initializing history.");
  } else if (currentSsids.empty()) {
      // No WiFi found, hard to say. Keep previous state or default to Static.
      Serial.println("[WiFi] No networks found.");
  }

  // Update history
  lastSsids = currentSsids;

  Serial.print("[WiFi] Environment State: ");
  Serial.println(isMobile ? "MOBILE" : "STATIC");

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

  int totalSignals = bleCount + wifiCount;

  Serial.print("[SUMMARY] BLE unique count  = ");
  Serial.println(bleCount);
  Serial.print("[SUMMARY] WiFi count        = ");
  Serial.println(wifiCount);
  Serial.print("[SUMMARY] Total signals     = ");
  Serial.println(totalSignals);

  // ---------- 3. Compute simple crowd level ----------
  String crowdText;
  int crowdLevel = 0;  // 0 = CALM, 1 = MODERATE, 2 = CROWDED

  if (bleCount < THRESHOLD_CALM) {
    crowdLevel = 0;
    crowdText = "CALM";
  } else if (bleCount < THRESHOLD_CROWDED) {
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

      sendLoRaWANData( wifiCount, bleCount, targetRssi, isMobile, appPort );
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep(loraWanClass);
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}
}

