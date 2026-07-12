/**********************************************************************************************
 *
 *  Beehold global leaderboard — Cloudflare Worker + KV
 *
 *  GET  /top?n=10          → { entries: [{ name, centiseconds, stars }, ...] }
 *  POST /submit            → body { name, centiseconds, stars }; header X-Beehold-Key
 *  POST /clear             → wipe all scores; header X-Beehold-Key
 *  OPTIONS *               → CORS preflight
 *
 **********************************************************************************************/

const MAX_STORED = 200;
const MAX_TOP = 50;
const NAME_RE = /^[A-Za-z0-9]{1,16}-[0-9]{10}$/;

// Baseline entries (run-id suffix is stripped in the game UI).
const SEED_SCORES = [
  { name: "fastbee-0000000001", centiseconds: 8 * 60 * 100, stars: 60 },   // 8:00.00
  { name: "slowbee-0000000001", centiseconds: 20 * 60 * 100, stars: 24 },  // 20:00.00
];

const corsHeaders = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
  "Access-Control-Allow-Headers": "Content-Type, X-Beehold-Key",
  "Access-Control-Max-Age": "86400",
};

function json(data, status = 200) {
  return new Response(JSON.stringify(data), {
    status,
    headers: {
      "Content-Type": "application/json; charset=utf-8",
      ...corsHeaders,
    },
  });
}

function requireKey(request, env) {
  const key = request.headers.get("X-Beehold-Key") || "";
  return !!(env.SUBMIT_KEY && key === env.SUBMIT_KEY);
}

async function loadBoard(env) {
  const raw = await env.BOARD.get("scores", "json");
  return Array.isArray(raw) ? raw : [];
}

async function saveBoard(env, entries) {
  await env.BOARD.put("scores", JSON.stringify(entries));
}

// Make sure baseline ghosts exist (does not overwrite if already present).
async function ensureSeeds(env, board) {
  let changed = false;
  for (const seed of SEED_SCORES) {
    if (!board.some((e) => e.name === seed.name)) {
      board.push({ name: seed.name, centiseconds: seed.centiseconds, stars: seed.stars, ts: 0 });
      changed = true;
    }
  }
  if (changed) {
    board.sort(sortBestFirst);
    if (board.length > MAX_STORED) board.length = MAX_STORED;
    await saveBoard(env, board);
  }
  return board;
}

function sanitizeEntry(body) {
  if (!body || typeof body !== "object") return null;

  const name = String(body.name || "").trim();
  if (!NAME_RE.test(name)) return null;

  let centiseconds = Number(body.centiseconds);
  if (!Number.isFinite(centiseconds)) return null;
  centiseconds = Math.floor(centiseconds);
  if (centiseconds < 1) centiseconds = 1;
  if (centiseconds > 10000000) centiseconds = 10000000;

  let stars = Number(body.stars);
  if (!Number.isFinite(stars)) stars = 0;
  stars = Math.floor(stars);
  if (stars < 0) stars = 0;
  if (stars > 999) stars = 999;

  return { name, centiseconds, stars, ts: Date.now() };
}

function sortBestFirst(a, b) {
  if (a.centiseconds !== b.centiseconds) return a.centiseconds - b.centiseconds;
  if (a.stars !== b.stars) return b.stars - a.stars;
  return (a.ts || 0) - (b.ts || 0);
}

async function handleTop(request, env) {
  const url = new URL(request.url);
  let n = parseInt(url.searchParams.get("n") || "10", 10);
  if (!Number.isFinite(n) || n < 1) n = 10;
  if (n > MAX_TOP) n = MAX_TOP;

  const board = await ensureSeeds(env, await loadBoard(env));
  board.sort(sortBestFirst);
  const entries = board.slice(0, n).map((e) => ({
    name: e.name,
    centiseconds: e.centiseconds,
    stars: e.stars,
  }));
  return json({ entries });
}

async function handleSubmit(request, env) {
  if (!requireKey(request, env)) {
    return json({ ok: false, error: "unauthorized" }, 401);
  }

  let body;
  try {
    body = await request.json();
  } catch {
    return json({ ok: false, error: "bad json" }, 400);
  }

  const entry = sanitizeEntry(body);
  if (!entry) return json({ ok: false, error: "invalid entry" }, 400);

  const board = await ensureSeeds(env, await loadBoard(env));
  // Unique run id in the name → keep each submit; replace only if same name+run repeats.
  const idx = board.findIndex((e) => e.name === entry.name);
  if (idx >= 0) board[idx] = entry;
  else board.push(entry);

  board.sort(sortBestFirst);
  if (board.length > MAX_STORED) board.length = MAX_STORED;
  await saveBoard(env, board);

  return json({ ok: true });
}

async function handleClear(request, env) {
  if (!requireKey(request, env)) {
    return json({ ok: false, error: "unauthorized" }, 401);
  }
  // Reset to baseline ghosts only (not a fully empty board).
  await saveBoard(env, []);
  await ensureSeeds(env, []);
  return json({ ok: true, cleared: true });
}

export default {
  async fetch(request, env) {
    if (request.method === "OPTIONS") {
      return new Response(null, { status: 204, headers: corsHeaders });
    }

    const url = new URL(request.url);
    const path = url.pathname.replace(/\/+$/, "") || "/";

    try {
      if (request.method === "GET" && (path === "/top" || path === "/")) {
        return await handleTop(request, env);
      }
      if (request.method === "POST" && path === "/submit") {
        return await handleSubmit(request, env);
      }
      if (request.method === "POST" && path === "/clear") {
        return await handleClear(request, env);
      }
      return json({ ok: false, error: "not found" }, 404);
    } catch (err) {
      return json({ ok: false, error: "server error" }, 500);
    }
  },
};
