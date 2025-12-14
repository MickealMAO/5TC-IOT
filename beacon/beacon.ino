#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>

// Define the name you want to broadcast
#define DEVICE_NAME "RG_BEACON"

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Beacon: " DEVICE_NAME);

  // Initialize the BLE device and set the local name
  // This name will be broadcasted and visible to scanners
  BLEDevice::init(DEVICE_NAME); 

  // Set the TX power for Advertising (ADV), Scan Response (SCAN), and Default
  esp_err_t advResult = esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
  esp_err_t scanResult = esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, ESP_PWR_LVL_P9);
  esp_err_t defResult = esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);

  // Create a BLE Advertising object
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  
  // Set the device name in the advertising data
  pAdvertising->setAppearance(0x0000); // Generic appearance
  pAdvertising->setScanResponse(true); // Allow scanning for more info (e.g., name)

  // Set advertising intervals (in multiples of 0.625ms). 
  // 0x20 is 20ms, 0x40 is 40ms. Lower interval means more frequent, but higher power use.
  pAdvertising->setMinPreferred(0x06); // Tmin = 6 * 0.625ms = 3.75ms
  pAdvertising->setMaxPreferred(0x08); // Tmax = 8 * 0.625ms = 5.0ms
  
  // Start advertising
  BLEDevice::startAdvertising();

  Serial.println("BLE advertising started as: " DEVICE_NAME);
  Serial.println("Scan for the device with any BLE scanner app.");
}

void loop() {
  // In a simple beacon application, the loop can be empty 
  // or used for other tasks like checking battery, or going into deep sleep.
  delay(2000); // Wait for 2 seconds
}
