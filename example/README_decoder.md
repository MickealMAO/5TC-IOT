# TTN Payload Decoder for RoomGuard

这些decoder用于解码从ESP32设备发送到The Things Network (TTN)的LoRaWAN数据。

## Payload格式

设备发送**4字节**的数据：

| Byte | 描述 | 类型 | 编码方式 |
|------|------|------|----------|
| 0-1  | WiFi设备数量 | uint16_t | Big-endian |
| 2-3  | BLE设备数量 | uint16_t | Big-endian |

## 解码字段

Decoder会自动解码并计算以下字段：

- **wifi_count**: 检测到的WiFi设备数量
- **ble_count**: 检测到的BLE设备数量  
- **total_signals**: 总信号数 (WiFi + BLE)
- **crowd_level**: 拥挤度级别 (0=CALM, 1=MODERATE, 2=CROWDED)
- **crowd_text**: 拥挤度文本描述

### 拥挤度阈值

根据Arduino代码中的定义：
- **CALM** (0): total_signals < 5
- **MODERATE** (1): 5 ≤ total_signals < 20
- **CROWDED** (2): total_signals ≥ 20

## 如何使用

### TTN v3 (The Things Stack)

1. 登录TTN Console
2. 进入你的Application
3. 选择 **Payload formatters** → **Uplink**
4. 选择 **Formatter type**: **JavaScript**
5. 复制 `ttn_decoder_v3.js` 的内容到编辑器
6. 点击 **Save changes**

### TTN v2 (The Things Network V2)

1. 登录TTN Console (v2)
2. 进入你的Application
3. 选择 **Payload Formats** 标签
4. 在 **decoder** 部分，复制 `ttn_decoder_v2.js` 的内容
5. 点击 **Save**

## 测试示例

### 示例1: 少量设备
**Payload**: `00 03 00 01` (WiFi=3, BLE=1)

**解码结果**:
```json
{
  "wifi_count": 3,
  "ble_count": 1,
  "total_signals": 4,
  "crowd_level": 0,
  "crowd_text": "CALM"
}
```

### 示例2: 中等拥挤
**Payload**: `00 08 00 04` (WiFi=8, BLE=4)

**解码结果**:
```json
{
  "wifi_count": 8,
  "ble_count": 4,
  "total_signals": 12,
  "crowd_level": 1,
  "crowd_text": "MODERATE"
}
```

### 示例3: 高度拥挤
**Payload**: `00 0F 00 10` (WiFi=15, BLE=16)

**解码结果**:
```json
{
  "wifi_count": 15,
  "ble_count": 16,
  "total_signals": 31,
  "crowd_level": 2,
  "crowd_text": "CROWDED"
}
```

## 与Arduino代码的对应关系

Arduino代码 (example.ino):
```cpp
void sendLoRaWANData(uint16_t wifiCount, uint16_t bleCount, uint8_t port) {
    appDataSize = 4;
    appData[0] = (wifiCount >> 8) & 0xFF;  // WiFi高字节
    appData[1] = wifiCount & 0xFF;          // WiFi低字节
    appData[2] = (bleCount >> 8) & 0xFF;   // BLE高字节
    appData[3] = bleCount & 0xFF;           // BLE低字节
}
```

JavaScript Decoder:
```javascript
data.wifi_count = (input.bytes[0] << 8) | input.bytes[1];
data.ble_count = (input.bytes[2] << 8) | input.bytes[3];
```

## 注意事项

⚠️ **重要**: 当前Arduino代码只发送4字节数据，虽然注释中提到5字节包含crowd level，但实际`appDataSize = 4`。如果将来修改Arduino代码添加第5字节发送crowd level，需要相应更新decoder。

建议的改进（可选）：
```cpp
// 在Arduino代码中添加crowd level作为第5字节
appDataSize = 5;
appData[4] = crowdLevel;  // 添加这一行
```

然后在decoder中：
```javascript
if (input.bytes.length === 5) {
  data.crowd_level_device = input.bytes[4];  // 从设备读取
}
```

## 数据可视化建议

在TTN或第三方平台（如Cayenne、Grafana等）中，可以使用以下方式可视化数据：

1. **时间序列图**: 显示wifi_count和ble_count随时间的变化
2. **面积图**: 显示total_signals的趋势
3. **颜色指示器**: 根据crowd_text显示不同颜色
   - CALM: 🟢 绿色
   - MODERATE: 🟡 黄色
   - CROWDED: 🔴 红色
4. **实时仪表盘**: 显示当前拥挤状态

## 故障排除

### 解码失败
- 检查payload长度是否为4字节
- 验证设备是否正确发送数据
- 检查TTN上的原始payload

### 数值异常
- WiFi/BLE count超过预期：检查设备周围环境
- 始终显示0：检查设备扫描功能是否正常工作

## 许可证

此decoder与RoomGuard项目共享相同的许可证。
