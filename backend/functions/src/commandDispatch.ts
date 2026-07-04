/**
 * SmartTONG AI — commandDispatch.ts  (Phase 2)
 *
 * POST /commandDispatch
 * Body: { binId: string, command: "OPEN_SLOT_A" | "OPEN_SLOT_B" | "LOCK" }
 *
 * Writes the command to Firebase RTDB: /commands/{binId}/pendingCommand
 * The ESP32 polls this path on every lid-close via MQTTClient.pollCommand().
 *
 * Authorization: requires X-SmartTong-Secret header matching DISPATCH_SECRET
 * env variable (set via: firebase functions:secrets:set DISPATCH_SECRET)
 */

import * as functions from "firebase-functions/v2/https";
import * as admin from "firebase-admin";

const VALID_COMMANDS = ["OPEN_SLOT_A", "OPEN_SLOT_B", "LOCK"] as const;
type Command = (typeof VALID_COMMANDS)[number];

export const commandDispatch = functions.onRequest(
  { secrets: ["DISPATCH_SECRET"] },
  async (req, res) => {
    // ── CORS preflight ──────────────────────────────────────────────────
    res.set("Access-Control-Allow-Origin", "*");
    if (req.method === "OPTIONS") {
      res.set("Access-Control-Allow-Methods", "POST");
      res.set("Access-Control-Allow-Headers", "Content-Type, X-SmartTong-Secret");
      res.status(204).send("");
      return;
    }

    if (req.method !== "POST") {
      res.status(405).json({ error: "Method not allowed — use POST" });
      return;
    }

    // ── Auth check ──────────────────────────────────────────────────────
    const secret = process.env.DISPATCH_SECRET;
    if (secret && req.headers["x-smarttong-secret"] !== secret) {
      res.status(403).json({ error: "Unauthorized" });
      return;
    }

    const { binId, command } = req.body as { binId: string; command: string };

    if (!binId || !command) {
      res.status(400).json({ error: "binId and command are required" });
      return;
    }

    if (!VALID_COMMANDS.includes(command as Command)) {
      res.status(400).json({
        error: `Invalid command. Must be one of: ${VALID_COMMANDS.join(", ")}`,
      });
      return;
    }

    try {
      const db = admin.database();

      // Write command to RTDB — ESP32 polls this within 5s of lid close
      await db.ref(`commands/${binId}/pendingCommand`).set(command);

      // Log the dispatch event to Firestore for audit trail
      await admin.firestore().collection("dispatch_log").add({
        binId,
        command,
        dispatchedAt: admin.firestore.FieldValue.serverTimestamp(),
        source: "commandDispatch_CF",
      });

      console.log(`[commandDispatch] ${command} → ${binId}`);

      res.json({
        success: true,
        binId,
        command,
        message: `Command ${command} queued for bin ${binId}. ESP32 will execute on next poll.`,
        queuedAt: new Date().toISOString(),
      });
    } catch (err) {
      console.error("[commandDispatch] Error:", err);
      res.status(500).json({ error: "Internal error", details: String(err) });
    }
  }
);
