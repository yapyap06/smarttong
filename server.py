"""
SmartTONG AI — Cloud Backend Server for Render
================================================
Serves both /predict (Cam Scanner) and /chat (Chat Assistant) endpoints on port PORT.
"""

import os
import json
import base64
import io
import numpy as np
from PIL import Image, ImageOps
from flask import Flask, request, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

# ── Load Model (Relative Paths) ──────────────────────────────────────────────
MODEL = None
CLASS_NAMES = ["Cardboard", "Glass", "Metal", "Paper", "Plastic", "Trash"]
BIN_MAP = {
    "Cardboard": {"bin": "Blue Recycling Bin (Kertas/Kadbod)", "color": "#1976D2", "tip": "Remove tape and keep cardboard dry before recycling."},
    "Paper":     {"bin": "Blue Recycling Bin (Kertas/Kadbod)", "color": "#1976D2", "tip": "Recycle clean paper only. Avoid wet or oily paper."},
    "Glass":     {"bin": "Brown Recycling Bin (Kaca)",         "color": "#795548", "tip": "Rinse glass containers before disposal."},
    "Plastic":   {"bin": "Orange Recycling Bin (Plastik)",     "color": "#FB8C00", "tip": "Empty bottles and flatten them to save space."},
    "Metal":     {"bin": "Orange Recycling Bin (Logam)",       "color": "#EF6C00", "tip": "Rinse cans before placing them in the recycling bin."},
    "Trash":     {"bin": "Black Bin (Sisa Baki)",              "color": "#424242", "tip": "Dispose of non-recyclable waste responsibly."},
}

def load_keras_model():
    global MODEL
    if MODEL is not None:
        return MODEL
    try:
        from tensorflow.keras.models import load_model
        base_dir = os.path.dirname(__file__)
        path1 = os.path.join(base_dir, "SmartTONG-AI", "models", "best_model_finetuned224.keras")
        path2 = os.path.join(base_dir, "SmartTONG-AI", "models", "best_model224.keras")
        model_path = path1 if os.path.exists(path1) else path2
        if os.path.exists(model_path):
            print(f"[SmartTONG Cloud] Loading TensorFlow model from: {model_path}")
            MODEL = load_model(model_path)
            print("[SmartTONG Cloud] Keras Model ready!")
            return MODEL
    except Exception as e:
        print(f"[SmartTONG Cloud] Keras model load notice: {e}")
    return None

# Attempt early load
load_keras_model()

# ── Health Check Endpoint ───────────────────────────────────────────────────
@app.route('/', methods=['GET'])
@app.route('/health', methods=['GET'])
def health():
    return jsonify({
        "status": "online",
        "service": "SmartTONG AI Cloud Backend",
        "model_loaded": MODEL is not None
    })

# ── Prediction Endpoint (/predict) ──────────────────────────────────────────
@app.route('/predict', methods=['POST', 'OPTIONS'])
def predict():
    if request.method == 'OPTIONS':
        return '', 200

    try:
        data = request.get_json(force=True) or {}
        img_b64 = data.get("image", "")
        if not img_b64:
            return jsonify({"error": "No image provided"}), 400

        if "," in img_b64:
            img_b64 = img_b64.split(",", 1)[1]

        img_bytes = base64.b64decode(img_b64)
        img = Image.open(io.BytesIO(img_bytes)).convert("RGB")
        img = ImageOps.exif_transpose(img)

        # 1. Try TensorFlow Keras Model if loaded
        m = load_keras_model()
        if m is not None:
            resized = img.resize((224, 224))
            arr = np.array(resized, dtype=np.float32)
            arr = np.expand_dims(arr, axis=0)
            preds = m.predict(arr, verbose=0)[0]
            idx = int(np.argmax(preds))
            mat = CLASS_NAMES[idx]
            conf = round(float(preds[idx]) * 100, 2)

            return jsonify({
                "class": mat,
                "confidence": conf,
                "bin": BIN_MAP[mat]["bin"],
                "color": BIN_MAP[mat]["color"],
                "tip": BIN_MAP[mat]["tip"],
                "all_probabilities": {
                    CLASS_NAMES[i]: round(float(preds[i]) * 100, 2)
                    for i in range(len(CLASS_NAMES))
                }
            })

        # 2. Gemini Vision AI Fallback — reads key from GEMINI_API_KEY env var (works on Render)
        import urllib.request
        import re

        gemini_key = os.environ.get("GEMINI_API_KEY", "").strip()
        if not gemini_key:
            # Try obfuscated built-in key as last resort
            try:
                import base64 as _b64
                gemini_key = _b64.b64decode('QVEuQWI0Uk40Smx0bTE2NmJsSG9nQ1RmVnlza09QaTYyYnZfQXVoWkxjandDWlRja21SaWc=').decode('utf-8')
            except Exception:
                pass

        if gemini_key:
            try:
                vision_url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key={gemini_key}"
                prompt_text = (
                    "You are a waste classification AI for SmartTONG Selangor recycling bins. "
                    "Analyse the image and classify the waste item into EXACTLY ONE of these categories: "
                    "Cardboard, Glass, Metal, Paper, Plastic, Trash. "
                    "Respond with valid JSON ONLY, no markdown, no explanation: "
                    '{"class": "CATEGORY", "confidence": 87.3, "bin": "BIN_NAME", "color": "#HEX", "tip": "SHORT_TIP"}. '
                    "Bin/color mapping: Cardboard=Blue Recycling Bin (Kertas/Kadbod)/#1976D2, "
                    "Paper=Blue Recycling Bin (Kertas/Kadbod)/#1976D2, Glass=Brown Recycling Bin (Kaca)/#795548, "
                    "Metal=Orange Recycling Bin (Logam)/#EF6C00, Plastic=Orange Recycling Bin (Plastik)/#FB8C00, "
                    "Trash=Black Bin (Sisa Baki)/#424242."
                )
                payload = {
                    "contents": [{
                        "parts": [
                            {"text": prompt_text},
                            {"inlineData": {"mimeType": "image/jpeg", "data": img_b64}}
                        ]
                    }],
                    "generationConfig": {"temperature": 0.1, "maxOutputTokens": 256}
                }
                req = urllib.request.Request(
                    vision_url,
                    data=json.dumps(payload).encode('utf-8'),
                    headers={'Content-Type': 'application/json'}
                )
                with urllib.request.urlopen(req, timeout=20) as resp:
                    res_data = json.loads(resp.read().decode('utf-8'))
                    text = (res_data.get('candidates', [{}])[0]
                            .get('content', {}).get('parts', [{}])[0].get('text', ''))
                    json_match = re.search(r'\{[\s\S]*?\}', text)
                    if json_match:
                        parsed = json.loads(json_match.group(0))
                        cat = parsed.get("class", "Trash")
                        if cat not in BIN_MAP:
                            cat = "Trash"
                        return jsonify({
                            "class": cat,
                            "confidence": float(parsed.get("confidence", 85.0)),
                            "bin": BIN_MAP[cat]["bin"],
                            "color": BIN_MAP[cat]["color"],
                            "tip": parsed.get("tip", BIN_MAP[cat]["tip"]),
                            "source": "gemini-vision"
                        })
            except Exception as err:
                print(f"[SmartTONG] Gemini Vision fallback error: {err}")

        # 3. No model and no Gemini key — return explicit error so the client knows
        return jsonify({
            "error": "AI model not loaded and no Gemini API key set. Set GEMINI_API_KEY env var on Render.",
            "model_loaded": False
        }), 503

    except Exception as e:
        print(f"[Predict Error] {e}")
        return jsonify({"error": str(e)}), 500

# ── Chat Assistant Endpoint (/chat) ─────────────────────────────────────────
@app.route('/chat', methods=['POST', 'OPTIONS'])
def chat():
    if request.method == 'OPTIONS':
        return '', 200

    try:
        data = request.get_json(force=True) or {}
        user_msg = data.get('message', '').strip()
        if not user_msg:
            return jsonify({'error': 'No message provided'}), 400

        # Run chat_agent module logic if available
        try:
            from backend.chat_agent import chat_with_silatanya
            reply = chat_with_silatanya(user_msg)
            if reply and reply.strip():
                return jsonify({'reply': reply})
        except Exception as e:
            print(f"[Chat Agent Module Error] {e}")

        # Direct fallback response if chat_agent fails
        msg = user_msg.lower()
        if 'milo' in msg or 'tin' in msg or 'logam' in msg:
            fallback = "Tin Milo atau tin aluminium diperbuat daripada **logam**. Sila bilas tin sehingga bersih dan buang ke dalam **Slot Logam (Warna Jingga)** di tong SmartTONG atau mana-mana tong kitar semula logam."
        elif 'plastik' in msg or 'botol' in msg:
            fallback = "Botol atau bekas plastik perlu dikosongkan dan dibilas sebelum dimasukkan ke dalam **Slot Plastik (Warna Jingga)**."
        else:
            fallback = f"Saya **SilaTanya AI** — pakar pengurusan sisa SmartTONG Selangor.\n\nSila tanya tentang cara kitar semula item, lokasi tong berdekatan, atau panduan aduan kebersihan!"

        return jsonify({'reply': fallback})

    except Exception as e:
        print(f"[Chat Error] {e}")
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 7860))
    print(f"[SmartTONG Cloud] Starting Flask server on port {port}...")
    app.run(host='0.0.0.0', port=port)
