/**
 * SmartTONG AI — aggregateKPIs Cloud Function
 *
 * Scheduled: runs daily at 23:00 MYT (15:00 UTC)
 * Aggregates daily KPIs: trips saved, fuel saved, recycling rate by zone
 */

import * as functions from "firebase-functions/v2/scheduler";
import * as admin from "firebase-admin";

const DIESEL_PRICE_RM    = 2.80;   // RM per litre
const FUEL_L_PER_TRIP    = 2.5;    // avg litres per bin trip
const CO2_KG_PER_L_DIESEL = 2.68;  // kg CO2 per litre diesel burned
const FIXED_SCHEDULE_DAILY_TRIPS = 700; // Baseline: 100 bins × 7 days fixed schedule

export const aggregateKPIs = functions.onSchedule(
  { schedule: "0 15 * * *", timeZone: "UTC" },
  async (_event) => {
    const db     = admin.firestore();
    const today  = new Date().toISOString().split("T")[0];
    console.log(`[aggregateKPIs] Running for date: ${today}`);

    // Fetch today's route (if generated)
    const routeSnap = await db.collection("routes")
      .where("date", "==", today)
      .get();

    let tripsToday     = FIXED_SCHEDULE_DAILY_TRIPS;  // default: all bins visited
    let tripsSavedToday = 0;

    if (!routeSnap.empty) {
      const routeData = routeSnap.docs[0].data();
      tripsToday      = routeData.totalBins || tripsToday;
      tripsSavedToday = routeData.tripsSaved || 0;
    }

    // Aggregate waste type distribution from today's readings
    const readingsSnap = await db.collectionGroup("readings")
      .where("timestamp", ">=", `${today}T00:00:00`)
      .where("timestamp", "<=", `${today}T23:59:59`)
      .get();

    const wasteTypeCounts: Record<string, number> = {
      organic:    0,
      recyclable: 0,
      hazardous:  0,
      empty:      0,
      unknown:    0,
    };
    let totalClassifications = 0;

    readingsSnap.docs.forEach((d) => {
      const wt = d.data().lastWasteType as string;
      if (wt && wasteTypeCounts[wt] !== undefined) {
        wasteTypeCounts[wt]++;
      }
      totalClassifications++;
    });

    // Recycling rate = recyclable / (total - empty - unknown)
    const recyclableCount = wasteTypeCounts["recyclable"];
    const denominator     = Math.max(1, totalClassifications - wasteTypeCounts["empty"] - wasteTypeCounts["unknown"]);
    const recyclingRate   = Math.round((recyclableCount / denominator) * 1000) / 10;

    // Fuel and CO2 savings
    const fuelSavedL    = tripsSavedToday * FUEL_L_PER_TRIP;
    const fuelSavedRM   = fuelSavedL * DIESEL_PRICE_RM;
    const co2SavedKg    = fuelSavedL * CO2_KG_PER_L_DIESEL;

    const kpi = {
      date:              today,
      tripsToday,
      tripsSaved:        tripsSavedToday,
      baselineTrips:     FIXED_SCHEDULE_DAILY_TRIPS,
      fuelSavedL:        Math.round(fuelSavedL * 10) / 10,
      fuelSavedRM:       Math.round(fuelSavedRM * 100) / 100,
      co2SavedKg:        Math.round(co2SavedKg * 100) / 100,
      recyclingRate,
      wasteTypeCounts,
      totalReadings:     totalClassifications,
      generatedAt:       new Date().toISOString(),
    };

    await db.collection("analytics").doc(`daily_${today}`).set(kpi);
    console.log("[aggregateKPIs] Saved:", kpi);
  }
);
