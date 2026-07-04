```markdown
# PROJECT PROPOSAL: SMARTTONG

**Category:** Institusi Pengajian Tinggi (IPT) / Akar Umbi  
**Theme Alignment:** Kesejahteraan Negeri Selangor (Public Cleansing & Digital Infrastructure Optimization)  
**Target Beneficiaries:** Pihak Berkuasa Tempatan (PBT e.g., MPKS), KDEB Waste Management (KDEBWM), and the Citizens of Selangor.  

---

## 1. Executive Summary

SmartTong is an end-to-end, IoT-driven, and AI-orchestrated solid waste management system designed explicitly to address critical public sanitation challenges in Selangor. Shifting from legacy reactive models, SmartTong introduces a proactive framework combining an **IoT Parasitic Retrofit Cap** for existing municipal bins with a **Clean-Theme, Dual-Interface Responsive Web Application (Citizen Portal & PBT/KDEB Command Dashboard)**. 

Driven by an automated multi-agent architecture, the system operates on a default-locked security mechanism. Bins open their intake flaps in real-time only after an app-based computer vision validation step verifies the item classification, completely preventing cross-contamination. This optimized ecosystem slashes municipal logistics costs, introduces low-cost asset protection against vandalism, and features complete audio-visual accessibility workflows.

---

## 2. Problem Statement & Strategic Relevance

The Selangor State Government and regional PBTs face high operational expenses and logistical gaps in waste management:
1. **Inefficient Routing & Asset Waste:** Standard trucks follow fixed routes, visiting empty suburban units while missing overflowing urban hotspots, which leads to leachate spills and drain clogs.
2. **Recycling Cross-Contamination:** Public recycling spaces suffer from high failure rates because users mix wet/organic waste into clean material channels, invalidating entire batches.
3. **The Theft and Cost Scaling Problem:** Deploying fully integrated smart dumpsters costing thousands of ringgit per unit introduces massive capital risks and vulnerability to electronic component theft.
4. **Accessibility Isolation:** Standard city waste infrastructure lacks validation systems or sensory feedback loops for individuals with visual or auditory impairments.

---

## 3. Product Architecture Overview


```

[IoT Parasitic Cap Module] ──(MQTT/WebSockets)──> [Multi-Agent Core API]
│
┌────────────────────────────────────────────┴────────────────────────────────────────────┐
▼                                                                                         ▼
[Citizen Interface: Rakyat Portal]                                                       [Government Interface: PBT Dashboard]

* Light-Theme Minimalist Design                                                          - Light-Theme Management Console
* Default Locked Flap Bypass via App Scan                                                - Proactive AI Fleet Route Planner
* Scoped Gemini "SilaTanya AI" Assistant                                                 - Integrated Geographic Aduan Engine
* Lestari Point Tiers & Dynamic Local Map                                                - Broadcast Broadcast & Winner Selector

```

### 3.1 UI/UX Visual Identity Specifications
To optimize usability during field testing and presentation evaluations, both interfaces follow a unified UI design strategy based on the current layout architecture found in `image_dc8f46.png`:
* **Main Theme Color Profile:** Clean white background primary layout (`#FFFFFF`), light gray utility panels (`#F8F9FA`), and high-contrast dark charcoal text components (`#212529`) replacing the dark green aesthetic.
* **Icon and Vector Engine:** All interface vectors and control buttons are rendered from the Lucide system library, using clear indicator lines without emojis.
* **Layout Structure:** Screen views use flat, minimal container styling. In alignment with `image_dc8f46.png`, the primary navigation layout relies on a persistent **Bottom Menu Bar** for rapid mobile switching.

### 3.2 Secure Onboarding & Authentication Layer
* **Citizen Login:** Requires validation mapping **Full Name, MyKad (IC Number), Mobile Number, and Housing Address**. These inputs are strictly verified against local municipal zoning boundaries to ensure data compliance under the PDPA.
* **Government Login:** Access is restricted through a secure **Command Code / PBT Personnel Token** mapped to official council staff credentials.

---

## 4. Hardware System Specifications (The Parasitic IoT Cap)

To avoid heavy capital expenditure and theft issues, the physical component is built as a modular retrofitting cap that clips beneath the lid of existing green municipal bins, maintaining a low component budget of approximately **RM 145.00**.

### 4.1 Bill of Materials (BOM)
* **Microcontroller Node:** ESP32 NodeMCU Board (Handles deep-sleep states and registers direct server data feeds via Wi-Fi/WebSockets).
* **Volumetric Sensor:** Waterproof Ultrasonic Transducer (JSN-SR04T) mounted facing down to calculate bin capacity.
* **Decomposition Probe:** Analog Waste/Soil Moisture Sensor positioned near the base lining to map fluid leaks or illegal commercial wet waste dumping.
* **Power Framework:** 3.7V 18650 Li-ion Battery Array + Custom Voltage Divider Resistor Loop (Allows the ESP32 to monitor its own battery decline).
* **Enclosure Shielding:** Waterproof ABS Plastic Box completely encasing internal components.
* **Actuator Mechanism:** 5V Micro Servo / Solenoid Lock Latch + Single-Channel Relay Module (Acts as the electronic lock keeping the intake flap shut).
* **Exhibition Indicators:** 0.96" I2C OLED Screen + Common Anode RGB LED (Provides physical visual validation status at the evaluation booth).

### 4.2 Locked-Flap Mechanical Logic Pipeline
1. **Default Secured State:** All intake slots remain physically locked. 
2. **Real-Time Validation Handshake:** When the user scans the bin's QR code and completes an image verification scan on their phone, the backend approves the item.
3. **Flap Activation:** The cloud backend pushes an encrypted token to the local ESP32. The microcontroller triggers the relay module, pulling back the solenoid latch or moving the micro-servo arm to unlock that specific category door.
4. **Safety Lockout Override:** The local sensor monitors volume levels. If capacity hits $\ge 85\%$ or the moisture probe flags severe fluid leakage, the door automatically locks shut and displays a critical warning message.

---

## 5. Software System Specifications (The Refined SmartTong App)

### 5.1 Government Interface (PBT / KDEB Dashboard)
1. **Home (AI Logistic Dispatch & Predictive Core):** Displays structural statuses marked as *Low, Normal, or Kritikal*. The multi-agent engine calculates a **Virtual Fill Rate ($\Delta V / \Delta t$)** based on neighborhood business density and historical trends, automatically arranging compactor truck routes before bins overflow. Includes a physical button for manual schedule override.
2. **Map (Dynamic Interruption Grid):** Shows live contractor compactor navigation lines overlaying a district layout. Features a clean dropdown menu to filter fields specifically for the **Selangor Zone** (sorting by districts like Petaling, Klang, or Kuala Selangor). Lists all monitored bins arranged from most critical to least.
3. **Report (Integrated Aduan Hub):** Consolidates citizen-submitted complaints, showing complete issue details alongside geo-tagged photographic evidence. Includes action buttons to instantly dispatch an emergency truck or flag a bin for maintenance.
4. **Award (Lestari Rewards Broadcaster):** Tracks monthly recycling leaderboards. Automatically filters the top 10 recycling champions each month and includes a text console allowing PBT staff to broadcast custom, point-backed congratulatory announcements directly to their accounts.

### 5.2 Citizen Interface (Rakyat Portal)
1. **Home (Main Operations Hub):** Displays accumulated Eco-Points, verified achievement tiers, and a clean map view highlighting nearby active bin nodes. Bins are clearly marked with a red indicator if they hit critical capacities or require maintenance.
2. **AI Chat Assistant ("SilaTanya AI"):** An embedded conversational interface powered by Google Gemini. The assistant uses specific system instructions to answer *only* waste classification or recycling queries, turning local Malaysian slang into accurate sorting advice.
3. **Aduan Portal (Incident Logger):** Allows users to capture real-time images, log description metadata, and automatically drop a geographic pin to report illegal dumping hotspots or damaged infrastructure.
4. **Tips Hub:** Feeds the latest public environmental announcements, sustainability articles, and sorting guidelines published by the Selangor State Government.

---

## 6. Multi-Agent Software Core & Processing Architecture

The backend infrastructure utilizes three specialized agents to process data streams and prevent logistical delays:
* **Reconnaissance Agent:** Manages initial data ingestion. It strips out signal noise, reads incoming sensor telemetry (Volume, Moisture, System Voltage) broadcast by the ESP32 node, and standardizes the incoming metadata.
* **Analysis Agent:** Assesses infrastructure threat metrics. If a node registers an unexpected weight shift or if the battery voltage divider logs a power drop below **20%**, it changes the asset index status to `KRITIKAL` or `MAINTENANCE REQUIRED`.
* **Orchestration Agent:** Directs logistical outputs. When a critical status is triggered, it instantly bypasses manual queues, overrides active compactor truck schedules on the PBT map panel, and issues an automated battery swap or collection ticket to the field crew.

---

## 7. Feasibility, Innovation Moat & Competitive Edge

* **Low Capital Cost & Risk:** Shifting electronics to a retrofitted, low-cost ESP32 cap module lowers production costs to ~RM50 per field unit. If a bin is stolen or damaged, the replacement cost to the council is negligible.
* **Active Contamination Prevention:** By keeping intake flaps locked until the smartphone camera vision verifies the trash item, it is impossible for users to drop liquids, unapproved objects, or hazardous items into recycling chambers.
* **Proactive Operational Model:** SmartTong removes the reporting lag common in current apps like iClean Selangor. By combining historical accumulation data with real-time sensor updates, it optimizes public cleaning logistics before public issues arise.

```