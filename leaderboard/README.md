# Beehold leaderboard (Cloudflare Worker)

HTTPS + CORS API for the itch.io web build. Replaces dreamlo.

## Endpoints

| Method | Path | Notes |
|--------|------|--------|
| `GET` | `/top?n=10` | Best times first (`centiseconds` ascending) |
| `POST` | `/submit` | JSON body + `X-Beehold-Key` header |
| `POST` | `/clear` | Wipe all scores (`X-Beehold-Key` required) |
| `OPTIONS` | `*` | CORS preflight |

Submit body:

```json
{ "name": "Player-0123456789", "centiseconds": 12345, "stars": 72 }
```

`name` must be `A-Za-z0-9` (1–16) + `-` + 10 digits (unique run id from the game).

---

## Deploy (step by step)

### 1. Cloudflare account

1. Sign up / log in at [https://dash.cloudflare.com](https://dash.cloudflare.com)
2. You do **not** need a custom domain — `*.workers.dev` is enough

### 2. Install Node.js + Wrangler

Install [Node.js LTS](https://nodejs.org/) if you do not have it, then in a terminal:

```bash
cd leaderboard
npx wrangler --version
```

(`npx` downloads Wrangler on demand; no global install required.)

### 3. Log in

```bash
cd leaderboard
npx wrangler login
```

A browser window opens — approve Cloudflare access.

### 4. Create the KV namespace

```bash
npx wrangler kv namespace create BOARD
npx wrangler kv namespace create BOARD --preview
```

Each command prints an **id**. Open `wrangler.toml` and replace:

- `REPLACE_WITH_KV_NAMESPACE_ID` → id from the first command  
- `REPLACE_WITH_KV_PREVIEW_ID` → id from the `--preview` command  

### 5. Set the submit secret

Pick a long random string (or keep the one already in `src/hex_scores.h` as `HEX_LEADERBOARD_KEY`). Set it on Cloudflare:

```bash
npx wrangler secret put SUBMIT_KEY
```

When prompted, paste the **same** value as `HEX_LEADERBOARD_KEY` in `src/hex_scores.h`.

### 6. Deploy

```bash
npx wrangler deploy
```

Wrangler prints a URL like:

```text
https://beehold-leaderboard.<your-subdomain>.workers.dev
```

### 7. Point the game at that URL

In `src/hex_scores.h`, set:

```c
#define HEX_LEADERBOARD_URL "https://beehold-leaderboard.mauricioohse.workers.dev"
```

(no trailing slash)

Rebuild the web game and re-upload to itch.

### 8. Quick smoke test

```bash
# Read board (empty at first)
curl "https://beehold-leaderboard.<your-subdomain>.workers.dev/top?n=10"

# Submit a fake run (use your real SUBMIT_KEY)
curl -X POST "https://beehold-leaderboard.<your-subdomain>.workers.dev/submit" \
  -H "Content-Type: application/json" \
  -H "X-Beehold-Key: YOUR_KEY_HERE" \
  -d "{\"name\":\"TestPlayer-1234567890\",\"centiseconds\":99999,\"stars\":24}"

# Wipe the entire board
curl -X POST "https://beehold-leaderboard.<your-subdomain>.workers.dev/clear" \
  -H "X-Beehold-Key: YOUR_KEY_HERE"
```

Then hit `/top` again — you should see the entry (or an empty list after `/clear`).

---

## Local preview (optional)

```bash
cd leaderboard
npx wrangler dev
```

Uses the preview KV binding. Point `HEX_LEADERBOARD_URL` at the printed `http://127.0.0.1:8787` URL only for local browser tests (itch still needs the deployed HTTPS URL).

---

## Notes

- The submit key is embedded in the web build (same trust model as dreamlo’s private code). It only stops casual spam, not a determined cheater.
- Free Workers + KV quotas are plenty for a jam game leaderboard.
- After changing the Worker code, run `npx wrangler deploy` again from `leaderboard/`.
