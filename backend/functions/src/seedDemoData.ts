/**
 * SmartTONG AI — seedDemoData Cloud Function
 *
 * GET /seedDemoData
 * Seeds 5 demo bins with realistic data into Firestore.
 * Run ONCE after Firebase project setup to populate the dashboard.
 *
 * DELETE THIS FUNCTION before production deployment.
 */

import * as functions from "firebase-functions/v2/https";
import * as admin from "firebase-admin";

const DEMO_BINS = [
  {
    binId:       "B01",
    name:        "Persiaran Kewajipan, USJ 1",
    lat:         3.0481,
    lng:         101.5787,
    zone:        "MBSJ",
    fillPercent: 85,
    lastWasteType: "organic",
    gasLevel:    220,
    hazardous:   false,
    lidOpenCount: 42,
    solarCharging: true,
    batteryMv:   3850,
  },
  {
    binId:       "B02",
    name:        "Jalan SS15/4, Subang Jaya",
    lat:         3.0754,
    lng:         101.5843,
    zone:        "MBSJ",
    fillPercent: 62,
    lastWasteType: "recyclable",
    gasLevel:    89,
    hazardous:   false,
    lidOpenCount: 27,
    solarCharging: true,
    batteryMv:   3910,
  },
  {
    binId:       "B03",
    name:        "Dataran Sunway, PJS 11",
    lat:         3.0680,
    lng:         101.6014,
    zone:        "MBPJ",
    fillPercent: 93,
    lastWasteType: "organic",
    gasLevel:    380,
    hazardous:   false,
    lidOpenCount: 61,
    solarCharging: false,
    batteryMv:   3620,
  },
  {
    binId:       "B04",
    name:        "Taman Jaya Park, Petaling Jaya",
    lat:         3.1069,
    lng:         101.6380,
    zone:        "MBPJ",
    fillPercent: 28,
    lastWasteType: "recyclable",
    gasLevel:    55,
    hazardous:   false,
    lidOpenCount: 12,
    solarCharging: true,
    batteryMv:   4050,
  },
  {
    binId:       "B05",
    name:        "Pasar Malam Shah Alam, Seksyen 14",
    lat:         3.0738,
    lng:         101.5183,
    zone:        "MBSA",
    fillPercent: 77,
    lastWasteType: "hazardous",
    gasLevel:    415,
    hazardous:   true,
    lidOpenCount: 19,
    solarCharging: true,
    batteryMv:   3780,
  },
];

export const seedDemoData = functions.onRequest(async (req, res) => {
  if (req.method !== "GET") {
    res.status(405).json({ error: "GET request required" });
    return;
  }

  const db  = admin.firestore();
  const now = new Date().toISOString();
  const batch = db.batch();

  for (const bin of DEMO_BINS) {
    // Write to /bins/{binId}
    const binRef = db.collection("bins").doc(bin.binId);
    batch.set(binRef, {
      ...bin,
      timestamp:   now,
      lastUpdated: admin.firestore.FieldValue.serverTimestamp(),
      prediction: {
        hoursToFull:     bin.fillPercent > 70 ? Math.round(Math.random() * 8 + 2) : null,
        confidence:      bin.fillPercent > 70 ? 0.82 : 0,
        fillRatePerHour: bin.fillPercent > 70 ? 4.2 : 0.8,
      },
    });

    // Write to /readings/{binId} (current reading)
    const readingRef = db.collection("readings").doc(bin.binId);
    batch.set(readingRef, {
      binId:         bin.binId,
      fillPercent:   bin.fillPercent,
      gasLevel:      bin.gasLevel,
      gasAlert:      bin.gasLevel > 300,
      lastWasteType: bin.lastWasteType,
      hazardous:     bin.hazardous,
      lidOpenCount:  bin.lidOpenCount,
      lat:           bin.lat,
      lng:           bin.lng,
      batteryMv:     bin.batteryMv,
      solarCharging: bin.solarCharging,
      timestamp:     now,
    });
  }

  await batch.commit();

  // Seed historical data for fill-rate prediction demo
  for (const bin of DEMO_BINS) {
    const histRef = db.collection("history").doc(bin.binId).collection("readings");
    const histBatch = db.batch();
    for (let h = 24; h >= 0; h--) {
      const t = new Date(Date.now() - h * 3600 * 1000).toISOString();
      const fill = Math.max(0, bin.fillPercent - h * (bin.fillPercent / 30));
      histBatch.set(histRef.doc(), {
        timestamp: t,
        fillPercent: Math.round(fill),
        gasLevel: Math.round(bin.gasLevel * (fill / bin.fillPercent + 0.1)),
        lastWasteType: bin.lastWasteType,
      });
    }
    await histBatch.commit();
  }

  res.json({
    success: true,
    seededBins: DEMO_BINS.length,
    message: "Demo data seeded. Remove seedDemoData before production!",
  });
});
