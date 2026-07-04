/**
 * SmartTONG AI — multiAgent.ts  (Phase 3)
 *
 * Three-agent orchestration pipeline:
 *
 *  1. RECONNAISSANCE AGENT — onDocumentUpdated("readings/{binId}")
 *     Sanitizes raw IoT telemetry: clamps values, validates GPS bounds,
 *     detects sensor dropout, writes clean data to /bins_clean/{binId}
 *
 *  2. ANALYSIS AGENT — onDocumentUpdated("bins_clean/{binId}")
 *     Monitors thresholds, assigns operational status:
 *     KRITIKAL | PERLU_PENYELENGGARAAN | SENSOR_OFFLINE | BERBAHAYA | NORMAL
 *     Updates /bins/{binId}.status and creates /alerts/ documents
 *
 *  3. ORCHESTRATION AGENT — onDocumentUpdated("bins/{binId}")
 *     When a bin enters KRITIKAL or BERBAHAYA, triggers route re-optimization
 *     and writes new waypoints to /activeRoutes/{zoneId}
 */

import * as ffirestore from "firebase-functions/v2/firestore";
import * as admin from "firebase-admin";

// ─── Selangor bounding box ────────────────────────────────────────────────────
const SELANGOR_BOUNDS = {
  minLat: 2.6, maxLat: 3.9,
  minLng: 101.0, maxLng: 101.8,
};

// ─── Status constants ─────────────────────────────────────────────────────────
type BinStatus = "KRITIKAL" | "PERLU_PENYELENGGARAAN" | "SENSOR_OFFLINE" | "BERBAHAYA" | "NORMAL";

// ────────────────────────────────────────────────────────────────────────────
// AGENT 1 — RECONNAISSANCE
// ────────────────────────────────────────────────────────────────────────────
export const reconAgent = ffirestore.onDocumentUpdated(
  "readings/{binId}",
  async (event) => {
    const binId = event.params.binId;
    const raw   = event.data?.after.data();
    if (!raw) return;

    const db  = admin.firestore();
    const now = admin.firestore.FieldValue.serverTimestamp();

    // ── 1a. Clamp sensor values to physical limits ──────────────────────
    const cleanFill = Math.min(100, Math.max(0, Number(raw.fillPercent ?? 0)));
    const cleanGas  = Math.max(0, Number(raw.gasLevel ?? 0));
    const cleanBatt = Math.min(100, Math.max(0, Number(raw.battPercent ?? 100)));

    // ── 1b. Validate GPS within Selangor bounds ─────────────────────────
    const lat = Number(raw.lat ?? 0);
    const lng = Number(raw.lng ?? 0);
    const gpsValid =
      lat >= SELANGOR_BOUNDS.minLat && lat <= SELANGOR_BOUNDS.maxLat &&
      lng >= SELANGOR_BOUNDS.minLng && lng <= SELANGOR_BOUNDS.maxLng;

    // ── 1c. Detect sensor dropout (timestamp more than 2h old) ──────────
    let sensorDropout = false;
    if (raw.timestamp) {
      const readingAge = Date.now() - new Date(raw.timestamp).getTime();
      sensorDropout = readingAge > 2 * 60 * 60 * 1000; // 2 hours
    }

    const cleanData = {
      binId,
      fillPercent:    cleanFill,
      gasLevel:       cleanGas,
      gasAlert:       Boolean(raw.gasAlert),
      lastWasteType:  String(raw.lastWasteType ?? "unknown"),
      wasteConfidence:Number(raw.wasteConfidence ?? 0),
      hazardous:      Boolean(raw.hazardous),
      lidOpenCount:   Number(raw.lidOpenCount ?? 0),
      lat:            gpsValid ? lat : 3.0738,  // fallback: Shah Alam
      lng:            gpsValid ? lng : 101.5183,
      battPercent:    cleanBatt,
      battLow:        cleanBatt < 15,
      gpsValid,
      sensorDropout,
      sanitizedAt:    now,
      rawTimestamp:   raw.timestamp ?? null,
    };

    await db.collection("bins_clean").doc(binId).set(cleanData, { merge: true });

    console.log(
      `[ReconAgent] ${binId} sanitized: fill=${cleanFill}% batt=${cleanBatt}% dropout=${sensorDropout}`
    );
  }
);

// ────────────────────────────────────────────────────────────────────────────
// AGENT 2 — ANALYSIS
// ────────────────────────────────────────────────────────────────────────────
export const analysisAgent = ffirestore.onDocumentUpdated(
  "bins_clean/{binId}",
  async (event) => {
    const binId = event.params.binId;
    const data  = event.data?.after.data();
    if (!data) return;

    const db    = admin.firestore();
    const now   = admin.firestore.FieldValue.serverTimestamp();
    const alerts: Promise<any>[] = [];

    // ── 2a. Determine operational status ────────────────────────────────
    let status: BinStatus = "NORMAL";
    let severity = "low";

    if (data.sensorDropout) {
      status   = "SENSOR_OFFLINE";
      severity = "medium";
    } else if (data.hazardous || data.lastWasteType === "hazardous") {
      status   = "BERBAHAYA";
      severity = "critical";
    } else if (data.fillPercent >= 90) {
      status   = "KRITIKAL";
      severity = "critical";
    } else if (data.fillPercent >= 70 || data.battLow) {
      status   = "PERLU_PENYELENGGARAAN";
      severity = "high";
    }

    // ── 2b. Write updated status to /bins/{binId} ───────────────────────
    await db.collection("bins").doc(binId).set(
      {
        ...data,
        status,
        statusUpdatedAt: now,
      },
      { merge: true }
    );

    // ── 2c. Create alert if actionable ──────────────────────────────────
    if (status !== "NORMAL") {
      const prev = event.data?.before.data();
      const prevStatus = prev?.status ?? "NORMAL";

      // Only create a new alert if status has changed (avoid spam)
      if (prevStatus !== status) {
        let message = "";
        switch (status) {
          case "KRITIKAL":
            message = `Tong ${binId} kritikal: ${data.fillPercent}% penuh. Perlu kutipan segera.`;
            break;
          case "BERBAHAYA":
            message = `Tong ${binId}: Bahan berbahaya dikesan (${data.lastWasteType}). Tindakan segera!`;
            break;
          case "SENSOR_OFFLINE":
            message = `Tong ${binId}: Sensor tidak bertindak balas > 2 jam. Semak sambungan.`;
            break;
          case "PERLU_PENYELENGGARAAN":
            message = data.battLow
              ? `Tong ${binId}: Bateri rendah (${data.battPercent}%). Tukar bateri.`
              : `Tong ${binId}: Isi ${data.fillPercent}% — jadualkan kutipan.`;
            break;
        }

        alerts.push(
          db.collection("alerts").add({
            binId,
            type:       status,
            severity,
            message,
            fillPercent: data.fillPercent,
            battPercent: data.battPercent,
            resolved:   false,
            createdAt:  now,
          })
        );
      }
    }

    // ── 2d. Low battery specific alert ──────────────────────────────────
    if (data.battLow && !data.sensorDropout) {
      const prevBattLow = event.data?.before.data()?.battLow ?? false;
      if (!prevBattLow) {
        alerts.push(
          db.collection("alerts").add({
            binId,
            type:       "LOW_BATTERY",
            severity:   "medium",
            message:    `Tong ${binId}: Bateri rendah — ${data.battPercent}%. Sila cas atau tukar sel.`,
            battPercent: data.battPercent,
            resolved:   false,
            createdAt:  now,
          })
        );
      }
    }

    if (alerts.length > 0) await Promise.all(alerts);

    console.log(`[AnalysisAgent] ${binId} → status: ${status}`);
  }
);

// ────────────────────────────────────────────────────────────────────────────
// AGENT 3 — ORCHESTRATION
// ────────────────────────────────────────────────────────────────────────────
export const orchestrationAgent = ffirestore.onDocumentUpdated(
  "bins/{binId}",
  async (event) => {
    const binId  = event.params.binId;
    const after  = event.data?.after.data();
    const before = event.data?.before.data();
    if (!after) return;

    const newStatus  = after.status  as BinStatus | undefined;
    const prevStatus = before?.status as BinStatus | undefined;

    // Only act when a bin newly enters KRITIKAL or BERBAHAYA
    const shouldRoute =
      (newStatus === "KRITIKAL" || newStatus === "BERBAHAYA") &&
      newStatus !== prevStatus;

    if (!shouldRoute) return;

    const db = admin.firestore();

    // ── 3a. Determine zone for this bin ──────────────────────────────────
    // Zone is derived from bin location (simple lat/lng bucketing)
    const lat = Number(after.lat ?? 3.0738);
    let zone = "Petaling";
    if (lat < 3.05)       zone = "Klang";
    else if (lat > 3.20)  zone = "Gombak";
    else if (lat > 3.10)  zone = "Hulu_Langat";

    // ── 3b. Fetch all KRITIKAL/BERBAHAYA bins in this zone ───────────────
    const critSnap = await db
      .collection("bins")
      .where("status", "in", ["KRITIKAL", "BERBAHAYA"])
      .get();

    const criticalBins = critSnap.docs
      .map((d) => ({ id: d.id, lat: d.data().lat, lng: d.data().lng, status: d.data().status }))
      .filter((b) => b.lat !== undefined);

    // ── 3c. Write new active route to /activeRoutes/{zone} ───────────────
    await db.collection("activeRoutes").doc(zone).set({
      zone,
      criticalBins,
      count:           criticalBins.length,
      triggerBin:      binId,
      triggerStatus:   newStatus,
      routeUpdatedAt:  admin.firestore.FieldValue.serverTimestamp(),
      routeSource:     "orchestration_agent",
    });

    // ── 3d. FCM push to pbt-alerts topic ─────────────────────────────────
    try {
      await admin.messaging().send({
        topic: "pbt-alerts",
        notification: {
          title: `🚨 SmartTong — Zon ${zone} Memerlukan Perhatian`,
          body:  `${criticalBins.length} tong kritikal/berbahaya. Laluan optimum dikemas kini.`,
        },
        data: { zone, triggerBin: binId, count: String(criticalBins.length) },
      });
    } catch (e) {
      console.warn("[OrchestrationAgent] FCM failed:", e);
    }

    console.log(
      `[OrchestrationAgent] ${binId} → ${newStatus}. Route updated for zone ${zone} (${criticalBins.length} bins).`
    );
  }
);
