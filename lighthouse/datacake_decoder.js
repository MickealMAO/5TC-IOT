function Decoder(payload, port) {
    // Expected Payload Structure (6 bytes):
    // Bytes 0-1: WiFi Count (uint16_t)
    // Bytes 2-3: BLE Count (uint16_t)
    // Byte 4:   Beacon RSSI (uint8_t: 0=Not Found, >0 = abs(RSSI))
    // Byte 5:   Environment State (uint8_t: 0=Static, 1=Mobile)

    // --- Safety Check: Validate Payload Length ---
    if (payload.length !== 6) {
        return [{ field: "ERROR", value: "Invalid payload length. Expected 6 bytes, got " + payload.length }];
    }

    // 1. Decode WiFi Count (Bytes 0-1)
    var wifi_count = (payload[0] << 8) | payload[1];

    // 2. Decode BLE Count (Bytes 2-3)
    var ble_count = (payload[2] << 8) | payload[3];

    // 3. Decode Beacon RSSI (Byte 4)
    var rssiByte = payload[4];
    var beacon_rssi;
    var beacon_detected;

    if (rssiByte === 0) {
        beacon_detected = false;
    } else {
        beacon_detected = true;
        // Convert the absolute value from the payload into a negative dBm value
        beacon_rssi = -1 * rssiByte;
    }

    // 4. Decode Environment State (Byte 5)
    var envByte = payload[5];
    var environment_state = (envByte === 1) ? "MOBILE" : "STATIC";

    // 5. Calculate Total Signals
    var total_signals = wifi_count + ble_count;

    // 6. Determine Crowd Level (Using the new thresholds: 40 and 80)
    var crowdText;
    var THRESHOLD_CALM = 40;
    var THRESHOLD_CROWDED = 80;

    if (total_signals < THRESHOLD_CALM) {
        crowdText = "CALM";
    } else if (total_signals < THRESHOLD_CROWDED) {
        crowdText = "MODERATE";
    } else {
        crowdText = "CROWDED";
    }

    // 7. Return Datacake formatted JSON array
    // Ensure all identifiers here match your Datacake Fields exactly.
    return [
        {
            field: "WIFI_COUNT",
            value: wifi_count
        },
        {
            field: "BLE_COUNT",
            value: ble_count
        },
        {
            field: "TOTAL_SIGNALS",
            value: total_signals
        },
        {
            field: "CROWD_STATUS",
            value: crowdText
        },
        {
            field: "BEACON_DETECTED",
            value: beacon_detected
        },
        {
            field: "BEACON_RSSI",
            value: beacon_rssi
        },
        {
            // NEW FIELD: Environment State
            field: "ENVIRONMENT_STATE",
            value: environment_state
        }
    ];
}