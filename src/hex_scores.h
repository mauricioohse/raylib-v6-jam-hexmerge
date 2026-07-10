/**********************************************************************************************
*
*   Beehold - Run stats + CSV history (desktop file + emscripten localStorage)
*
**********************************************************************************************/

#ifndef HEX_SCORES_H
#define HEX_SCORES_H

#include <stdbool.h>

#define HEX_SCORES_MAX 10
#define HEX_SCORES_FILE "beehold_scores.txt"
#define HEX_SCORES_HARDCORE_FILE "beehold_scores_hardcore.txt"
#define HEX_RUNS_CSV_FILE "beehold_runs.csv"
#define HEX_RUN_MAX_LEVELS 20

typedef struct HexLevelResult
{
    float timeSec;
} HexLevelResult;

typedef struct HexRunResult
{
    bool won;
    bool hardcore;
    int levelsRecorded;     // how many level slots have data
    HexLevelResult levels[HEX_RUN_MAX_LEVELS];
    float totalTime;
} HexRunResult;

// Load sorted best total times. hardcore selects the hardcore board.
int HexScoresLoad(float *outTimes, int maxCount, bool hardcore);

// Insert a finished total run time. Returns rank 1..n, or 0 if not top.
int HexScoresSubmit(float seconds, bool hardcore);

// Append this run's per-level rows to beehold_runs.csv (localStorage on web).
void HexScoresAppendRunCsv(const HexRunResult *run);

// Format seconds as M:SS.CC into buf (bufSize >= 16).
void HexScoresFormat(float seconds, char *buf, int bufSize);

#endif // HEX_SCORES_H
