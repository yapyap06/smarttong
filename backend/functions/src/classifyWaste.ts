/**
 * SmartTONG AI — classifyWaste.ts  (Phase 3)
 *
 * POST /classifyWaste
 * Body: { imageBase64: string, mimeType?: "image/jpeg" | "image/png" }
 *
 * Calls Gemini 1.5 Flash with an Acceptance Rule Matrix system prompt.
 * Returns: { label, category, accepted, slot, confidence, reason, isHazardous }
 *
 * Acceptance Rule Matrix:
 *   DITERIMA (accepted):
 *     PLASTIK  — botol PET/HDPE bersih, bekas plastik keras kering
 *     KERTAS   — surat khabar, kadbod kering, kertas pejabat
 *     LOGAM    — tin aluminium, tin sardin dibilas, foil aluminium bersih
 *     KACA     — botol kaca (hantar ke pusat kitar semula kaca)
 *
 *   DITOLAK / SISA BAKI:
 *     Kotak tapau berlemak, tisu, lampin, sisa makanan, komposit
 *
 *   BAHAYA (hazardous):
 *     Bateri, ubat-ubatan, bahan kimia, cat, lampu neon, e-sisa
 */

import * as functions from "firebase-functions/v2/https";
import * as admin from "firebase-admin";

const GEMINI_API_URL =
  "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent";

const SYSTEM_PROMPT = `Anda adalah sistem klasifikasi sisa pintar SmartTong.
Analisa gambar yang diberikan dan tentukan jenis sisa mengikut Matriks Penerimaan SmartTong.

Matriks Penerimaan SmartTong Selangor:
KATEGORI DITERIMA (accepted: true):
  - PLASTIK: Botol PET/HDPE bersih, beg plastik bersih, bekas plastik keras kering
  - KERTAS: Surat khabar, majalah, kadbod kering, kertas pejabat, sampul surat
  - LOGAM: Tin aluminium, tin sardin dibilas, foil aluminium bersih
  - KACA: Botol kaca bersih (hantar ke pusat kitar kaca)

KATEGORI DITOLAK - SISA BAKI (accepted: false, hazardous: false):
  - Kotak tapau berlemak, kotak pizza, tisu, serbet, lampin pakai buang
  - Sisa makanan, bahan organik, plastik berlapis komposit
  - Straw, penyedut minuman, pembungkus gula-gula

KATEGORI BAHAYA (accepted: false, hazardous: true):
  - Bateri (AA, AAA, lithium, kenderaan)
  - E-sisa (telefon bimbit, komputer, elektronik)
  - Ubat-ubatan, bahan kimia, cat, pelarut
  - Lampu neon/CFL, lampu LED lama

Balas HANYA dalam format JSON ini (tiada teks lain):
{
  "label": "nama item dalam Bahasa Malaysia",
  "category": "PLASTIK|KERTAS|LOGAM|KACA|SISA_BAKI|BAHAYA",
  "accepted": true/false,
  "slot": "PLASTIK|KERTAS|LOGAM|KACA|SISA_BAKI|E_SISA_HUB",
  "confidence": 0.0-1.0,
  "reason": "penjelasan ringkas dalam Bahasa Malaysia (max 15 kata)",
  "isHazardous": true/false
}`;

interface ClassifyResponse {
  label: string;
  category: string;
  accepted: boolean;
  slot: string;
  confidence: number;
  reason: string;
  isHazardous: boolean;
}

export const classifyWaste = functions.onRequest(
  { secrets: ["GEMINI_API_KEY"], timeoutSeconds: 30 },
  async (req, res) => {
    res.set("Access-Control-Allow-Origin", "*");
    if (req.method === "OPTIONS") {
      res.set("Access-Control-Allow-Methods", "POST");
      res.set("Access-Control-Allow-Headers", "Content-Type");
      res.status(204).send("");
      return;
    }

    if (req.method !== "POST") {
      res.status(405).json({ error: "POST only" });
      return;
    }

    const { imageBase64, mimeType = "image/jpeg", binId } = req.body as {
      imageBase64: string;
      mimeType?: string;
      binId?: string;
    };

    if (!imageBase64) {
      res.status(400).json({ error: "imageBase64 required" });
      return;
    }

    const apiKey = process.env.GEMINI_API_KEY;
    if (!apiKey) {
      res.status(500).json({ error: "GEMINI_API_KEY secret not set" });
      return;
    }

    try {
      // ── Call Gemini Vision ────────────────────────────────────────────
      const geminiRes = await fetch(`${GEMINI_API_URL}?key=${apiKey}`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          systemInstruction: { parts: [{ text: SYSTEM_PROMPT }] },
          contents: [
            {
              parts: [
                { text: "Klasifikasikan sisa dalam gambar ini:" },
                {
                  inlineData: {
                    mimeType,
                    data: imageBase64,
                  },
                },
              ],
            },
          ],
          generationConfig: {
            temperature: 0.1,
            maxOutputTokens: 256,
            responseMimeType: "application/json",
          },
        }),
      });

      if (!geminiRes.ok) {
        const errText = await geminiRes.text();
        throw new Error(`Gemini API error ${geminiRes.status}: ${errText}`);
      }

      const geminiData = await geminiRes.json() as any;
      const rawText: string =
        geminiData?.candidates?.[0]?.content?.parts?.[0]?.text ?? "{}";

      let result: ClassifyResponse;
      try {
        result = JSON.parse(rawText);
      } catch {
        // Fallback if model doesn't return clean JSON
        result = {
          label: "Tidak dapat dikenalpasti",
          category: "SISA_BAKI",
          accepted: false,
          slot: "SISA_BAKI",
          confidence: 0.3,
          reason: "Gambar tidak jelas atau item tidak dikenali",
          isHazardous: false,
        };
      }

      // ── Log classification to Firestore ───────────────────────────────
      const db = admin.firestore();
      await db.collection("classifications").add({
        binId: binId ?? "UNKNOWN",
        ...result,
        classifiedAt: admin.firestore.FieldValue.serverTimestamp(),
      });

      // ── If hazardous: auto-create an alert ────────────────────────────
      if (result.isHazardous && binId) {
        await db.collection("alerts").add({
          binId,
          type: "HAZARDOUS_SCAN",
          severity: "critical",
          message: `Bahan berbahaya dikesan via CV scan di tong ${binId}: ${result.label}`,
          fillPercent: 0,
          resolved: false,
          createdAt: admin.firestore.FieldValue.serverTimestamp(),
        });
      }

      console.log(
        `[classifyWaste] ${result.label} → ${result.category} (accepted:${result.accepted})`
      );

      res.json({
        success: true,
        ...result,
        classifiedAt: new Date().toISOString(),
      });
    } catch (err) {
      console.error("[classifyWaste] Error:", err);
      res.status(500).json({ error: "Classification failed", details: String(err) });
    }
  }
);
