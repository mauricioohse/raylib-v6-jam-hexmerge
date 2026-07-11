/**********************************************************************************************
*
*   Beehold - Run stats + CSV history + dreamlo global board
*
**********************************************************************************************/

#ifndef HEX_SCORES_H
#define HEX_SCORES_H

#include "raylib.h"

#include <stdbool.h>

#define HEX_SCORES_MAX 10
#define HEX_SCORES_FILE "beehold_scores.txt"
#define HEX_BEST_RUN_FILE "beehold_best_run.txt"
#define HEX_RUNS_CSV_FILE "beehold_runs.csv"
#define HEX_RUN_MAX_LEVELS 32
#define HEX_LEVEL_STARS_MAX 3
#define HEX_PLAYER_NAME_MAX 16
#define HEX_GLOBAL_TOP_MAX 10

// Set to 1 to force-send dreamlo scores even in _DEBUG or with time < 60s (testing).
#ifndef DEBUG_TEST_SCORE
#define DEBUG_TEST_SCORE 0
#endif

// dreamlo private code (add + read via /pipe). HTTP only for this board.
#define HEX_DREAMLO_PRIVATE "x49B2n34kE2qdvJfjMcmLgitvEWXnwH06lya-wqv2vcQ"
#define HEX_DREAMLO_MIN_TIME_SEC 60.0f
#define HEX_DREAMLO_MIN_STARS 24  // 1* per level across the full campaign
#define HEX_DREAMLO_RUN_ID_LEN 10
#define HEX_DREAMLO_NAME_MAX (HEX_PLAYER_NAME_MAX + 1 + HEX_DREAMLO_RUN_ID_LEN)

typedef struct HexLevelResult
{
    float timeSec;
    int stars;              // 1..HEX_LEVEL_STARS_MAX
} HexLevelResult;

typedef struct HexRunResult
{
    bool won;
    int levelsRecorded;     // how many level slots have data
    HexLevelResult levels[HEX_RUN_MAX_LEVELS];
    float totalTime;
    int totalStars;
} HexRunResult;

typedef struct HexGlobalEntry
{
    char name[HEX_PLAYER_NAME_MAX + 1];
    float timeSec;
    int stars;
} HexGlobalEntry;

// Stars earned for a level: 3 minus deaths, clamped to [1, HEX_LEVEL_STARS_MAX].
int HexScoresStarsFromDeaths(int deaths);

// Load sorted best total times.
int HexScoresLoad(float *outTimes, int maxCount);

// Insert a finished total run time. Returns rank 1..n, or 0 if not top.
int HexScoresSubmit(float seconds);

// Full best-run breakdown (for scores screen). Returns false if none saved.
bool HexScoresLoadBestRun(HexRunResult *out);

// Save run as the best if it wins and beats the stored best time (or none exists).
void HexScoresSaveBestRunIfBetter(const HexRunResult *run);

// True if this run is eligible to post to dreamlo (respects DEBUG_TEST_SCORE).
bool HexScoresCanSubmitGlobal(const HexRunResult *run);

// Submit a named winning run to dreamlo (web). Posts as name-########## so each
// run is a unique entry; the UI strips the suffix when displaying.
void HexScoresSubmitGlobalNamed(const HexRunResult *run, const char *name);

// Async fetch top N (web). Poll HexScoresGlobalFetchReady().
void HexScoresFetchGlobalTop(int count);
bool HexScoresGlobalFetchPending(void);
bool HexScoresGlobalFetchReady(void);
int HexScoresCopyGlobalTop(HexGlobalEntry *out, int maxCount);

// Saved player name (localStorage on web / soft default on desktop).
void HexScoresLoadPlayerName(char *buf, int bufSize);
void HexScoresSavePlayerName(const char *name);

// Web HTML name field (no-ops on desktop — use GetCharPressed there).
void HexScoresNamePromptShow(const char *initial);
void HexScoresNamePromptHide(void);
bool HexScoresNamePromptEnterPressed(void);
void HexScoresNamePromptRead(char *buf, int bufSize);  // sanitized A-Za-z0-9

// Append this run's per-level rows to beehold_runs.csv (localStorage on web).
void HexScoresAppendRunCsv(const HexRunResult *run);

// Format seconds as M:SS.CC into buf (bufSize >= 16).
void HexScoresFormat(float seconds, char *buf, int bufSize);

// Draw 1..3 rating stars (filled + empty). Falls back to '*' text if textures missing.
void HexScoresDrawLevelStars(Texture2D filled, Texture2D empty, int stars, float x, float y, float scale);

#endif // HEX_SCORES_H
