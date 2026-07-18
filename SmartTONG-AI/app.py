import os
import json
import numpy as np
import gradio as gr
from tensorflow.keras.models import load_model
from PIL import Image

# ==========================================================
# Load Model
# ==========================================================

MODEL_PATH = "C:/Users/Dell/Downloads/SmartTong/SmartTONG-AI/models/best_model_finetuned224.keras"

if not os.path.exists(MODEL_PATH):
    MODEL_PATH = "C:/Users/Dell/Downloads/SmartTong/SmartTONG-AI/models/best_model224.keras"

print(f"Loading model from: {MODEL_PATH}")
model = load_model(MODEL_PATH)

# ==========================================================
# Classes & Bin Mapping
# ==========================================================

class_names = ["Cardboard", "Glass", "Metal", "Paper", "Plastic", "Trash"]

bin_mapping = {
    "Cardboard": {"bin": "Blue Recycling Bin (Kertas/Kadbod)", "category": "Paper & Cardboard", "color": "#1976D2", "tip": "Remove tape and keep cardboard dry before recycling."},
    "Paper":     {"bin": "Blue Recycling Bin (Kertas/Kadbod)", "category": "Paper & Cardboard", "color": "#1976D2", "tip": "Recycle clean paper only. Avoid wet or oily paper."},
    "Glass":     {"bin": "Brown Recycling Bin (Kaca)",         "category": "Glass",             "color": "#795548", "tip": "Rinse glass containers before disposal."},
    "Plastic":   {"bin": "Orange Recycling Bin (Plastik)",     "category": "Plastic",           "color": "#FB8C00", "tip": "Empty bottles and flatten them to save space."},
    "Metal":     {"bin": "Orange Recycling Bin (Logam)",       "category": "Metal",             "color": "#EF6C00", "tip": "Rinse cans before placing them in the recycling bin."},
    "Trash":     {"bin": "Black Bin (Sisa Baki)",              "category": "General Waste",     "color": "#424242", "tip": "Dispose of non-recyclable waste responsibly."},
}

# ==========================================================
# Core Prediction (shared by both UI and JSON API)
# ==========================================================

def run_prediction(img):
    """Run model inference. img must be a PIL Image."""
    img = img.convert("RGB").resize((224, 224))
    img_array = np.array(img, dtype=np.float32)
    img_array = np.expand_dims(img_array, axis=0)
    prediction = model.predict(img_array, verbose=0)[0]
    idx = int(np.argmax(prediction))
    confidence = float(prediction[idx])
    material = class_names[idx]
    all_probs = {class_names[i]: float(prediction[i]) for i in range(len(class_names))}
    return material, confidence, all_probs

# ==========================================================
# Gradio UI Function
# ==========================================================

def classify_image(img):
    try:
        if img is None:
            return "<h3 style='color:red'>Please upload an image.</h3>"
        material, confidence, _ = run_prediction(img)
        info = bin_mapping[material]
        return f"""
<div style="font-family:Arial;background:#F8F9FA;padding:30px;border-radius:20px;max-width:700px;margin:auto;text-align:center;border:1px solid #DDD;">
  <h1 style="color:#2E7D32;">SmartTONG AI</h1><hr>
  <h2 style="color:#666;">Detected Material</h2>
  <h1 style="font-size:42px;color:{info['color']};">{material}</h1>
  <p style="font-size:22px;color:#666;">Confidence</p>
  <h2 style="color:#2E7D32;">{confidence:.2%}</h2><hr>
  <h2>Recommended Disposal</h2>
  <div style="background:{info['color']};padding:18px;border-radius:15px;display:inline-block;color:white;font-size:24px;font-weight:bold;">{info['bin']}</div>
  <br><br><h3>Category</h3><p style="font-size:22px;">{info['category']}</p><hr>
  <h3>Recycling Tip</h3><p style="font-size:18px;line-height:1.7;">{info['tip']}</p>
</div>"""
    except Exception as e:
        return f"<h3 style='color:red'>Error: {e}</h3>"

# ==========================================================
# JSON API Function (called by dashboard scanner via Gradio API)
# ==========================================================

def predict_api(img):
    """Returns JSON string for the dashboard scanner to consume."""
    try:
        if img is None:
            return json.dumps({"error": "No image provided"})
        if isinstance(img, np.ndarray):
            img = Image.fromarray(img.astype('uint8'))
        material, confidence, all_probs = run_prediction(img)
        info = bin_mapping[material]
        return json.dumps({
            "class": material,
            "confidence": round(confidence * 100, 2),
            "bin": info["bin"],
            "category": info["category"],
            "color": info["color"],
            "tip": info["tip"],
            "all_probabilities": {k: round(v * 100, 2) for k, v in all_probs.items()}
        })
    except Exception as e:
        return json.dumps({"error": str(e)})

# ==========================================================
# Gradio App: Webcam UI + Upload + JSON API tab
# ==========================================================

with gr.Blocks(title="SmartTONG AI Waste Classifier") as app:

    gr.Markdown("# SmartTONG AI Waste Classifier")
    gr.Markdown("Powered by EfficientNetV2 · TensorFlow · SmartTONG AINS 2026")

    with gr.Tab("Live Webcam"):
        gr.Interface(
            fn=classify_image,
            inputs=gr.Image(type="pil", sources=["webcam"], streaming=True),
            outputs=gr.HTML(),
            live=True,
        )

    with gr.Tab("Upload & Analyse"):
        with gr.Row():
            upload_in = gr.Image(type="pil", label="Upload Image", sources=["upload"])
            upload_out = gr.HTML(label="Result")
        gr.Button("Classify").click(fn=classify_image, inputs=upload_in, outputs=upload_out)

    with gr.Tab("JSON API (Dashboard Scanner)"):
        gr.Markdown("### Dashboard Scanner API\nThe SmartTong dashboard scanner sends images here and reads back structured JSON.")
        api_img  = gr.Image(type="numpy", label="Image Input", sources=["upload"])
        api_out  = gr.Textbox(label="JSON Response", lines=12)
        gr.Button("Predict (JSON)").click(fn=predict_api, inputs=api_img, outputs=api_out, api_name="predict")

if __name__ == "__main__":
    app.launch(share=True)