/**
 * SmartTONG AI — predictOverflow Cloud Function
 *
 * GET /predictOverflow?binId=B01
 * Returns: { binId, hoursToFull, confidence, predictedAt }
 *
 * Algorithm: Linear regression on fill-rate over last 24 hours.
 * If fill rate is positive (bin getting fuller), extrapolate to 100%.
 */

import * as functions from "firebase-functions/v2/https";
import * as admin from "firebase-admin";

interface Reading {
  fillPercent: number;
  timestamp: string;
}

export const predictOverflow = functions.onRequest(async (req, res) => {
  const binId = req.query.binId as string;

  if (!binId) {
    res.status(400).json({ error: "binId query param required" });
    return;
  }

  try {
    const db = admin.firestore();
    const cutoff = new Date(Date.now() - 24 * 60 * 60 * 1000).toISOString();

    // Fetch last 24h readings for this bin
    const snap = await db
      .collection("history")
      .doc(binId)
      .collection("readings")
      .where("timestamp", ">=", cutoff)
      .orderBy("timestamp", "asc")
      .get();

    if (snap.empty || snap.size < 2) {
      res.json({
        binId,
        hoursToFull: null,
        confidence: 0,
        message: "Insufficient data for prediction (need at least 2 readings)",
        predictedAt: new Date().toISOString(),
      });
      return;
    }

    const readings: Reading[] = snap.docs.map((d) => d.data() as Reading);

    // Linear regression: time (hours from first reading) vs fillPercent
    const t0 = new Date(readings[0].timestamp).getTime();
    const points = readings.map((r) => ({
      x: (new Date(r.timestamp).getTime() - t0) / 3600000, // hours from start
      y: r.fillPercent,
    }));

    const n = points.length;
    const sumX  = points.reduce((s, p) => s + p.x, 0);
    const sumY  = points.reduce((s, p) => s + p.y, 0);
    const sumXY = points.reduce((s, p) => s + p.x * p.y, 0);
    const sumX2 = points.reduce((s, p) => s + p.x * p.x, 0);

    const slope     = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    const intercept = (sumY - slope * sumX) / n;
    const currentFill = readings[readings.length - 1].fillPercent;

    // R² for confidence
    const meanY = sumY / n;
    const ssTot = points.reduce((s, p) => s + Math.pow(p.y - meanY, 2), 0);
    const ssRes = points.reduce((s, p) => s + Math.pow(p.y - (slope * p.x + intercept), 2), 0);
    const rSquared = ssTot === 0 ? 1 : 1 - ssRes / ssTot;

    let hoursToFull: number | null = null;
    if (slope > 0.1) {
      // Time to reach 100% from current fill: (100 - currentFill) / slope
      hoursToFull = Math.max(0, (100 - currentFill) / slope);
    }

    const result = {
      binId,
      currentFill: Math.round(currentFill),
      hoursToFull: hoursToFull ? Math.round(hoursToFull * 10) / 10 : null,
      fillRatePerHour: Math.round(slope * 10) / 10,
      confidence: Math.max(0, Math.min(1, rSquared)),
      readingsUsed: n,
      predictedAt: new Date().toISOString(),
    };

    // Save prediction back to bin document
    await db.collection("bins").doc(binId).update({
      prediction: result,
      lastUpdated: admin.firestore.FieldValue.serverTimestamp(),
    });

    res.json(result);
  } catch (err) {
    console.error("predictOverflow error:", err);
    res.status(500).json({ error: "Internal error", details: String(err) });
  }
});
