/**********************************************************************************************
*
*   Beehold - Persistent best-run times (desktop file + emscripten localStorage)
*
**********************************************************************************************/

#ifndef HEX_SCORES_H
#define HEX_SCORES_H

#define HEX_SCORES_MAX 10
#define HEX_SCORES_FILE "beehold_scores.txt"

// Load sorted best times (ascending seconds). Returns count written to outTimes.
int HexScoresLoad(float *outTimes, int maxCount);

// Insert a finished run time, keep sorted best, persist. Returns new rank 1..n, or 0 if not top.
int HexScoresSubmit(float seconds);

// Format seconds as M:SS.CC into buf (bufSize >= 16).
void HexScoresFormat(float seconds, char *buf, int bufSize);

#endif // HEX_SCORES_H
