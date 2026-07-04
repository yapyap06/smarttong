/**
 * SmartTONG AI — rewardPoints.ts  (Phase 4)
 *
 * Firestore trigger: fires on every new /reports/{reportId} document.
 * Awards Eco-Points based on report type and updates:
 *   - /users/{icHash}/points   (individual accumulator)
 *   - /leaderboard/{icHash}    (public ranking document)
 *
 * Points schedule:
 *   overflow  → +10 pts
 *   illegal   → +15 pts
 *   hazardous → +25 pts
 *   pothole   → +10 pts
 *   broken    → +10 pts
 *
 * Tier milestones:
 *   0–499    → Pejuang 🌿
 *   500–999  → Juara 🏆
 *   1000+    → Wira 🦅
 */

import * as ffirestore from "firebase-functions/v2/firestore";
import * as admin from "firebase-admin";

const POINTS_MAP: Record<string, number> = {
  overflow:  10,
  illegal:   15,
  hazardous: 25,
  pothole:   10,
  broken:    10,
  other:     5,
};

function getTier(points: number): { name: string; emoji: string } {
  if (points >= 1000) return { name: "Wira",    emoji: "🦅" };
  if (points >= 500)  return { name: "Juara",   emoji: "🏆" };
  return                     { name: "Pejuang", emoji: "🌿" };
}

export const rewardPoints = ffirestore.onDocumentCreated(
  "reports/{reportId}",
  async (event) => {
    const reportId = event.params.reportId;
    const data     = event.data?.data();
    if (!data) return;

    const icHash   = data.icHash as string | undefined;
    const type     = (data.type as string) ?? "other";
    const pts      = POINTS_MAP[type] ?? 5;

    if (!icHash) {
      console.warn(`[rewardPoints] Report ${reportId} has no icHash — skipping points.`);
      return;
    }

    const db  = admin.firestore();
    const now = admin.firestore.FieldValue.serverTimestamp();

    // ── Atomically increment user points ────────────────────────────────
    const userRef = db.collection("users").doc(icHash);
    await db.runTransaction(async (tx) => {
      const userDoc = await tx.get(userRef);
      const currentPts  = userDoc.exists ? (userDoc.data()!.points as number) : 0;
      const newPts      = currentPts + pts;
      const prevTier    = getTier(currentPts);
      const newTier     = getTier(newPts);
      const tieredUp    = newTier.name !== prevTier.name;

      tx.set(
        userRef,
        {
          points:          newPts,
          tier:            newTier.name,
          tierEmoji:       newTier.emoji,
          lastReportType:  type,
          lastPointsEarned: pts,
          lastReportAt:    now,
          updatedAt:       now,
        },
        { merge: true }
      );

      // Update public leaderboard
      tx.set(
        db.collection("leaderboard").doc(icHash),
        {
          icHash,
          points:    newPts,
          tier:      newTier.name,
          tierEmoji: newTier.emoji,
          updatedAt: now,
        },
        { merge: true }
      );

      // If tier upgraded → send FCM push to citizen
      if (tieredUp) {
        const token = userDoc.data()?.fcmToken as string | undefined;
        if (token) {
          await admin.messaging().send({
            token,
            notification: {
              title: `🎉 Tahniah! Anda naik ke tahap ${newTier.name} ${newTier.emoji}`,
              body:  `Anda kini mempunyai ${newPts} Mata Kebersihan. Teruskan usaha mulia!`,
            },
            data: { points: String(newPts), tier: newTier.name },
          });
        }
      }
    });

    // Mark the report as processed
    await db.collection("reports").doc(reportId).update({
      pointsAwarded: pts,
      pointsProcessedAt: now,
    });

    console.log(
      `[rewardPoints] ${icHash} +${pts} pts (type: ${type}). Report: ${reportId}`
    );
  }
);
