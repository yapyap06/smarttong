# SmartTONG AI — Physical Enclosure & Hardware Assembly Guide
**A guide to retrofitting a standard green wheelie bin with the SmartTong Pod**

This guide provides instructions to assemble the physical scale-model or pilot prototype of the SmartTong retrofit pod. The design is optimized for water resistance (IP65) and easy installation on the lid of any existing municipal wheelie bin.

---

## 🛠️ Required Materials & Tools
1. **IP65 Waterproof Project Box** (approx. 115mm × 90mm × 55mm, clear lid recommended for NeoPixel status viewing).
2. **JSN-SR04T Waterproof Ultrasonic Sensor Probe** (with 2.5m cable).
3. **SG90 9g Micro Servo** or **12V Solenoid Latch + Relay Module**.
4. **0.96" SSD1306 OLED Display Screen** (I2C).
5. **WS2812B NeoPixel RGB LED Ring** or single diffuse RGB LED.
6. **MQ-135 Gas Sensor** + **PIR Motion Sensor**.
7. **ESP32 NodeMCU Dev Kit** + **18650 Battery Shield & Batteries**.
8. **Small 5V/6V Solar Panel** (1W - 2W).
9. **Drill with 22mm hole saw bit** (for JSN-SR04T probe mounting) and small bits.
10. **Hot glue gun & silicone sealant** (for weatherproofing).

---

## 📐 Enclosure Layout & Drilling Template

```
                   TOP VIEW OF IP65 POD LID
        ┌──────────────────────────────────────────────┐
        │  [====== Solar Panel Mounting Bracket ======]│
        │                                              │
        │      ┌──────────────────────────────┐        │
        │      │    [==== OLED Window ====]   │        │
        │      │          36mm x 20mm         │        │
        │      └──────────────────────────────┘        │
        │                                              │
        │   (O) RGB LED Indicator      ( ) PIR Lens    │
        │                                              │
        └──────────────────────────────────────────────┘
```

### Holes to Drill:
1. **Lid Underside Cutout**:
   - Drill a **22mm hole** directly through the center of the wheelie bin lid where the waterproof ultrasonic sensor probe will point downward into the bin cavity.
   - Secure the JSN-SR04T probe rubber gasket tightly and seal it with silicone glue.
2. **OLED Faceplate Window**:
   - Cut a rectangular **36mm × 20mm hole** on the front or top face of the project box.
   - Use a clear acrylic sheet or transparent tape on the inside to protect the screen from water splashes.
3. **PIR Sensor Cutout**:
   - Drill a **12mm hole** for the dome of the PIR sensor. Ensure it points slightly outward to detect citizens approaching from 1-2 meters away.
4. **Wiring Pass-through**:
   - Drill a small **5mm hole** on the lower side of the project box for the solar panel input wires. Use a cable gland (PG7) to ensure waterproof sealing.

---

## 🔌 Actuator Latch & Servo Installation
To demonstrate remote slot unlocking or automatic opening:
1. **Lid Flap Mechanism**:
   - Cut a small flap/door (15cm × 15cm) on one side of the main wheelie bin lid (specifically for recyclables).
   - Mount the SG90 servo motor inside the lid using hot glue.
   - Connect the servo horn to the flap locking tab using a rigid steel wire or paperclip.
   - Calibration: 
     - **0 degrees**: Locked (latch holds flap closed).
     - **90 degrees**: Unlocked (flap drops or can be pushed open easily).

---

## ☀️ Solar Charging Integration
1. Mount the solar panel on a bracket on top of the project box at a **15-degree angle** facing South (optimal for Selangor, Malaysia).
2. Wire the solar panel outputs to the input pins of the **TP4056 Solar Charger Module**.
3. Wire the TP4056 output to the 18650 Battery array, which powers the ESP32 `5V` (or `VIN`) and `GND` pins.
4. **Resistor Divider**: 
   - Connect the battery positive terminal `VBAT` to the resistor divider ($R_1 = 100\text{k}\Omega$, $R_2 = 47\text{k}\Omega$).
   - Connect the midpoint of the divider to ESP32 **GPIO 35** for real-time voltage monitoring.

---

## 🏷️ QR Code Label Placement
1. Generate a QR code containing the URL of the Citizen PWA pre-loaded with the Bin ID (e.g. `https://yourdomain.com/citizen-app/index.html?binId=B01`).
2. Print the QR code on a high-durability vinyl sticker.
3. Stick the label on the **front-center of the wheelie bin lid**, directly below the project box, with a call-to-action:
   > **"IMBAS QR & IMBAS BARANG UNTUK MULA KITAR SEMULA"** (Scan QR & scan item to start recycling).
