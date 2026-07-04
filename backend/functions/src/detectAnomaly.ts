/**
 * SmartTONG AI — detectAnomaly Cloud Function
 *
 * Firestore trigger: fires on every new reading written to /readings/{binId}
 * Creates alerts in /alerts/ collection and sends FCM push notifications.
 */

import * as functions from "firebase-functions/v2/firestore";
import * as admin from "firebase-admin";

const HAZARDOUS_LABEL  = "hazardous";
const GAS_THRESHOLD    = 300;
const CRITICAL_FILL    = 90;

export const detectAnomaly = functions.onDocumentUpdated(
  "readings/{binId}",
  async (event) => {
    const binId  = event.params.binId;
    const data   = event.data?.after.data();
    if (!data) return;

    const db  = admin.firestore();
    const now = admin.firestore.FieldValue.serverTimestamp();
    const alerts: Promise<any>[] = [];

    // ── 1. Hazardous waste detected ───────────────────────────────────────
    if (data.lastWasteType === HAZARDOUS_LABEL) {
      console.log(`[detectAnomaly] HAZARDOUS waste in bin ${binId}`);
      alerts.push(
        db.collection("alerts").add({
          binId,
          type:      "HAZARDOUS",
          severity:  "critical",
          message:   `Bahan berbahaya dikesan di tong ${binId}. Tindakan segera diperlukan.`,
          fillPercent: data.fillPercent,
          resolved:  false,
          createdAt: now,
        })
      );
    }

    // ── 2. Bin critically full (> 90%) ────────────────────────────────────
    if (data.fillPercent >= CRITICAL_FILL) {
      console.log(`[detectAnomaly] Bin ${binId} critically full: ${data.fillPercent}%`);
      alerts.push(
        db.collection("alerts").add({
          binId,
          type:      "FULL",
          severity:  "high",
          message:   `Tong ${binId} penuh ${data.fillPercent}%. Perlu dikosongkan segera.`,
          fillPercent: data.fillPercent,
          resolved:  false,
          createdAt: now,
        })
      );
    }

    // ── 3. High gas level (possible decomposition / illegal dumping) ──────
    if (data.gasLevel > GAS_THRESHOLD && data.fillPercent < 30) {
      console.log(`[detectAnomaly] Gas alert in ${binId}: gas=${data.gasLevel}, fill=${data.fillPercent}%`);
      alerts.push(
        db.collection("alerts").add({
          binId,
          type:      "ILLEGAL_DUMP",
          severity:  "medium",
          message:   `Bau mencurigakan dikesan di tong ${binId} (pengisian rendah, gas tinggi). Mungkin pembuangan haram.`,
          fillPercent: data.fillPercent,
          gasLevel:    data.gasLevel,
          resolved:    false,
          createdAt:   now,
        })
      );
    }

    // ── 4. Send FCM push to PBT dashboard topic ───────────────────────────
    if (alerts.length > 0) {
      await Promise.all(alerts);
      try {
        await admin.messaging().send({
          topic:        "pbt-alerts",
          notification: {
            title: `SmartTONG AI — Amaran Tong ${binId}`,
            body:  `Tong ${binId}: Isi ${data.fillPercent}% | Jenis: ${data.lastWasteType}`,
          },
          data: {
            binId,
            fillPercent: String(data.fillPercent),
            type:        data.lastWasteType,
          },
        });
      } catch (err) {
        console.warn("[detectAnomaly] FCM send failed (normal if no FCM token registered):", err);
      }
    }
  }
);
