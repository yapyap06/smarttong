/**
 * SmartTONG AI — simulateTick.ts  (Phase 4)
 *
 * GET /simulateTick?zone=Petaling
 *
 * Advances the simulation by ONE timestep (represents ~30min of real time).
 * Designed for the judging booth: hit this endpoint repeatedly to show
 * live bin fill changes, route recalculations, and critical alerts
 * on the dashboard — without needing real hardware publishing.
 *
 * Each tick:
 *   1. Increments fillPercent for each bin by a randomized rate
 *   2. 5% chance of triggering a hazardous event on a random bin
 *   3. "Collects" bins that have been dispatched (resets to 5%)
 *   4. Writes updated readings to /readings/{binId} (triggers multi-agent pipeline)
 *   5. Returns summary of what changed
 */

import * as functions from "firebase-functions/v2/https";
import * as admin from "firebase-admin";

// Simulated bins and their fill rate variability per zone
const ZONE_BINS: Record<string, string[]> = {
  Petaling:      ["B01", "B02", "B03"],
  Klang:         ["B04", "B05"],
  Gombak:        ["B06", "B07"],
  Hulu_Langat:   ["B08"],
  ALL:           ["B01", "B02", "B03", "B04", "B05", "B06", "B07", "B08"],
};

// Fill rate per 30-min tick (% per tick, with randomness)
const BIN_FILL_RATES: Record<string, number> = {
  B01: 8,   // Busy: Persiaran Kewajipan
  B02: 5,   // Medium: SS15
  B03: 12,  // Very busy: Dataran Sunway
  B04: 3,   // Low: Taman Jaya
  B05: 7,   // Medium: Pasar Malam
  B06: 4,
  B07: 6,
  B08: 2,
};

interface TickResult {
  binId: string;
  prevFill: number;
  newFill: number;
  status: string;
  event?: string;
}

export const simulateTick = functions.onRequest(async (req, res) => {
  res.set("Access-Control-Allow-Origin", "*");
  if (req.method === "OPTIONS") {
    res.set("Access-Control-Allow-Methods", "GET");
    res.status(204).send("");
    return;
  }

  const zone    = (req.query.zone as string) ?? "ALL";
  const binIds  = ZONE_BINS[zone] ?? ZONE_BINS.ALL;
  const db      = admin.firestore();
  const rtdb    = admin.database();
  const now     = admin.firestore.FieldValue.serverTimestamp();
  const results: TickResult[] = [];
  const batch   = db.batch();

  for (const binId of binIds) {
    // ── Read current state from Firestore bins collection ────────────────
    const binDoc = await db.collection("bins").doc(binId).get();
    const current = binDoc.data() ?? {};

    const prevFill = Number(current.fillPercent ?? Math.random() * 40 + 10);
    const prevBatt = Number(current.battPercent ?? 85);
    const isDispatched = current.status === "DISPATCHED";

    // ── Calculate new fill ───────────────────────────────────────────────
    let newFill: number;
    if (isDispatched) {
      newFill = 5; // Truck collected it — reset
    } else {
      const rate = BIN_FILL_RATES[binId] ?? 5;
      const jitter = (Math.random() - 0.3) * 4; // ±4% jitter, biased positive
      newFill = Math.min(98, prevFill + rate + jitter);
    }

    // ── Simulate slow battery drain (0.3% per tick) ──────────────────────
    const newBatt = Math.max(5, prevBatt - 0.3);

    // ── 5% chance: hazardous event ──────────────────────────────────────
    const isHazardous = Math.random() < 0.05;
    const wasteTypes  = ["PLASTIK", "KERTAS", "LOGAM", "SISA_BAKI", "KACA"];
    const wasteType   = isHazardous ? "hazardous" : wasteTypes[Math.floor(Math.random() * wasteTypes.length)];

    // ── Write to RTDB readings (triggers Firestore multi-agent pipeline) ─
    await rtdb.ref(`readings/${binId}`).set({
      binId,
      fillPercent:    Math.round(newFill * 10) / 10,
      gasLevel:       isHazardous ? 420 : Math.floor(Math.random() * 200 + 50),
      gasAlert:       isHazardous,
      lastWasteType:  wasteType,
      wasteConfidence: 0.85 + Math.random() * 0.1,
      hazardous:      isHazardous,
      lidOpenCount:   (current.lidOpenCount ?? 0) + Math.floor(Math.random() * 3),
      lat:            current.lat ?? 3.0738,
      lng:            current.lng ?? 101.5183,
      battPercent:    Math.round(newBatt),
      battLow:        newBatt < 15,
      timestamp:      new Date().toISOString(),
      simulated:      true,
    });

    // ── Also update Firestore bins doc directly for dashboard speed ──────
    let status: string;
    if (isHazardous)      status = "BERBAHAYA";
    else if (newFill >= 90) status = "KRITIKAL";
    else if (newFill >= 70) status = "PERLU_PENYELENGGARAAN";
    else                   status = "NORMAL";

    batch.set(
      db.collection("bins").doc(binId),
      {
        binId,
        fillPercent:   Math.round(newFill * 10) / 10,
        battPercent:   Math.round(newBatt),
        battLow:       newBatt < 15,
        lastWasteType: wasteType,
        hazardous:     isHazardous,
        status,
        simulated:     true,
        statusUpdatedAt: now,
      },
      { merge: true }
    );

    const result: TickResult = {
      binId,
      prevFill: Math.round(prevFill),
      newFill:  Math.round(newFill),
      status,
    };
    if (isHazardous)  result.event = "HAZARDOUS_DETECTED";
    if (isDispatched) result.event = "COLLECTED";
    results.push(result);
  }

  await batch.commit();

  const newCritical  = results.filter((r) => r.status === "KRITIKAL").length;
  const newHazardous = results.filter((r) => r.event === "HAZARDOUS_DETECTED").length;
  const collected    = results.filter((r) => r.event === "COLLECTED").length;

  console.log(
    `[simulateTick] Zone: ${zone} | ${binIds.length} bins updated | critical:${newCritical} hazardous:${newHazardous} collected:${collected}`
  );

  res.json({
    success:       true,
    zone,
    binsUpdated:   results.length,
    newCritical,
    newHazardous,
    collected,
    tickAt:        new Date().toISOString(),
    bins:          results,
  });
});
