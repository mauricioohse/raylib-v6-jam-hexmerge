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
        int n = snprintf(out, (int)cap, "run,won,level,time_sec\n");
        if (n > 0) pos = (size_t)n;
    }

    FreeRunsCsv(existing);

    for (int i = 0; i < run->levelsRecorded; i++)
    {
        if (pos + 96 >= cap) break;
        int n = snprintf(out + pos, (int)(cap - pos), "%ld,%d,%d,%.3f\n",
                         runId,
                         run->won? 1 : 0,
                         i + 1,
                         run->levels[i].timeSec);
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
