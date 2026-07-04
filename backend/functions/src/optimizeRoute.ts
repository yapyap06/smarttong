/**
 * SmartTONG AI — optimizeRoute Cloud Function
 *
 * POST /optimizeRoute
 * Body: { "zone": "MBSJ", "date": "2026-07-02" }
 * Returns: ordered bin list + estimated distance/time + route polyline
 *
 * Uses Google Maps Directions API with optimizeWaypoints: true
 */

import * as functions from "firebase-functions/v2/https";
import * as admin from "firebase-admin";

// Set this in Firebase environment: firebase functions:config:set maps.key="YOUR_API_KEY"
const MAPS_API_KEY = process.env.GOOGLE_MAPS_API_KEY || "";
const FILL_THRESHOLD = 70;        // Only collect bins >= 70% full
const DEPOT_LAT = 3.0785;         // PBT depot location (Subang Jaya example)
const DEPOT_LNG = 101.5144;

interface BinDoc {
  binId: string;
  lat: number;
  lng: number;
  fillPercent: number;
  zone: string;
}

export const optimizeRoute = functions.onRequest(async (req, res) => {
  if (req.method !== "POST") {
    res.status(405).json({ error: "POST required" });
    return;
  }

  const { zone = "ALL", date = new Date().toISOString().split("T")[0] } = req.body;

  try {
    const db = admin.firestore();

    // Fetch all bins that need collection
    let query = db.collection("bins").where("fillPercent", ">=", FILL_THRESHOLD);
    if (zone !== "ALL") {
      query = query.where("zone", "==", zone) as any;
    }

    const snap = await query.get();
    const bins: BinDoc[] = snap.docs.map((d) => ({
      binId:       d.id,
      lat:         d.data().lat,
      lng:         d.data().lng,
      fillPercent: d.data().fillPercent,
      zone:        d.data().zone,
    }));

    if (bins.length === 0) {
      res.json({
        date,
        zone,
        bins: [],
        totalBins:         0,
        estimatedDistance: 0,
        estimatedMinutes:  0,
        tripsSaved:        0,
        message:           "No bins above threshold. No collection required today.",
      });
      return;
    }

    // Sort bins by fill% descending (urgency order — priority in case API call fails)
    bins.sort((a, b) => b.fillPercent - a.fillPercent);

    // Build Google Maps Directions API request
    const origin      = `${DEPOT_LAT},${DEPOT_LNG}`;
    const destination = origin; // Return to depot
    const waypoints   = bins.map((b) => `${b.lat},${b.lng}`).join("|");

    let optimizedOrder = bins.map((_, i) => i);  // default: as-is
    let totalDistanceM = 0;
    let totalDurationS = 0;

    if (MAPS_API_KEY && bins.length > 0) {
      const url = `https://maps.googleapis.com/maps/api/directions/json?` +
        `origin=${origin}&destination=${destination}` +
        `&waypoints=optimize:true|${waypoints}` +
        `&key=${MAPS_API_KEY}`;

      const resp = await fetch(url);
      const json = await resp.json() as any;

      if (json.status === "OK" && json.routes?.[0]) {
        optimizedOrder = json.routes[0].waypoint_order;
        for (const leg of json.routes[0].legs) {
          totalDistanceM += leg.distance.value;
          totalDurationS += leg.duration.value;
        }
      }
    }

    // Reorder bins by optimized waypoint order
    const orderedBins = optimizedOrder.map((i: number) => bins[i]);

    // Cost savings estimate
    const totalBinsInZone  = snap.size;
    const fixedScheduleTrips = totalBinsInZone;       // Fixed schedule = visit all bins
    const optimizedTrips    = orderedBins.length;     // Smart route = only full bins
    const tripsSaved        = fixedScheduleTrips - optimizedTrips;
    const fuelSavedL        = (tripsSaved * 2.5);      // avg 2.5L per bin trip
    const fuelSavedRM       = fuelSavedL * 2.80;       // RM 2.80/L diesel

    const routeResult = {
      date,
      zone,
      bins:              orderedBins,
      totalBins:         orderedBins.length,
      estimatedDistanceKm: Math.round(totalDistanceM / 100) / 10,
      estimatedMinutes:  Math.round(totalDurationS / 60),
      tripsSaved,
      fuelSavedL:        Math.round(fuelSavedL * 10) / 10,
      fuelSavedRM:       Math.round(fuelSavedRM * 100) / 100,
      generatedAt:       new Date().toISOString(),
    };

    // Save route to Firestore
    await db.collection("routes").doc(`${date}_${zone}`).set(routeResult);

    res.json(routeResult);
  } catch (err) {
    console.error("optimizeRoute error:", err);
    res.status(500).json({ error: "Internal error", details: String(err) });
  }
});
