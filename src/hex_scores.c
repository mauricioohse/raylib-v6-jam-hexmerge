/**********************************************************************************************
*
*   Beehold - Persistent best-run times
*
**********************************************************************************************/

#include "hex_scores.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(PLATFORM_WEB)
    #include <emscripten.h>

    EM_JS(void, HexScoresJsSave, (const char *text), {
        try { localStorage.setItem("beehold_scores", UTF8ToString(text)); } catch (e) {}
    });

    EM_JS(char *, HexScoresJsLoad, (), {
        try {
            var s = localStorage.getItem("beehold_scores");
            if (!s) return 0;
            var len = lengthBytesUTF8(s) + 1;
            var buf = _malloc(len);
            stringToUTF8(s, buf, len);
            return buf;
        } catch (e) { return 0; }
    });
#endif

//----------------------------------------------------------------------------------
// Persistence
//----------------------------------------------------------------------------------
static char *LoadScoresText(void)
{
#if defined(PLATFORM_WEB)
    return HexScoresJsLoad();   // malloc'd; free with free()
#else
    return LoadFileText(HEX_SCORES_FILE);   // free with UnloadFileText()
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
    HexScoresJsSave(text);
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

    // Insert keeping ascending order; drop anything beyond the top N
    int insertAt = count;
    for (int i = 0; i < count; i++)
    {
        if (seconds < times[i]) { insertAt = i; break; }
    }

    if (insertAt >= HEX_SCORES_MAX) return 0;   // not a top score

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
