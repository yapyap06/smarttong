# SmartTONG — Sistem Pemantauan Sisa Pintar
## Implementation Plan for AINS 2026 (Anugerah Inovasi Negeri Selangor)
**Tema: Kesejahteraan Negeri Selangor | Kategori: [pilih semasa daftar]**

---

## 1. Why this concept, and how it maps to the judging rubric

AINS 2026 marks you on five weighted components. Every design decision below is chosen to score on a specific one:

| Kriteria | Wajaran | How SmartTONG scores here |
|---|---|---|
| Faedah / Impak | 30% | Quantifiable fuel/labour savings for PBT truck routes, fewer overflow complaints, cleaner public spaces — all measurable and demoable with real numbers |
| Keberkesanan | 25% | Live fill-level data replaces guesswork; trucks visit bins that are actually full, not on a fixed schedule |
| Tahap Inovasi & Kreativiti | 20% | Predictive fill forecasting, touchless lid, retrofit form factor, citizen gamification layer |
| Kebolehlaksanaan & Replikasi | 15% | Clip-on retrofit kit — works on any existing bin, no capital bin-replacement cost, solar-powered so it works in low-infrastructure areas (good for Akar Umbi/rural replication) |
| Pembentangan & Dokumentasi | 10% | Clean product name/branding, working live dashboard, professional report and video |

**The core idea:** don't build a smart bin — build a **retrofit sensor pod** that clips onto bins the government already owns, feeding a live map + predictive analytics + a citizen reporting layer. This is the single choice that most improves your replication and cost-savings story versus every team that just puts a sensor in a new bin.

---

## 2. Product concept

**Name:** SmartTONG (*tong* = bin in Malay; also reads as "Smart-TON-G", i.e. Selangor)

**One-line pitch:** *A solar-powered retrofit sensor kit that turns any public bin into a live data point — so PBT trucks stop guessing, citizens stop smelling overflow, and Selangor gets measurably cleaner streets.*

**Three layers:**
1. **Hardware pod** — clips onto an existing bin lid, measures fill level, weight, and odour, reports over WiFi/LoRa, solar-charged.
2. **Cloud + AI backend** — stores readings, predicts *when* a bin will overflow (not just whether it's full now), computes optimized collection routes.
3. **Two front-ends** — a PBT operations dashboard, and a citizen PWA for reporting overflow/illegal dumping with a small rewards system.

**Stretch feature (mention in your report, build only if time allows):** the citizen app's "report" button can double as a generic civic-issue reporter — same QR code, same backend — letting a resident flag either an overflowing bin *or* a pothole nearby. This directly echoes both halves of the 2026 tema ("pembersihan awam dan jalan raya berlubang") and is a cheap way to show the judges you understood the theme, not just the waste half of it.

---

## 3. Hardware — bill of materials

Prices are rough Malaysia online-marketplace estimates (Shopee/Lazada/Cytron), for **one working demo node**. Build one full node; a second node is optional if you want to show the dashboard handling multiple bins.

| Component | Purpose | Est. price (RM) |
|---|---|---|
| ESP32 DevKit V1 (WiFi + BT) | Main controller | 18–25 |
| JSN-SR04T waterproof ultrasonic sensor | Fill-level sensing | 15–20 |
| Load cell (5–20 kg) + HX711 amplifier | Weight sensing (optional but strong for "data-driven" story) | 20–25 |
| MQ-135 gas sensor | Odour/decomposition detection (overflow proxy, novel) | 8–12 |
| IR proximity or PIR sensor | Touchless lid trigger | 5–8 |
| SG90 micro servo | Automatic lid open/close | 8–15 |
| WS2812 RGB LED (or 3x single LEDs) | Green/amber/red fill-status indicator, visible from outside | 5–10 |
| Active buzzer | Full-bin audible alert | 2 |
| 2x 18650 Li-ion cell + holder | Power storage | 12–15 |
| TP4056 solar charge/protection module | Solar charging circuit | 5 |
| Small 5–6V, 1–2W solar panel | Off-grid power | 15–25 |
| IP65 waterproof enclosure box | Weatherproofing | 15–25 |
| Perfboard, jumper wires, resistors, connectors | Assembly | 10–15 |
| **Subtotal (core node)** | | **≈ RM130–180** |

**Optional upgrades (only if budget/time allow):**

| Component | Purpose | Est. price (RM) |
|---|---|---|
| ESP32-CAM module | On-device waste-type image capture for the AI classification demo | 25–35 |
| LoRa module (RA-02 / SX1278) + a second unit as gateway | Long-range, low-power comms for rural/no-WiFi sites — strong "replication" argument | 35–45 each |
| SIM800L GSM module | Cellular fallback if no WiFi/LoRa coverage | 20–30 |

**Recommendation given your 13-day timeline:** build the **WiFi-only version first** (skip LoRa/GSM). Mention LoRaWAN as the "phase 2 replication path" in your report — reviewers reward a credible scaling story even if the demo itself uses WiFi. If parts don't arrive in time, see Section 6 — you can demo the entire data pipeline in simulation before physical parts even arrive.

---

## 4. Software architecture

```
Smart bin node (ESP32 + sensors)
        |  HTTPS/MQTT, every 5–15 min or on-trigger
        v
Connectivity gateway (WiFi direct, or LoRa->WiFi bridge)
        |
        v
Cloud backend (Firebase / Supabase)
   - Realtime DB or Postgres: bin readings, GPS, history
   - Cloud Function: fill-level -> "days to full" prediction (simple linear regression on fill-rate is enough for a demo)
   - Route engine: nearest-neighbour / Google Maps Directions API with waypoint optimization over "bins above 70% full"
        |
        +--> PBT dashboard (web app)
        |     - Live map (Leaflet or Google Maps JS) of all bins, colour-coded by fill %
        |     - Alerts feed ("Bin #7 will overflow in ~14 hours")
        |     - Optimized route view for the day's collection
        |     - Cost-savings calculator (fuel/labour saved vs fixed-schedule collection)
        |
        +--> Citizen PWA
              - QR code on each physical bin opens a report form (overflow, illegal dumping, nearby pothole)
              - Leaderboard / points for verified reports ("Juara Kebersihan")
              - Push notification when a nearby bin is emptied
```

**Suggested stack:**
- Firmware: Arduino/C++ on ESP32 (Arduino IDE or PlatformIO)
- Backend: Firebase (Firestore + Cloud Functions) — fastest to stand up solo in under two weeks; Supabase is a fine alternative if you prefer SQL
- Dashboard: React + Tailwind + Leaflet.js + Recharts for the analytics/cost charts
- Citizen app: a React PWA (installable, no app-store submission needed — important given your timeline)
- AI/ML for waste classification demo: Google's Teachable Machine (train in a browser, export TFLite) is the fastest path to a working classifier demo without building a model from scratch
- Route optimization: Google Maps Directions API with `optimizeWaypoints: true` is enough for a convincing demo — you don't need to write your own TSP solver

---

## 5. Building it with Google Antigravity

Antigravity is Google's agent-first IDE (Gemini 3 Pro by default, with Claude Sonnet models selectable). It plans a task, writes code, runs it in a terminal, and even opens a real browser to click through and verify your dashboard — which is genuinely useful here because it can generate the screenshots you need for your Lampiran B report as a side effect of testing.

**How to use it for this project:**

1. **Start in Plan Mode, not Fast Mode**, for anything non-trivial (the dashboard, the backend schema). Plan Mode makes the agent produce an implementation-plan artifact *before* touching code — review it, correct assumptions, then let it execute. This avoids wasted cycles given your tight deadline.
2. **Give it this document as context.** Paste (or attach) this plan into your first prompt so the agent scaffolds the right folder structure (`firmware/`, `backend/`, `dashboard/`, `citizen-app/`, `docs/`) in one pass instead of guessing.
3. **Split work into separate agent tasks**, since Antigravity supports running multiple agents in parallel via Manager View:
   - Agent A: dashboard (React + Leaflet + Firebase SDK)
   - Agent B: backend (Firestore schema, Cloud Functions for prediction + route optimization)
   - Agent C: citizen PWA
   - You handle firmware yourself (or with Fast Mode single-file help) since flashing hardware needs your physical presence anyway.
4. **Let the browser-in-the-loop agent self-test the dashboard.** Ask it to launch the dev server, open it in its embedded browser, click through the bin map and alerts, and report back with screenshots — those screenshots are usable directly in your Lampiran B "before/after" and "pelaksanaan" sections.
5. **Firmware caveat:** Antigravity's terminal agent can write and lint Arduino/C++ code, but it cannot flash a physical ESP32 for you — you'll compile/upload yourself via Arduino IDE or `platformio run --target upload`. Ask the agent to generate the firmware `.ino`/`.cpp` files and a `README.md` with the exact upload steps.
6. **Sample first prompt to give the agent** (adapt as needed):
   > "Scaffold a project with `firmware/` (ESP32 Arduino code for ultrasonic + load cell + MQ-135 + servo, publishing JSON to Firebase Realtime DB every 10 minutes), `backend/` (Firebase Cloud Functions: a fill-prediction function and a route-optimization function using Google Maps Directions API), `dashboard/` (React + Tailwind + Leaflet showing bins on a map colour-coded by fill %, with an alerts panel and a cost-savings calculator), and `citizen-app/` (a React PWA with a QR-triggered report form and a points leaderboard). Use Firestore for storage. Start in Plan Mode and show me the plan before writing code."

---

## 6. If hardware doesn't arrive in time

Given the 13-day window, don't let shipping delays block you. Start firmware development **today** in [Wokwi](https://wokwi.com) — a free browser-based ESP32 simulator that supports ultrasonic sensors, servos, and WiFi networking. You can develop and demo the *entire* sensor-to-cloud pipeline in simulation, then swap in real hardware the moment it arrives (the code doesn't change). This de-risks your whole timeline.

---

## 7. 13-day build timeline (today = 2 July, deadline = 15 July 2026)

| Days | Dates | Focus |
|---|---|---|
| 1–2 | 2–3 Jul | Lock the concept, name, category. Order hardware online (same-day/next-day delivery services). Start Wokwi simulation of the firmware in parallel. Set up Antigravity project scaffold. |
| 3–5 | 4–6 Jul | Firmware: sensor reading + WiFi publish loop (in Wokwi and/or on real ESP32 once it arrives). Backend: Firestore schema + basic write/read. Dashboard: map skeleton with mock data. |
| 6–8 | 7–9 Jul | Wire real sensors if hardware has arrived. Backend: prediction function + route optimization endpoint. Dashboard: live data binding, alerts panel, cost-savings calculator. |
| 9–10 | 10–11 Jul | Citizen PWA: QR report form, leaderboard. Assemble physical demo enclosure (IP65 box, servo lid, LED indicator). End-to-end test: sensor → cloud → dashboard, live. |
| 11–12 | 12–13 Jul | Write the Lampiran B report (max 20 pages — use Antigravity's dashboard screenshots for "pelaksanaan" section). Storyboard and film the ≤5-minute Lampiran C video. Get sign-off from Ketua Jabatan/Majikan/Dekan/Pengetua if your category requires it (all except Akar Umbi). |
| 13 | 14 Jul | Final QA, proofreading, upload document/video links (must be unlisted, not private) to Hab metaSEL, complete the online entry form. |
| — | 15 Jul | **Submission deadline.** Submit at least a few hours early — don't cut it to the wire on a government portal. |

**After Penilaian 1**, if shortlisted: Penilaian 2 (September, 5-minute Microsoft Teams pitch) and Penilaian 3 (October, physical demo at Dewan Jubli Perak PSUK Selangor, 10-minute presentation + possible site visit for gov/private entries).

---

## 8. Report and video checklist (Lampiran B & C requirements)

**Report (max 20 pages)** must cover, per the official format: Pengenalan, Analisis Punca, Penyelesaian, Pelaksanaan, Impak dan Data, Kos, Potensi Replikasi. Two sections deserve extra effort because they map straight to the highest-weighted rubric items:
- **Impak dan Data (feeds the 30%-weighted Faedah/Impak criterion):** even with a small pilot, show *some* numbers — estimated collection trips saved per week, estimated fuel cost saved, estimated reduction in overflow incidents. Projected numbers with clear assumptions are acceptable and expected for a prototype.
- **Kebolehlaksanaan & Replikasi (15%):** explicitly state the retrofit cost per bin, and that it requires no new bin purchase — this is your strongest differentiator, make it visible.

**Video (≤5 minutes)** should, in order: state the problem (overflowing bins/public cleanliness in Selangor) in the first 20–30 seconds, show the physical pod being clipped onto a bin, show the dashboard live with real or simulated data, show the citizen app QR-report flow, close with the cost/impact numbers. Keep narration over B-roll rather than a static talking head — panels are scoring dozens of entries and a demo-driven video reads as more credible than a pitch-driven one.

---

## 9. Contact for questions

Seksyen Pembudayaan Inovasi, Bahagian Korporat, Pejabat Setiausaha Kerajaan Negeri Selangor
Tel: 03-55447053 | E-mel: inovasi@selangor.gov.my
