# SmartTONG AI — Edge Impulse Waste Classifier Training Guide
**Train a custom image classification model for ESP32-CAM using Edge Impulse**

This guide walks you through collecting a dataset, training a lightweight MobileNet model, and exporting it as an Arduino library to enable edge-AI classification directly on the SmartTong device.

---

## 📅 Step 1: Set Up Your Project
1. Sign up for a free account at [Edge Impulse Studio](https://studio.edgeimpulse.com).
2. Create a new project named `SmartTong_Waste_Classifier`.
3. In the project dashboard, select **Developer Mode** and choose **Images** as your data type.

---

## 📷 Step 2: Data Collection
Collect at least **150-200 images per category** to ensure model robustness. Focus on the main classes defined in the SmartTong Acceptance Rule Matrix:

| Class Label | Examples to Photograph | Target Count |
|---|---|---|
| `plastic` | Mineral water bottles, clear plastic bags, plastic food containers | 200 images |
| `paper` | Cardboard boxes, old newspapers, notebook papers | 200 images |
| `metal` | Aluminium beverage cans (Milo, Coca-Cola), food tin cans | 200 images |
| `hazardous` | AA/AAA batteries, old mobile phones, electronics | 150 images |
| `trash` | Greasy food wrappers, used tissues, soiled cups | 150 images |

### How to capture:
- Use your phone camera or the ESP32-CAM via the Edge Impulse daemon.
- Capture from different angles, distances, and lighting conditions.
- Ensure the background matches the interior of the trash bin opening.
- Split the dataset: **80% Training**, **20% Testing** (Edge Impulse can auto-split this).

---

## 🧠 Step 3: Configure the Impulse
In the Edge Impulse Studio sidebar, click on **Impulse Design**:
1. **Input Block**: Add an **Image** block. Set the width and height to `96 x 96` pixels (optimal size to fit the ESP32's limited SRAM). Set color depth to **RGB** (or Grayscale to save memory).
2. **Processing Block**: Add an **Image** processing block. This extracts raw features from pixel data.
3. **Learning Block**: Add a **Transfer Learning (Images)** block. This uses a pre-trained MobileNet model optimized for microcontrollers.
4. Click **Save Impulse**.

---

## ⚙️ Step 4: Feature Generation & Training
1. Go to **Image** in the sidebar. Click **Save parameters**, then click **Generate features**.
   - Review the *Feature Explorer* cluster map. Ensure similar categories are clustered together. If classes overlap significantly, consider pruning bad images.
2. Go to **Transfer Learning** in the sidebar:
   - **Number of epochs**: `50`
   - **Learning rate**: `0.005`
   - **Model architecture**: Select **MobileNetV2 0.35** (or **MobileNetV1 0.25** for extreme low-power ESP32 variants).
   - Click **Start training**.
3. Target **Accuracy**: You should aim for **> 82% validation accuracy** before exporting.

---

## 📦 Step 5: Export as Arduino Library
1. Go to the **Deployment** tab in the sidebar.
2. Under "Search platforms", select **Arduino library**.
3. Under "Select options", choose **Quantized (int8)** for maximum performance and low RAM footprint.
4. Click **Build**.
5. Save the downloaded `.zip` file.

---

## 🔌 Step 6: Integrating into SmartTong Firmware
1. In the Arduino IDE, go to **Sketch** → **Include Library** → **Add .ZIP Library...** and select the file downloaded in Step 5.
2. Open `CameraAI.h` in your project folder.
3. Uncomment the configuration line:
   ```cpp
   #define USE_REAL_CLASSIFIER
   ```
4. In `CameraAI.cpp`, include the newly added Edge Impulse header:
   ```cpp
   #include <SmartTong_Waste_Classifier_inferencing.h>
   ```
5. Compile and flash the code to your ESP32-CAM board. The device will now classify items in real-time locally without requiring internet access.
