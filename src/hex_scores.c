/**********************************************************************************************
*
*   Beehold - Persistent best-run times + CSV run history
*
**********************************************************************************************/

#include "hex_scores.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(PLATFORM_WEB)
    #include <emscripten.h>

    EM_JS(void, HexScoresJsSaveKey, (const char *key, const char *text), {
        try { localStorage.setItem(UTF8ToString(key), UTF8ToString(text)); } catch (e) {}
    });

    EM_JS(char *, HexScoresJsLoadKey, (const char *key), {
        try {
            var s = localStorage.getItem(UTF8ToString(key));
            if (!s) return 0;
            var len = lengthBytesUTF8(s) + 1;
            var buf = _malloc(len);
            stringToUTF8(s, buf, len);
            return buf;
        } catch (e) { return 0; }
    });

    EM_JS(void, HexRunsJsSave, (const char *text), {
        try { localStorage.setItem("beehold_runs_csv", UTF8ToString(text)); } catch (e) {}
    });

    EM_JS(char *, HexRunsJsLoad, (), {
        try {
            var s = localStorage.getItem("beehold_runs_csv");
            if (!s) return 0;
            var len = lengthBytesUTF8(s) + 1;
            var buf = _malloc(len);
            stringToUTF8(s, buf, len);
            return buf;
        } catch (e) { return 0; }
    });

    // Fire-and-forget POST to Cloudflare Worker. name is URL-safe (A-Za-z0-9-##########).
    EM_JS(void, HexLbJsSubmit, (const char *baseUrl, const char *apiKey, const char *name, int centiseconds, int stars), {
        try {
            var url = UTF8ToString(baseUrl);
            while (url.length > 0 && url.charAt(url.length - 1) === "/") url = url.slice(0, -1);
            url = url + "/submit";
            fetch(url, {
                method: "POST",
                mode: "cors",
                cache: "no-store",
                headers: {
                    "Content-Type": "application/json",
                    "X-Beehold-Key": UTF8ToString(apiKey)
                },
                body: JSON.stringify({
                    name: UTF8ToString(name),
                    centiseconds: centiseconds | 0,
                    stars: stars | 0
                })
            }).catch(function () {});
        } catch (e) {}
    });

    EM_JS(void, HexLbJsSaveName, (const char *name), {
        try { localStorage.setItem("beehold_player_name", UTF8ToString(name)); } catch (e) {}
    });

    EM_JS(char *, HexLbJsLoadName, (), {
        try {
            var s = localStorage.getItem("beehold_player_name");
            if (!s) s = "";
            s = String(s).replace(/[^A-Za-z0-9]/g, "").slice(0, 16);
            var len = lengthBytesUTF8(s) + 1;
            var buf = _malloc(len);
            stringToUTF8(s, buf, len);
            return buf;
        } catch (e) {
            var buf = _malloc(1);
            HEAPU8[buf] = 0;
            return buf;
        }
    });

    EM_JS(void, HexLbJsFetchTop, (const char *baseUrl, int count), {
        try {
            globalThis.__beeholdPipeState = 1;
            globalThis.__beeholdPipeText = "";
            var url = UTF8ToString(baseUrl);
            while (url.length > 0 && url.charAt(url.length - 1) === "/") url = url.slice(0, -1);
            url = url + "/top?n=" + (count | 0);
            fetch(url, { method: "GET", mode: "cors", cache: "no-store" })
                .then(function (r) { return r.text(); })
                .then(function (t) {
                    try {
                        var j = JSON.parse(t || "{}");
                        var entries = j && j.entries;
                        if (!entries || !Array.isArray(entries)) {
                            globalThis.__beeholdPipeText = "";
                            globalThis.__beeholdPipeState = 2;
                            return;
                        }
                        var lines = [];
                        for (var i = 0; i < entries.length; i++) {
                            var e = entries[i] || {};
                            // name|score|centiseconds|stars|date  (score unused; matches old pipe parser)
                            lines.push([
                                e.name || "",
                                "0",
                                String(e.centiseconds != null ? e.centiseconds : 0),
                                String(e.stars != null ? e.stars : 0),
                                ""
                            ].join("|"));
                        }
                        globalThis.__beeholdPipeText = lines.join("\n");
                        globalThis.__beeholdPipeState = 2;
                    } catch (err) {
                        globalThis.__beeholdPipeText = "";
                        globalThis.__beeholdPipeState = 3;
                    }
                })
                .catch(function () {
                    globalThis.__beeholdPipeText = "";
                    globalThis.__beeholdPipeState = 3;
                });
        } catch (e) {
            globalThis.__beeholdPipeState = 3;
        }
    });

    EM_JS(int, HexLbJsFetchState, (), {
        return globalThis.__beeholdPipeState | 0;
    });

    EM_JS(char *, HexLbJsFetchText, (), {
        try {
            var s = globalThis.__beeholdPipeText || "";
            var len = lengthBytesUTF8(s) + 1;
            var buf = _malloc(len);
            stringToUTF8(s, buf, len);
            return buf;
        } catch (e) { return 0; }
    });

    EM_JS(void, HexNamePromptShow, (const char *initial), {
        try {
            var old = document.getElementById("beehold-name-wrap");
            if (old) old.remove();
            globalThis.__beeholdNameEnter = 0;

            var wrap = document.createElement("div");
            wrap.id = "beehold-name-wrap";
            wrap.style.cssText = "position:fixed;left:0;top:0;right:0;bottom:0;z-index:10000;pointer-events:none;";

            var inp = document.createElement("input");
            inp.id = "beehold-name";
            inp.type = "text";
            inp.maxLength = 16;
            inp.spellcheck = false;
            inp.autocomplete = "off";
            inp.value = UTF8ToString(initial || "");
            inp.placeholder = "YOURNAME";
            inp.style.cssText =
                "pointer-events:auto;position:fixed;left:50%;top:58%;transform:translate(-50%,-50%);" +
                "width:220px;height:36px;font-size:20px;font-family:monospace;text-align:center;" +
                "border:2px solid #ffb347;background:#1e2430;color:#fff;outline:none;border-radius:4px;";
            inp.addEventListener("input", function () {
                this.value = this.value.replace(/[^A-Za-z0-9]/g, "");
            });
            // Keep keys in the field — raylib/emscripten otherwise eats Backspace etc.
            function keepKeys(e) {
                e.stopPropagation();
                if (e.key === "Enter") {
                    e.preventDefault();
                    globalThis.__beeholdNameEnter = 1;
                }
            }
            inp.addEventListener("keydown", keepKeys);
            inp.addEventListener("keyup", keepKeys);
            inp.addEventListener("keypress", keepKeys);
            wrap.appendChild(inp);
            document.body.appendChild(wrap);
            setTimeout(function () {
                try {
                    var canvas = document.getElementById("canvas");
                    if (canvas) canvas.blur();
                } catch (err) {}
                inp.focus();
                inp.select();
            }, 30);
        } catch (e) {}
    });

    EM_JS(void, HexNamePromptHide, (), {
        try {
            var wrap = document.getElementById("beehold-name-wrap");
            if (wrap) wrap.remove();
            globalThis.__beeholdNameEnter = 0;
        } catch (e) {}
    });

    EM_JS(int, HexNamePromptEnterPressed, (), {
        if (globalThis.__beeholdNameEnter) {
            globalThis.__beeholdNameEnter = 0;
            return 1;
        }
        return 0;
    });

    EM_JS(char *, HexNamePromptRead, (), {
        try {
            var inp = document.getElementById("beehold-name");
            var s = inp ? String(inp.value || "") : "";
            s = s.replace(/[^A-Za-z0-9]/g, "").slice(0, 16);
            var len = lengthBytesUTF8(s) + 1;
            var buf = _malloc(len);
            stringToUTF8(s, buf, len);
            return buf;
        } catch (e) {
            var buf = _malloc(1);
            HEAPU8[buf] = 0;
            return buf;
        }
    });
#endif

//----------------------------------------------------------------------------------
// Best-times persistence
//----------------------------------------------------------------------------------
static char *LoadScoresText(void)
{
#if defined(PLATFORM_WEB)
    return HexScoresJsLoadKey("beehold_scores");
#else
    return LoadFileText(HEX_SCORES_FILE);
#endif
}

static void FreeScoresText(char *text)
{
    if (text == NULL) return;
#if defined(PLATFORM_WEB)
    free(text);
#else
    UnloadFileText(text);
#endif
}

static void SaveScoresText(const char *text)
{
#if defined(PLATFORM_WEB)
    HexScoresJsSaveKey("beehold_scores", text);
#else
    SaveFileText(HEX_SCORES_FILE, (char *)text);
#endif
}

static int ParseScores(const char *text, float *outTimes, int maxCount)
{
    if ((text == NULL) || (outTimes == NULL) || (maxCount <= 0)) return 0;

    int count = 0;
    const char *p = text;
    while ((*p != '\0') && (count < maxCount))
    {
        while ((*p == ' ') || (*p == '\t') || (*p == '\r') || (*p == '\n')) p++;
        if (*p == '\0') break;

        char *end = NULL;
        float v = strtof(p, &end);
        if (end == p) break;
        if (v > 0.0f) outTimes[count++] = v;
        p = end;
    }
    return count;
}

static void SortAscending(float *times, int count)
{
    for (int i = 1; i < count; i++)
    {
        float key = times[i];
        int j = i - 1;
        while ((j >= 0) && (times[j] > key))
        {
            times[j + 1] = times[j];
            j--;
        }
        times[j + 1] = key;
    }
}

//----------------------------------------------------------------------------------
// CSV run history
//----------------------------------------------------------------------------------
static char *LoadRunsCsv(void)
{
#if defined(PLATFORM_WEB)
    return HexRunsJsLoad();
#else
    return LoadFileText(HEX_RUNS_CSV_FILE);
#endif
}

static void FreeRunsCsv(char *text)
{
    if (text == NULL) return;
#if defined(PLATFORM_WEB)
    free(text);
#else
    UnloadFileText(text);
#endif
}

static void SaveRunsCsv(const char *text)
{
#if defined(PLATFORM_WEB)
    HexRunsJsSave(text);
#else
    SaveFileText(HEX_RUNS_CSV_FILE, (char *)text);
#endif
}

//----------------------------------------------------------------------------------
// Public API
//----------------------------------------------------------------------------------
int HexScoresStarsFromDeaths(int deaths)
{
    if (deaths < 0) deaths = 0;
    int stars = HEX_LEVEL_STARS_MAX - deaths;
    if (stars < 1) stars = 1;
    if (stars > HEX_LEVEL_STARS_MAX) stars = HEX_LEVEL_STARS_MAX;
    return stars;
}

int HexScoresLoad(float *outTimes, int maxCount)
{
    char *text = LoadScoresText();
    int count = ParseScores(text, outTimes, maxCount);
    FreeScoresText(text);
    SortAscending(outTimes, count);
    return count;
}

int HexScoresSubmit(float seconds)
{
    if (seconds <= 0.0f) return 0;

    float times[HEX_SCORES_MAX + 1] = { 0 };
    int count = HexScoresLoad(times, HEX_SCORES_MAX);

    int insertAt = count;
    for (int i = 0; i < count; i++)
    {
        if (seconds < times[i]) { insertAt = i; break; }
    }

    if (insertAt >= HEX_SCORES_MAX) return 0;

    for (int i = (count < HEX_SCORES_MAX)? count : (HEX_SCORES_MAX - 1); i > insertAt; i--)
    {
        times[i] = times[i - 1];
    }
    times[insertAt] = seconds;
    if (count < HEX_SCORES_MAX) count++;

    char buf[256];
    int pos = 0;
    for (int i = 0; i < count; i++)
    {
        int n = snprintf(buf + pos, (int)sizeof(buf) - pos, "%.3f\n", times[i]);
        if (n <= 0) break;
        pos += n;
        if (pos >= (int)sizeof(buf) - 1) break;
    }
    SaveScoresText(buf);

    return insertAt + 1;
}

static char *LoadBestRunText(void)
{
#if defined(PLATFORM_WEB)
    return HexScoresJsLoadKey("beehold_best_run");
#else
    return LoadFileText(HEX_BEST_RUN_FILE);
#endif
}

static void SaveBestRunText(const char *text)
{
#if defined(PLATFORM_WEB)
    HexScoresJsSaveKey("beehold_best_run", text);
#else
    SaveFileText(HEX_BEST_RUN_FILE, (char *)text);
#endif
}

bool HexScoresLoadBestRun(HexRunResult *out)
{
    if (out == NULL) return false;
    memset(out, 0, sizeof(*out));

    char *text = LoadBestRunText();
    if (text == NULL) return false;

    // Format:
    // won totalTime totalStars levels
    // time stars   (one line per level)
    const char *p = text;
    int won = 0;
    int levels = 0;
    if (sscanf(p, "%d %f %d %d", &won, &out->totalTime, &out->totalStars, &levels) < 4)
    {
        FreeScoresText(text);
        return false;
    }
    out->won = (won != 0);
    if (levels < 0) levels = 0;
    if (levels > HEX_RUN_MAX_LEVELS) levels = HEX_RUN_MAX_LEVELS;
    out->levelsRecorded = levels;

    // advance to next line
    while ((*p != '\0') && (*p != '\n')) p++;
    if (*p == '\n') p++;

    for (int i = 0; i < levels; i++)
    {
        float t = 0.0f;
        int stars = 1;
        if (sscanf(p, "%f %d", &t, &stars) < 2) break;
        if (stars < 1) stars = 1;
        if (stars > HEX_LEVEL_STARS_MAX) stars = HEX_LEVEL_STARS_MAX;
        out->levels[i].timeSec = t;
        out->levels[i].stars = stars;

        while ((*p != '\0') && (*p != '\n')) p++;
        if (*p == '\n') p++;
    }

    FreeScoresText(text);
    return (out->levelsRecorded > 0);
}

void HexScoresSaveBestRunIfBetter(const HexRunResult *run)
{
    if ((run == NULL) || !run->won || (run->levelsRecorded <= 0) || (run->totalTime <= 0.0f))
        return;

    HexRunResult existing = { 0 };
    bool hasBest = HexScoresLoadBestRun(&existing);
    if (hasBest && (run->totalTime >= existing.totalTime))
        return;

    // Header + one line per level; keep buffer generous for 32 levels
    char buf[2048];
    int pos = 0;
    int n = snprintf(buf, (int)sizeof(buf), "%d %.3f %d %d\n",
                     1, run->totalTime, run->totalStars, run->levelsRecorded);
    if (n <= 0) return;
    pos = n;

    for (int i = 0; i < run->levelsRecorded; i++)
    {
        n = snprintf(buf + pos, (int)sizeof(buf) - pos, "%.3f %d\n",
                     run->levels[i].timeSec, run->levels[i].stars);
        if (n <= 0) break;
        pos += n;
        if (pos >= (int)sizeof(buf) - 1) break;
    }

    SaveBestRunText(buf);
}

bool HexScoresCanSubmitGlobal(const HexRunResult *run)
{
    if ((run == NULL) || !run->won || (run->totalTime <= 0.0f)) return false;
#if !DEBUG_TEST_SCORE
    #if defined(_DEBUG)
        return false;
    #endif
    if (run->totalTime < HEX_GLOBAL_MIN_TIME_SEC) return false;
    if (run->totalStars < HEX_GLOBAL_MIN_STARS) return false;
#endif
    return true;
}

static void SanitizePlayerName(const char *in, char *out, int outSize)
{
    if ((out == NULL) || (outSize <= 0)) return;
    out[0] = '\0';
    if (in == NULL) return;

    int n = 0;
    for (const char *p = in; *p && (n < outSize - 1); p++)
    {
        char c = *p;
        if (((c >= 'A') && (c <= 'Z')) ||
            ((c >= 'a') && (c <= 'z')) ||
            ((c >= '0') && (c <= '9')))
        {
            out[n++] = c;
        }
    }
    out[n] = '\0';
}

// If in ends with -##########, copy the base name; otherwise copy in as-is.
static void StripGlobalRunId(const char *in, char *out, int outSize)
{
    if ((out == NULL) || (outSize <= 0)) return;
    out[0] = '\0';
    if (in == NULL) return;

    int len = (int)strlen(in);
    if (len > HEX_GLOBAL_RUN_ID_LEN + 1)
    {
        const char *dash = in + len - HEX_GLOBAL_RUN_ID_LEN - 1;
        if (*dash == '-')
        {
            bool allDigit = true;
            for (int i = 1; i <= HEX_GLOBAL_RUN_ID_LEN; i++)
            {
                char c = dash[i];
                if ((c < '0') || (c > '9')) { allDigit = false; break; }
            }
            if (allDigit)
            {
                int baseLen = (int)(dash - in);
                if (baseLen > outSize - 1) baseLen = outSize - 1;
                if (baseLen < 0) baseLen = 0;
                memcpy(out, in, (size_t)baseLen);
                out[baseLen] = '\0';
                return;
            }
        }
    }

    strncpy(out, in, (size_t)(outSize - 1));
    out[outSize - 1] = '\0';
}

static void MakeGlobalSubmitName(const char *baseName, char *out, int outSize)
{
    if ((out == NULL) || (outSize <= 0)) return;
    out[0] = '\0';
    if ((baseName == NULL) || (baseName[0] == '\0')) return;

    char id[HEX_GLOBAL_RUN_ID_LEN + 1];
    for (int i = 0; i < HEX_GLOBAL_RUN_ID_LEN; i++)
        id[i] = (char)('0' + GetRandomValue(0, 9));
    id[HEX_GLOBAL_RUN_ID_LEN] = '\0';

    snprintf(out, (size_t)outSize, "%s-%s", baseName, id);
}

void HexScoresLoadPlayerName(char *buf, int bufSize)
{
    if ((buf == NULL) || (bufSize <= 0)) return;
    buf[0] = '\0';
#if defined(PLATFORM_WEB)
    char *loaded = HexLbJsLoadName();
    if (loaded != NULL)
    {
        SanitizePlayerName(loaded, buf, bufSize);
        free(loaded);
    }
#else
    (void)bufSize;
#endif
}

void HexScoresSavePlayerName(const char *name)
{
    char clean[HEX_PLAYER_NAME_MAX + 1];
    SanitizePlayerName(name, clean, (int)sizeof(clean));
    if (clean[0] == '\0') return;
#if defined(PLATFORM_WEB)
    HexLbJsSaveName(clean);
#else
    (void)clean;
#endif
}

void HexScoresSubmitGlobalNamed(const HexRunResult *run, const char *name)
{
    if (!HexScoresCanSubmitGlobal(run)) return;

    char clean[HEX_PLAYER_NAME_MAX + 1];
    SanitizePlayerName(name, clean, (int)sizeof(clean));
    if (clean[0] == '\0') return;

#if defined(PLATFORM_WEB)
    int stars = run->totalStars;
    if (stars < 0) stars = 0;
    int centiseconds = (int)(run->totalTime*100.0f + 0.5f);
    if (centiseconds < 1) centiseconds = 1;

    HexScoresSavePlayerName(clean);

    char submitName[HEX_GLOBAL_NAME_MAX + 1];
    MakeGlobalSubmitName(clean, submitName, (int)sizeof(submitName));
    HexLbJsSubmit(HEX_LEADERBOARD_URL, HEX_LEADERBOARD_KEY, submitName, centiseconds, stars);
#else
    (void)run;
#endif
}

//----------------------------------------------------------------------------------
// Global leaderboard fetch
//----------------------------------------------------------------------------------
static HexGlobalEntry sGlobalTop[HEX_GLOBAL_TOP_MAX];
static int sGlobalTopCount = 0;
static int sGlobalFetchState = 0;   // 0 idle, 1 pending, 2 ready, 3 error

#if defined(PLATFORM_WEB)
static void ParseGlobalPipe(const char *text)
{
    sGlobalTopCount = 0;
    if (text == NULL) return;

    const char *p = text;
    while ((*p != '\0') && (sGlobalTopCount < HEX_GLOBAL_TOP_MAX))
    {
        while ((*p == '\r') || (*p == '\n')) p++;
        if (*p == '\0') break;
        if (strncmp(p, "ERROR", 5) == 0) break;

        char line[256];
        int n = 0;
        while ((*p != '\0') && (*p != '\n') && (*p != '\r') && (n < (int)sizeof(line) - 1))
            line[n++] = *p++;
        line[n] = '\0';
        while ((*p == '\n') || (*p == '\r')) p++;

        // name|score|centiseconds|stars|date|...
        char *fields[6] = { 0 };
        int fieldCount = 0;
        char *tok = line;
        fields[fieldCount++] = tok;
        for (char *c = line; *c && (fieldCount < 6); c++)
        {
            if (*c == '|')
            {
                *c = '\0';
                fields[fieldCount++] = c + 1;
            }
        }
        if (fieldCount < 4) continue;

        HexGlobalEntry *e = &sGlobalTop[sGlobalTopCount];
        memset(e, 0, sizeof(*e));

        // Strip optional -########## run id before sanitizing for display
        char stripped[HEX_GLOBAL_NAME_MAX + 1];
        StripGlobalRunId(fields[0], stripped, (int)sizeof(stripped));
        SanitizePlayerName(stripped, e->name, (int)sizeof(e->name));
        if (e->name[0] == '\0') continue;

        int cs = atoi(fields[2]);
        if (cs < 0) cs = 0;
        e->timeSec = (float)cs/100.0f;
        e->stars = atoi(fields[3]);
        if (e->stars < 0) e->stars = 0;
        sGlobalTopCount++;
    }
}
#endif

void HexScoresFetchGlobalTop(int count)
{
    if (count < 1) count = 1;
    if (count > HEX_GLOBAL_TOP_MAX) count = HEX_GLOBAL_TOP_MAX;
    sGlobalTopCount = 0;
    sGlobalFetchState = 1;
#if defined(PLATFORM_WEB)
    HexLbJsFetchTop(HEX_LEADERBOARD_URL, count);
#else
    (void)count;
    sGlobalFetchState = 3;
#endif
}

bool HexScoresGlobalFetchPending(void)
{
#if defined(PLATFORM_WEB)
    int st = HexLbJsFetchState();
    if ((sGlobalFetchState == 1) && (st == 2))
    {
        char *text = HexLbJsFetchText();
        ParseGlobalPipe(text);
        if (text) free(text);
        sGlobalFetchState = 2;
    }
    else if ((sGlobalFetchState == 1) && (st == 3))
    {
        sGlobalFetchState = 3;
    }
#endif
    return (sGlobalFetchState == 1);
}

bool HexScoresGlobalFetchReady(void)
{
    HexScoresGlobalFetchPending();  // pump async result
    return (sGlobalFetchState == 2);
}

int HexScoresCopyGlobalTop(HexGlobalEntry *out, int maxCount)
{
    if ((out == NULL) || (maxCount <= 0)) return 0;
    HexScoresGlobalFetchReady();
    int n = sGlobalTopCount;
    if (n > maxCount) n = maxCount;
    for (int i = 0; i < n; i++) out[i] = sGlobalTop[i];
    return n;
}

void HexScoresNamePromptShow(const char *initial)
{
#if defined(PLATFORM_WEB)
    HexNamePromptShow(initial? initial : "");
#else
    (void)initial;
#endif
}

void HexScoresNamePromptHide(void)
{
#if defined(PLATFORM_WEB)
    HexNamePromptHide();
#endif
}

bool HexScoresNamePromptEnterPressed(void)
{
#if defined(PLATFORM_WEB)
    return HexNamePromptEnterPressed() != 0;
#else
    return false;
#endif
}

void HexScoresNamePromptRead(char *buf, int bufSize)
{
    if ((buf == NULL) || (bufSize <= 0)) return;
    buf[0] = '\0';
#if defined(PLATFORM_WEB)
    char *raw = HexNamePromptRead();
    if (raw != NULL)
    {
        SanitizePlayerName(raw, buf, bufSize);
        free(raw);
    }
#endif
}

void HexScoresAppendRunCsv(const HexRunResult *run)
{
    if ((run == NULL) || (run->levelsRecorded <= 0)) return;

    long runId = (long)time(NULL);
    char *existing = LoadRunsCsv();

    size_t oldLen = (existing != NULL)? strlen(existing) : 0;
    size_t cap = oldLen + 64 + (size_t)run->levelsRecorded*96 + 8;
    char *out = (char *)malloc(cap);
    if (out == NULL)
    {
        FreeRunsCsv(existing);
        return;
    }

    size_t pos = 0;
    bool needHeader = (existing == NULL) || (oldLen == 0) ||
                      (strncmp(existing, "run,", 4) != 0);

    if (!needHeader && (existing != NULL))
    {
        memcpy(out, existing, oldLen);
        pos = oldLen;
        if ((pos > 0) && (out[pos - 1] != '\n')) out[pos++] = '\n';
    }
    else
    {
        int n = snprintf(out, (int)cap, "run,won,level,time_sec,stars\n");
        if (n > 0) pos = (size_t)n;
    }

    FreeRunsCsv(existing);

    for (int i = 0; i < run->levelsRecorded; i++)
    {
        if (pos + 96 >= cap) break;
        int n = snprintf(out + pos, (int)(cap - pos), "%ld,%d,%d,%.3f,%d\n",
                         runId,
                         run->won? 1 : 0,
                         i + 1,
                         run->levels[i].timeSec,
                         run->levels[i].stars);
        if (n <= 0) break;
        pos += (size_t)n;
    }
    out[pos] = '\0';

    SaveRunsCsv(out);
    free(out);
}

void HexScoresFormat(float seconds, char *buf, int bufSize)
{
    if ((buf == NULL) || (bufSize <= 0)) return;
    if (seconds < 0.0f) seconds = 0.0f;

    int totalCs = (int)(seconds*100.0f + 0.5f);
    int mins = totalCs/6000;
    int secs = (totalCs/100)%60;
    int cs = totalCs%100;
    snprintf(buf, bufSize, "%d:%02d.%02d", mins, secs, cs);
}

void HexScoresDrawLevelStars(Texture2D filled, Texture2D empty, int stars, float x, float y, float scale)
{
    if (stars < 1) stars = 1;
    if (stars > HEX_LEVEL_STARS_MAX) stars = HEX_LEVEL_STARS_MAX;
    if (scale <= 0.0f) scale = 1.0f;

    bool useTex = (filled.id != 0) && (empty.id != 0);
    if (useTex)
    {
        float size = (float)filled.width*scale;
        float gap = 4.0f*scale;
        for (int s = 0; s < HEX_LEVEL_STARS_MAX; s++)
        {
            Texture2D tex = (s < stars)? filled : empty;
            Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
            Rectangle dst = { x + (float)s*(size + gap), y, size, size };
            DrawTexturePro(tex, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
        }
        return;
    }

    // Text fallback if PNGs failed to load (e.g. stale web pack)
    int fontSize = 20;
    if (scale >= 1.5f) fontSize = 30;
    if (scale >= 2.5f) fontSize = 40;
    float gap = (float)fontSize + 6.0f;
    for (int s = 0; s < HEX_LEVEL_STARS_MAX; s++)
    {
        const char *glyph = "*";
        Color col = (s < stars)? (Color){ 255, 220, 70, 255 } : (Color){ 90, 98, 110, 255 };
        DrawText(glyph, (int)(x + (float)s*gap), (int)y, fontSize, col);
    }
}
