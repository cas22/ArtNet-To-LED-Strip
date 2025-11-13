# ArtNet to WS28xx LED Strip ESP Controller

A lightweight ESP-based controller that receives **ArtNet** DMX data over Ethernet or WiFi and drives **WS28xx** LED strips.

---

## Features

- Supports **WS28xx** LED strips.
- Receives **ArtNet DMX** over Ethernet/WiFi.
- Configurable number of LEDs, grouping and multi-DMX universe mapping.
- Smooth frame updates with minimal latency.
- Tested with Sound Switch (both with a PC and a Denon Prime 4 Plus) 
- OTA firmware updates.

---

## Hardware Requirements

- **ESP32-ETH01** (or an ESP32 Dev Kit)
- WS28xx LED strip
- Power supply capable of driving your LED strip (consider ~60mA per LED at full brightness)
- Optional: level shifter for 3.3V ESP → 5V LED data line

---

## Software Requirements

- Arduino IDE or PlatformIO
- ESP32 board support
- Libraries:
  - [NeoPixelBus](https://github.com/makuna/NeoPixelBus) (Tested with 2.8.3)
  - [Artnet](https://github.com/hideakitai/ArtNet) (Tested with 0.9.2)

---