/**********************************************************************************************
*
*   raylib - Advance Game template
*
*   Ending Screen Functions Definitions (Init, Update, Draw, Unload)
*
*   Copyright (c) 2014-2022 Ramon Santamaria (@raysan5)
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "hex_scores.h"

#include <stdio.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;
static float bestTimes[HEX_SCORES_MAX] = { 0 };
static int bestCount = 0;

//----------------------------------------------------------------------------------
// Ending Screen Functions Definition
//----------------------------------------------------------------------------------
void InitEndingScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;
    bestCount = HexScoresLoad(bestTimes, HEX_SCORES_MAX, lastRun.hardcore);
}

void UpdateEndingScreen(void)
{
    if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
    {
        finishScreen = 1;
        PlaySound(fxCoin);
    }
}

void DrawEndingScreen(void)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){ 24, 28, 36, 255 });

    const char *title = lastRun.won? "MEADOW COMPLETE"
                                   : (lastRun.hardcore? "HARDCORE FAILED" : "YOU LOSE! TRY AGAIN?");
    Color titleCol = lastRun.won? (Color){ 255, 179, 71, 255 } : (Color){ 255, 110, 100, 255 };
    int titleSize = lastRun.won? 40 : 34;
    DrawText(title, (sw - MeasureText(title, titleSize))/2, 28, titleSize, titleCol);

    char totalLine[64];
    char timeBuf[16];
    HexScoresFormat(lastRun.totalTime, timeBuf, (int)sizeof(timeBuf));
    snprintf(totalLine, sizeof(totalLine), "Total  %s", timeBuf);
    DrawText(totalLine, (sw - MeasureText(totalLine, 22))/2, 72, 22, RAYWHITE);

    const char *header = "LEVEL    TIME";
    int headerY = 112;
    DrawText(header, (sw - MeasureText(header, 18))/2, headerY, 18, (Color){ 160, 170, 180, 255 });

    int n = lastRun.levelsRecorded;
    if (n <= 0)
    {
        const char *empty = "No level data";
        DrawText(empty, (sw - MeasureText(empty, 20))/2, sh/2, 20, LIGHTGRAY);
    }
    else
    {
        int lineH = 22;
        int listTop = headerY + 28;
        int listBottom = sh - 150;
        int maxVisible = (listBottom - listTop)/lineH;
        if (maxVisible < 1) maxVisible = 1;

        int start = 0;
        if (n > maxVisible) start = n - maxVisible;  // show latest levels if overflow

        for (int i = start; i < n; i++)
        {
            char t[16];
            HexScoresFormat(lastRun.levels[i].timeSec, t, (int)sizeof(t));
            char line[64];
            snprintf(line, sizeof(line), "%2d     %8s", i + 1, t);

            int y = listTop + (i - start)*lineH;
            Color col = RAYWHITE;
            if (!lastRun.won && (i == n - 1)) col = (Color){ 255, 160, 140, 255 };
            DrawText(line, (sw - MeasureText(line, 20))/2, y, 20, col);
        }
    }

    // Compact best-times strip near the bottom (wins only matter for board, but always show)
    const char *boardTitle = lastRun.hardcore? "BEST HARDCORE TIMES" : "BEST TOTAL TIMES";
    int boardY = sh - 130;
    DrawText(boardTitle, (sw - MeasureText(boardTitle, 16))/2, boardY, 16, (Color){ 180, 190, 200, 255 });

    if (bestCount <= 0)
    {
        const char *empty = "No records yet";
        DrawText(empty, (sw - MeasureText(empty, 16))/2, boardY + 22, 16, LIGHTGRAY);
    }
    else
    {
        char row[128];
        row[0] = '\0';
        int shown = (bestCount < 5)? bestCount : 5;
        for (int i = 0; i < shown; i++)
        {
            char t[16];
            HexScoresFormat(bestTimes[i], t, (int)sizeof(t));
            char piece[24];
            snprintf(piece, sizeof(piece), "%s%s", (i > 0)? "  " : "", t);
            strncat(row, piece, sizeof(row) - strlen(row) - 1);
        }
        DrawText(row, (sw - MeasureText(row, 16))/2, boardY + 22, 16, (Color){ 255, 220, 70, 255 });
    }

    const char *hint = lastRun.won? "ENTER / TAP  return to menu" : "ENTER / TAP  try again from menu";
    DrawText(hint, (sw - MeasureText(hint, 18))/2, sh - 40, 18, LIGHTGRAY);
}

void UnloadEndingScreen(void)
{
    // Nothing to unload
}

int FinishEndingScreen(void)
{
    return finishScreen;
}
