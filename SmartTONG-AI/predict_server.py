"""
SmartTong AI — Standalone Prediction Server
============================================
Run this INSTEAD OF app.py when using the dashboard scanner:

    python predict_server.py

Then open dashboard/index.html and use the Imbas (Scanner) button.
Server runs on http://127.0.0.1:7862
"""

import json
import base64
import io
import os
import numpy as np
from http.server import HTTPServer, BaseHTTPRequestHandler
from PIL import Image
from tensorflow.keras.models import load_model

# ── Model ────────────────────────────────────────────────────────────────────
MODEL_PATH = "models/best_model_finetuned224.keras"
if not os.path.exists(MODEL_PATH):
    MODEL_PATH = "models/best_model224.keras"

print(f"\n[SmartTong] Loading model from: {MODEL_PATH}")
model = load_model(MODEL_PATH)
print("[SmartTong] Model ready!\n")

CLASS_NAMES = ["Cardboard", "Glass", "Metal", "Paper", "Plastic", "Trash"]

BIN_MAP = {
    "Cardboard": {"bin": "Blue Recycling Bin (Kertas/Kadbod)", "color": "#1976D2", "tip": "Remove tape and keep cardboard dry before recycling."},
    "Paper":     {"bin": "Blue Recycling Bin (Kertas/Kadbod)", "color": "#1976D2", "tip": "Recycle clean paper only. Avoid wet or oily paper."},
    "Glass":     {"bin": "Brown Recycling Bin (Kaca)",         "color": "#795548", "tip": "Rinse glass containers before disposal."},
    "Plastic":   {"bin": "Orange Recycling Bin (Plastik)",     "color": "#FB8C00", "tip": "Empty bottles and flatten them to save space."},
    "Metal":     {"bin": "Orange Recycling Bin (Logam)",       "color": "#EF6C00", "tip": "Rinse cans before placing them in the recycling bin."},
    "Trash":     {"bin": "Black Bin (Sisa Baki)",              "color": "#424242", "tip": "Dispose of non-recyclable waste responsibly."},
}

PORT = 7862


# ── HTTP Handler ─────────────────────────────────────────────────────────────
class PredictHandler(BaseHTTPRequestHandler):

    # ── CORS preflight ──────────────────────────────────────────────────────
    def do_OPTIONS(self):
        self.send_response(200)
        self._cors()
        self.end_headers()

    # ── Prediction endpoint ─────────────────────────────────────────────────
    def do_POST(self):
        if self.path != "/predict":
            self.send_response(404)
            self.end_headers()
            return

        try:
            length = int(self.headers.get("Content-Length", 0))
            body = json.loads(self.rfile.read(length))
            img_b64 = body.get("image", "")

            # Strip data-URL prefix (e.g. "data:image/jpeg;base64,...")
            if "," in img_b64:
                img_b64 = img_b64.split(",", 1)[1]

            img_bytes = base64.b64decode(img_b64)
            img = Image.open(io.BytesIO(img_bytes)).convert("RGB")
            
            # CRITICAL: Apply EXIF orientation natively
            from PIL import ImageOps
            img = ImageOps.exif_transpose(img)
            
            # Aspect-Preserving Center-Square Crop to match 224x224 training geometry
            w, h = img.size
            min_dim = min(w, h)
            left = (w - min_dim) // 2
            top = (h - min_dim) // 2
            cropped = img.crop((left, top, left + min_dim, top + min_dim))
            resized = cropped.resize((224, 224))

            arr = np.array(resized, dtype=np.float32)
            arr = np.expand_dims(arr, axis=0)

            preds = model.predict(arr, verbose=0)[0]
            idx   = int(np.argmax(preds))
            mat   = CLASS_NAMES[idx]
            conf  = round(float(preds[idx]) * 100, 2)

            result = {
                "class":      mat,
                "confidence": conf,
                "bin":        BIN_MAP[mat]["bin"],
                "color":      BIN_MAP[mat]["color"],
                "tip":        BIN_MAP[mat]["tip"],
                "all_probabilities": {
                    CLASS_NAMES[i]: round(float(preds[i]) * 100, 2)
                    for i in range(len(CLASS_NAMES))
                }
            }
            print(f"[SmartTong] Predicted: {mat} ({conf:.1f}%)")
            self._json(200, result)

        except Exception as exc:
            print(f"[SmartTong] ERROR: {exc}")
            self._json(500, {"error": str(exc)})

    # ── Helpers ─────────────────────────────────────────────────────────────
    def _cors(self):
        self.send_header("Access-Control-Allow-Origin",  "*")
        self.send_header("Access-Control-Allow-Methods", "POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

    def _json(self, code, data):
        body = json.dumps(data).encode()
        self.send_response(code)
        self.send_header("Content-Type",   "application/json")
        self.send_header("Content-Length", str(len(body)))
        self._cors()
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):  # suppress default noisy logs
        pass


# ── Main ──────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    server = HTTPServer(("127.0.0.1", PORT), PredictHandler)
    print(f"[SmartTong] Prediction server running at http://127.0.0.1:{PORT}")
    print("[SmartTong] Keep this running while using the dashboard scanner.")
    print("[SmartTong] Press Ctrl+C to stop.\n")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[SmartTong] Server stopped.")
