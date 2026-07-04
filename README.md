# SmartTONG AI 🗑️
## Sistem Pemantauan Sisa Pintar Berkuasa AI · AINS 2026

> *A solar-powered retrofit sensor kit + AI that turns any public bin into a live data point — with waste classification, overflow prediction, OKU audio guidance, and real-time PBT route optimization.*

---

## 📁 Project Structure

```
SmartTong/
├── firmware/
│   ├── SmartTong_main/          ← Arduino sketch (flash to ESP32)
│   │   ├── SmartTong_main.ino   ← MAIN ENTRY POINT
│   │   ├── Config.h             ← ⚙️  EDIT THIS FIRST (WiFi, Firebase, pins)
│   │   ├── SensorHub.h/.cpp     ← Ultrasonic, MQ-135, PIR
│   │   ├── CameraAI.h/.cpp      ← AI waste classifier (mock → real swap)
│   │   ├── LidController.h/.cpp ← Servo lid control
│   │   ├── LEDStatus.h/.cpp     ← NeoPixel fill indicator
│   │   ├── OKUAudio.h/.cpp      ← DFPlayer Mini audio feedback
│   │   └── MQTTClient.h/.cpp    ← WiFi + Firebase HTTP publish
│   ├── wokwi/
│   │   ├── diagram.json         ← Paste into Wokwi circuit editor
│   │   └── wokwi.toml           ← Wokwi project config
│   └── edge_impulse/
│       └── TRAINING_GUIDE.md   ← Full Edge Impulse training walkthrough
├── backend/
│   ├── functions/src/
│   │   ├── index.ts             ← Firebase Functions entry point
│   │   ├── predictOverflow.ts   ← AI fill prediction (linear regression)
│   │   ├── optimizeRoute.ts     ← Truck route optimization
│   │   ├── detectAnomaly.ts     ← Real-time alert generation
│   │   ├── aggregateKPIs.ts     ← Daily KPI cron job
│   │   └── seedDemoData.ts      ← One-time demo data seeder
│   └── firestore.rules          ← Security rules
├── dashboard/
│   └── index.html               ← PBT Ops Dashboard (open in browser)
├── citizen-app/
│   ├── index.html               ← Citizen PWA (Juara Kebersihan)
│   ├── manifest.json            ← PWA manifest
│   └── sw.js                    ← Service worker (offline support)
└── docs/                        ← Lampiran B screenshots go here
```

---

## ⚡ Quick Start (No Hardware Yet — Wokwi Simulation)

### 1. Open Wokwi Simulation
1. Go to [https://wokwi.com/projects/new/esp32](https://wokwi.com/projects/new/esp32)
2. Copy the contents of `firmware/SmartTong_main/SmartTong_main.ino` into the code editor
3. Copy `SensorHub.h/cpp`, `CameraAI.h/cpp`, `LidController.h/cpp`, `LEDStatus.h/cpp`, `OKUAudio.h/cpp`, `MQTTClient.h/cpp`, `Config.h` into separate tabs
4. In the diagram editor, paste the contents of `firmware/wokwi/diagram.json`
5. Install libraries in Wokwi: ArduinoJson, Adafruit NeoPixel, ESP32Servo
6. Press ▶ to simulate!

### 2. Set Up Firebase
1. Go to [https://console.firebase.google.com](https://console.firebase.google.com) → New Project
2. Enable **Firestore Database** (test mode for development)
3. Enable **Realtime Database** (for ESP32 HTTPS publishing)
4. Copy your Firebase config into `Config.h` (FIREBASE_HOST, FIREBASE_AUTH)
5. Copy Firebase Web config into `dashboard/index.html` and `citizen-app/index.html`
6. Deploy Cloud Functions:
   ```bash
   cd backend/functions
   npm install
   firebase deploy --only functions
   ```
7. Seed demo data:
   ```
   GET https://your-region-your-project.cloudfunctions.net/seedDemoData
   ```

### 3. Open Dashboard
- Open `dashboard/index.html` in any browser — **no server needed**
- Works immediately with demo data
- Connects to Firebase automatically when config is set

### 4. Open Citizen App
- Open `citizen-app/index.html` in a mobile browser
- Or serve with: `python -m http.server 3000` and visit `localhost:3000`
- Install as PWA: Chrome → "Add to Home Screen"

---

## 🔧 Hardware Setup (When Parts Arrive)

### Required Libraries (Arduino IDE → Library Manager)
```
Adafruit NeoPixel      by Adafruit
ESP32Servo             by Kevin Harrington
ArduinoJson            by Benoit Blanchon  (v6.x)
```

### Pin Connections

| Component | ESP32 Pin | Notes |
|---|---|---|
| Ultrasonic TRIG | GPIO 5 | |
| Ultrasonic ECHO | GPIO 18 | |
| MQ-135 AO | GPIO 34 | ADC1 only (34-39) |
| PIR OUT | GPIO 14 | |
| Servo Signal | GPIO 13 | |
| NeoPixel DIN | GPIO 27 | |
| DFPlayer RX | GPIO 16 (ESP32 TX2) | |
| DFPlayer TX | GPIO 17 (ESP32 RX2) | |

All pins are configurable in `Config.h`.

### First Flash Steps
1. Select board: **ESP32 Dev Module**
2. Partition scheme: **Default 4MB with spiffs**
3. PSRAM: **Disabled** (enable when using ESP32-CAM)
4. Upload speed: 115200
5. Flash, open Serial Monitor at 115200 baud, watch startup logs

---

## 🤖 AI Classifier — Swap Mock → Real

The firmware ships with a mock classifier for Wokwi testing.  
When hardware + Edge Impulse model are ready:

1. Follow `firmware/edge_impulse/TRAINING_GUIDE.md`
2. Install the exported Edge Impulse library (`.zip`)
3. In `CameraAI.h`, uncomment line: `#define USE_REAL_CLASSIFIER`
4. In `CameraAI.cpp`, uncomment the ESP32-CAM init and `run_classifier()` code
5. Re-flash — **no other code changes needed**

---

## 📊 Impact Numbers (for Lampiran B)

| Metric | Value | Source |
|---|---|---|
| Malaysia daily waste | 39,900 tonnes | KPKT 2025 estimate |
| Landfill dependency | 82.5% | Solid Waste Corp 2024 |
| Selangor KDEBWM trucks | 1,100+ | kdebwm.com |
| Illegal dumpsites closed (2025) | 3,634 | KPKT enforcement data |
| SmartTONG trip reduction | ~25% | Nearest-neighbour route sim |
| Fuel saved (100 bins, weekly) | RM 1,176 | Calculated: 420 L × RM 2.80 |
| CO₂ avoided (100 bins, weekly) | 1.1 tonnes | 2.68 kg/L × 420 L |
| Overflow reduction | ~77% | Alarm-triggered collection |
| Retrofit cost per bin | ~RM 180 | Shopee/Cytron BOM estimate |
| Payback period (100 bins) | ~15 weeks | Fuel savings alone |

---

## 📅 13-Day Timeline

| Day | Date | Milestone |
|---|---|---|
| 1 | 2 Jul | Order hardware · Wokwi sim start · Firebase setup |
| 2 | 3 Jul | Firmware complete in Wokwi · Firebase publishing live |
| 3 | 4 Jul | Dashboard live with real Firestore data |
| 4 | 5 Jul | Citizen PWA QR report flow working · Edge Impulse dataset collection |
| 5 | 6 Jul | Model trained >80% · Hardware arriving |
| 6 | 7 Jul | Physical wiring on perfboard |
| 7 | 8 Jul | ESP32-CAM real classifier integrated |
| 8 | 9 Jul | DFPlayer OKU audio working · Full LED status |
| 9 | 10 Jul | End-to-end test: sensor → Firebase → dashboard |
| 10 | 11 Jul | IP65 enclosure assembly · Solar charging circuit |
| 11 | 12 Jul | Lampiran B report writing |
| 12 | 13 Jul | Lampiran C video recording & editing |
| 13 | 14 Jul | Final QA · Upload to Hab metaSEL |
| — | 15 Jul | **SUBMISSION DEADLINE** |

---

## 🏆 AINS 2026 Rubric Mapping

| Kriteria | Wajaran | SmartTONG AI advantage |
|---|---|---|
| Faedah/Impak | 30% | Quantified RM savings + OKU inclusion + recycling KPI |
| Keberkesanan | 25% | Live data + 4–12h overflow prediction |
| Inovasi & Kreativiti | 20% | On-device TFLite + OKU audio = first in Malaysia |
| Kebolehlaksanaan | 15% | RM 180 retrofit, solar, no new bins needed |
| Pembentangan | 10% | Live dashboard demo + structured video |

---

## 🔗 Resources

- **Wokwi** (simulation): https://wokwi.com/projects/new/esp32
- **Edge Impulse** (AI training): https://studio.edgeimpulse.com
- **Firebase Console**: https://console.firebase.google.com
- **AINS 2026 Portal**: https://inovasi.selangor.gov.my
- **Hab metaSEL**: https://hab.selangor.gov.my

---

*SmartTONG AI · AINS 2026 · Kesejahteraan Negeri Selangor*
