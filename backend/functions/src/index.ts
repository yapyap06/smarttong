/**
 * SmartTONG AI — Cloud Functions entry point
 * Initialize Firebase Admin SDK once, then export all functions.
 */

import * as admin from "firebase-admin";
admin.initializeApp();

// ── Phase 1 & 2: Original pipeline ──────────────────────────────────────────
export { predictOverflow }  from "./predictOverflow";
export { optimizeRoute }    from "./optimizeRoute";
export { aggregateKPIs }    from "./aggregateKPIs";
export { seedDemoData }     from "./seedDemoData";

// ── Phase 2: Remote actuation ────────────────────────────────────────────────
export { commandDispatch }  from "./commandDispatch";

// ── Phase 3: AI Vision + Multi-Agent ────────────────────────────────────────
export { classifyWaste }                                       from "./classifyWaste";
export { reconAgent, analysisAgent, orchestrationAgent }      from "./multiAgent";

// ── Phase 4: Rewards + Live Simulation ──────────────────────────────────────
export { rewardPoints }     from "./rewardPoints";
export { simulateTick }     from "./simulateTick";

// ── Phase 2: Anomaly detection (existing) ────────────────────────────────────
export { detectAnomaly }    from "./detectAnomaly";
