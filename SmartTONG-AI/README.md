# 🤖 SmartTONG AI Module

The SmartTONG AI module is responsible for classifying recyclable waste materials using a deep learning image classification model. The trained model will be deployed as a TensorFlow Lite (`.tflite`) model and integrated into the SmartTONG Flutter application for real-time camera and gallery image prediction.

---

# Objective

Classify waste into **5 recyclable categories**:

| Class | Description |
|---------|-------------|
| Plastic | Plastic bottles, containers, bags, etc. |
| Paper | Paper + Cardboard |
| Glass | Glass bottles, jars, etc. |
| Metal | Aluminium cans, steel cans, metal objects |
| General Waste | Non-recyclable waste |

The application should display:

- Predicted class
- Confidence percentage
- Recycling recommendation

---

# AI Technology Stack

| Component | Technology |
|------------|------------|
| Language | Python 3.11 |
| Framework | TensorFlow 2.x |
| Model | MobileNetV3 Small |
| Training | Transfer Learning |
| Deployment | TensorFlow Lite |
| Mobile App | Flutter |
| Image Processing | OpenCV |

---

# Project Structure

```
AI/

├── dataset/
│   ├── train/
│   ├── validation/
│   └── test/
│
├── models/
│
├── outputs/
│   ├── checkpoints/
│   ├── logs/
│   ├── plots/
│   └── best_model.keras
│
├── scripts/
│   ├── train.py
│   ├── evaluate.py
│   ├── predict.py
│   ├── convert_tflite.py
│   └── utils.py
│
├── requirements.txt
└── README.md
```

---

# Dataset

The dataset has already been manually organized into the following folders.

```
dataset/

train/
    plastic/
    paper/
    glass/
    metal/
    general_waste/

validation/
    plastic/
    paper/
    glass/
    metal/
    general_waste/

test/
    plastic/
    paper/
    glass/
    metal/
    general_waste/
```

Folder names are used as the class labels automatically by TensorFlow.

Dataset Split

- Training : 80%
- Validation : 10%
- Testing : 10%

---

# Training Pipeline

```
Dataset
        │
        ▼
Load Images
        │
        ▼
Resize (224 × 224)
        │
        ▼
Normalize Pixel Values
        │
        ▼
Data Augmentation
        │
        ▼
Transfer Learning
(MobileNetV3 Small)
        │
        ▼
Custom Classification Head
        │
        ▼
Train Model
        │
        ▼
Validation
        │
        ▼
Model Checkpoint
        │
        ▼
Fine Tuning
        │
        ▼
Evaluation
        │
        ▼
Export TensorFlow Model
        │
        ▼
Convert to TensorFlow Lite
        │
        ▼
Flutter Integration
```

---

# Data Preprocessing

Images are resized to:

```
224 × 224 pixels
```

Image preprocessing includes:

- Rescaling
- Random Rotation
- Random Zoom
- Random Translation
- Random Brightness
- Horizontal Flip

Data augmentation improves robustness against different camera angles and lighting conditions.

---

# Model Architecture

Base Model

```
MobileNetV3 Small
```

Transfer Learning

```
Image

↓

MobileNetV3 Small (Frozen)

↓

GlobalAveragePooling

↓

Dense (256)

↓

Dropout (0.3)

↓

Dense (5)

↓

Softmax
```

The final layer predicts five waste categories.

---

# Training Configuration

| Parameter | Value |
|------------|-------|
| Image Size | 224 × 224 |
| Batch Size | 32 |
| Epochs | 20 |
| Optimizer | Adam |
| Learning Rate | 0.0001 |
| Loss Function | Categorical Crossentropy |
| Metrics | Accuracy |

Callbacks:

- EarlyStopping
- ModelCheckpoint
- ReduceLROnPlateau

---

# Fine Tuning

After the initial training converges:

1. Unfreeze the final MobileNetV3 layers.
2. Continue training with a lower learning rate.
3. Save the best-performing model.

This improves prediction accuracy without overfitting.

---

# Evaluation

The trained model is evaluated using the unseen test dataset.

Evaluation metrics include:

- Test Accuracy
- Precision
- Recall
- F1 Score
- Confusion Matrix

Training artifacts generated:

```
outputs/

accuracy.png

loss.png

confusion_matrix.png

classification_report.txt
```

---

# Export

The trained model is exported as:

```
best_model.keras
```

Then converted into

```
best_model.tflite
```

The TensorFlow Lite model is used inside the Flutter application.

---

# Flutter Integration

```
Flutter

↓

Camera / Gallery

↓

TensorFlow Lite Model

↓

Prediction

↓

Plastic

96.42%

↓

Display Recycling Recommendation
```

Inference is performed entirely on-device.

No internet connection is required.

---

# Future Improvements

- Increase dataset diversity
- Add Organic Waste category
- Detect multiple waste objects
- Object detection using YOLOv8
- Automatic waste segmentation
- Quantized TensorFlow Lite model for faster inference