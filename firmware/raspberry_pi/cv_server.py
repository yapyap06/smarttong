#!/usr/bin/env python3
"""
SmartTONG AI — cv_server.py  (Phase 5)
Raspberry Pi Flask Vision Server

Runs a lightweight HTTP server on the Pi that accepts JPEG images
and returns Gemini Vision waste classification results.

The ESP32 triggers classification by POSTing to:
  POST http://{PI_LOCAL_IP}:5000/classify

Hardware:
  - Raspberry Pi Zero 2W / Pi 4 / Pi 5
  - Pi Camera Module 3 (or USB webcam via OpenCV)

Setup:
  pip install flask requests picamera2 pillow

Usage:
  python3 cv_server.py --gemini-key YOUR_KEY --port 5000

For systemd auto-start, create /etc/systemd/system/smarttong-cv.service
"""

import argparse
import base64
import io
import json
import logging
import os
import time
from threading import Thread

try:
    from flask import Flask, jsonify, request
    from PIL import Image
    import requests
except ImportError:
    print("ERROR: Missing dependencies. Run: pip install flask requests pillow")
    raise

# ── Optional: Pi Camera (fallback to no camera if not available) ─────────────
try:
    from picamera2 import Picamera2
    PICAMERA_AVAILABLE = True
except ImportError:
    PICAMERA_AVAILABLE = False

# ─── Flask app ────────────────────────────────────────────────────────────────
app = Flask(__name__)
logging.basicConfig(level=logging.INFO, format="[%(levelname)s] %(message)s")
log = logging.getLogger(__name__)

GEMINI_API_URL = (
    "https://generativelanguage.googleapis.com/v1beta/"
    "models/gemini-1.5-flash:generateContent"
)

SYSTEM_PROMPT = """Anda adalah sistem klasifikasi sisa pintar SmartTong.
Analisa gambar dan kembalikan JSON:
{
  "label": "nama item Bahasa Malaysia",
  "category": "PLASTIK|KERTAS|LOGAM|KACA|SISA_BAKI|BAHAYA",
  "accepted": true/false,
  "slot": "PLASTIK|KERTAS|LOGAM|KACA|SISA_BAKI|E_SISA_HUB",
  "confidence": 0.0-1.0,
  "reason": "penjelasan ringkas (max 15 kata)",
  "isHazardous": true/false
}
Balas HANYA JSON, tiada teks lain."""

GEMINI_API_KEY = os.environ.get("GEMINI_API_KEY", "")

# ─── Pi Camera singleton ──────────────────────────────────────────────────────
_camera = None

def get_camera():
    global _camera
    if _camera is None and PICAMERA_AVAILABLE:
        try:
            _camera = Picamera2()
            config = _camera.create_still_configuration(
                main={"size": (640, 480)},
                lossless=False,
            )
            _camera.configure(config)
            _camera.start()
            time.sleep(1)  # Warm-up
            log.info("Pi Camera initialised.")
        except Exception as e:
            log.warning(f"Pi Camera init failed: {e}")
            _camera = None
    return _camera


def capture_jpeg() -> bytes | None:
    """Capture a JPEG from the Pi Camera."""
    cam = get_camera()
    if cam is None:
        return None
    stream = io.BytesIO()
    cam.capture_file(stream, format="jpeg")
    return stream.getvalue()


def image_to_base64(jpeg_bytes: bytes) -> str:
    return base64.b64encode(jpeg_bytes).decode("utf-8")


def call_gemini(image_b64: str, mime_type: str = "image/jpeg") -> dict:
    """Call Gemini Vision API and return parsed classification dict."""
    if not GEMINI_API_KEY:
        return {
            "label": "API key not set",
            "category": "SISA_BAKI",
            "accepted": False,
            "slot": "SISA_BAKI",
            "confidence": 0.0,
            "reason": "GEMINI_API_KEY environment variable tidak ditetapkan",
            "isHazardous": False,
        }

    body = {
        "systemInstruction": {"parts": [{"text": SYSTEM_PROMPT}]},
        "contents": [
            {
                "parts": [
                    {"text": "Klasifikasikan sisa dalam gambar ini:"},
                    {"inlineData": {"mimeType": mime_type, "data": image_b64}},
                ]
            }
        ],
        "generationConfig": {
            "temperature": 0.1,
            "maxOutputTokens": 256,
            "responseMimeType": "application/json",
        },
    }

    resp = requests.post(
        f"{GEMINI_API_URL}?key={GEMINI_API_KEY}",
        headers={"Content-Type": "application/json"},
        json=body,
        timeout=20,
    )
    resp.raise_for_status()

    raw = resp.json()
    text = raw["candidates"][0]["content"]["parts"][0]["text"]
    return json.loads(text)


# ─── Routes ───────────────────────────────────────────────────────────────────

@app.route("/health")
def health():
    """Health check — useful for ESP32 to verify Pi is reachable."""
    return jsonify({
        "status": "ok",
        "camera": PICAMERA_AVAILABLE and get_camera() is not None,
        "geminiKey": bool(GEMINI_API_KEY),
        "server": "SmartTONG cv_server v1.0",
    })


@app.route("/classify", methods=["POST"])
def classify():
    """
    POST /classify
    Accepts either:
      a) JSON body: { "imageBase64": "<b64>", "mimeType": "image/jpeg" }
      b) Multipart: image file upload as 'image' field
      c) No body: captures from Pi Camera directly
    Returns classification JSON.
    """
    try:
        mime_type = "image/jpeg"
        image_b64: str | None = None

        # ── Option A: base64 in JSON body (from ESP32 HTTP POST) ─────────
        if request.content_type and "application/json" in request.content_type:
            body = request.get_json(silent=True) or {}
            image_b64 = body.get("imageBase64")
            mime_type = body.get("mimeType", "image/jpeg")

        # ── Option B: multipart file upload ──────────────────────────────
        elif "multipart" in (request.content_type or ""):
            file = request.files.get("image")
            if file:
                jpeg_bytes = file.read()
                image_b64 = image_to_base64(jpeg_bytes)
                mime_type = file.content_type or "image/jpeg"

        # ── Option C: capture from Pi Camera ─────────────────────────────
        if image_b64 is None:
            jpeg_bytes = capture_jpeg()
            if jpeg_bytes is None:
                return jsonify({"error": "No image provided and Pi Camera unavailable"}), 400
            image_b64 = image_to_base64(jpeg_bytes)
            log.info("Image captured from Pi Camera.")

        log.info(f"Classifying image ({mime_type}, {len(image_b64)} b64 chars)...")
        result = call_gemini(image_b64, mime_type)
        log.info(f"Result: {result.get('label')} → {result.get('category')} "
                 f"(accepted={result.get('accepted')})")

        return jsonify({"success": True, **result, "server": "pi_cv_server"})

    except json.JSONDecodeError as e:
        log.error(f"Gemini response parse error: {e}")
        return jsonify({"error": "Failed to parse Gemini response", "detail": str(e)}), 502
    except requests.HTTPError as e:
        log.error(f"Gemini HTTP error: {e}")
        return jsonify({"error": "Gemini API error", "detail": str(e)}), 502
    except Exception as e:
        log.error(f"Unexpected error: {e}")
        return jsonify({"error": "Internal server error", "detail": str(e)}), 500


@app.route("/capture")
def capture_preview():
    """GET /capture — returns a JPEG preview image (for debugging)."""
    jpeg_bytes = capture_jpeg()
    if jpeg_bytes is None:
        return jsonify({"error": "Camera not available"}), 503
    from flask import Response
    return Response(jpeg_bytes, mimetype="image/jpeg")


# ─── Entry point ──────────────────────────────────────────────────────────────
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="SmartTONG Pi CV Server")
    parser.add_argument("--gemini-key", default="", help="Gemini API key")
    parser.add_argument("--port", type=int, default=5000, help="Server port (default: 5000)")
    parser.add_argument("--host", default="0.0.0.0", help="Bind host (default: 0.0.0.0)")
    args = parser.parse_args()

    if args.gemini_key:
        GEMINI_API_KEY = args.gemini_key
        os.environ["GEMINI_API_KEY"] = args.gemini_key

    log.info(f"SmartTONG CV Server starting on {args.host}:{args.port}")
    log.info(f"Pi Camera: {'available' if PICAMERA_AVAILABLE else 'NOT available'}")
    log.info(f"Gemini key: {'SET' if GEMINI_API_KEY else 'NOT SET — set GEMINI_API_KEY env'}")

    # Pre-warm camera in background
    if PICAMERA_AVAILABLE:
        Thread(target=get_camera, daemon=True).start()

    app.run(host=args.host, port=args.port, debug=False, threaded=True)
