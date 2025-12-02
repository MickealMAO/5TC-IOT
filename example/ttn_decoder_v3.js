// TTN v3 Payload Decoder for RoomGuard Device
// This decoder processes the 4-byte payload sent by the ESP32 device
// Payload format:
//   Byte 0-1: WiFi count (uint16_t, big-endian)
//   Byte 2-3: BLE count (uint16_t, big-endian)

function decodeUplink(input) {
  var data = {};
  var warnings = [];
  var errors = [];
  
  // Check if we have the expected payload size
  if (input.bytes.length !== 4) {
    errors.push("Invalid payload length. Expected 4 bytes, got " + input.bytes.length);
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
// Test with example payload: [0x00, 0x08, 0x00, 0x0C]
// Expected output:
// {
//   wifi_count: 8,
//   ble_count: 12,
//   total_signals: 20,
//   crowd_level: 2,
//   crowd_text: "CROWDED"
// }
