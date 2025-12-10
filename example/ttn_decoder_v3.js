// TTN v3 Payload Decoder for RoomGuard Device
// This decoder processes the 5-byte payload sent by the ESP32 device
// Payload format:
//   Byte 0-1: WiFi count (uint16_t, big-endian)
//   Byte 2-3: BLE count (uint16_t, big-endian)
//   Byte 4:   Beacon RSSI (uint8_t: 0=Not Found, >0 = abs(RSSI))

function decodeUplink(input) {
  var data = {};
  var warnings = [];
  var errors = [];

  // Check if we have the expected payload size
  if (input.bytes.length !== 5) {
    errors.push("Invalid payload length. Expected 5 bytes, got " + input.bytes.length);
    return {
      data: data,
      warnings: warnings,
      errors: errors
    };
  }

  // Decode WiFi count (bytes 0-1, big-endian)
  data.wifi_count = (input.bytes[0] << 8) | input.bytes[1];

  // Decode BLE count (bytes 2-3, big-endian)
  data.ble_count = (input.bytes[2] << 8) | input.bytes[3];

  // Decode Beacon RSSI (byte 4)
  var rssiByte = input.bytes[4];
  if (rssiByte === 0) {
    data.beacon_detected = false;
    data.beacon_rssi = null;
  } else {
    data.beacon_detected = true;
    data.beacon_rssi = -1 * rssiByte; // Convert back to negative dBm
  }

  // Calculate total signals
  data.total_signals = data.wifi_count + data.ble_count;

  // Determine crowd level based on thresholds
  // These thresholds match the Arduino code:
  // THRESHOLD_CALM = 5
  // THRESHOLD_CROWDED = 20
  var crowdLevel;
  var crowdText;

  if (data.total_signals < 5) {
    crowdLevel = 0;
    crowdText = "CALM";
  } else if (data.total_signals < 20) {
    crowdLevel = 1;
    crowdText = "MODERATE";
  } else {
    crowdLevel = 2;
    crowdText = "CROWDED";
  }

  data.crowd_level = crowdLevel;
  data.crowd_text = crowdText;

  // Add metadata for better visualization
  data.timestamp = new Date().toISOString();

  return {
    data: data,
    warnings: warnings,
    errors: errors
  };
}

// Example usage and test cases
// Test with example payload: [0x00, 0x08, 0x00, 0x0C, 0x50]
// Expected output:
// {
//   wifi_count: 8,
//   ble_count: 12,
//   beacon_detected: true,
//   beacon_rssi: -80,
//   total_signals: 20,
//   crowd_level: 2,
//   crowd_text: "CROWDED"
// }
