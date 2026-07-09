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
    bestCount = HexScoresLoad(bestTimes, HEX_SCORES_MAX);
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

    const char *title = "MEADOW COMPLETE";
    int titleSize = 40;
    DrawText(title, (sw - MeasureText(title, titleSize))/2, 48, titleSize, (Color){ 255, 179, 71, 255 });

    char yourTime[48];
    char timeBuf[16];
    HexScoresFormat(lastRunTime, timeBuf, (int)sizeof(timeBuf));
    snprintf(yourTime, sizeof(yourTime), "Your time  %s", timeBuf);
    DrawText(yourTime, (sw - MeasureText(yourTime, 28))/2, 110, 28, RAYWHITE);

    const char *boardTitle = "BEST TIMES";
    DrawText(boardTitle, (sw - MeasureText(boardTitle, 24))/2, 170, 24, (Color){ 200, 210, 220, 255 });

    if (bestCount <= 0)
    {
        const char *empty = "No records yet";
        DrawText(empty, (sw - MeasureText(empty, 20))/2, sh/2, 20, LIGHTGRAY);
    }
    else
    {
        int lineH = 28;
        int blockH = bestCount*lineH;
        int y0 = (sh - blockH)/2;
        if (y0 < 210) y0 = 210;

        for (int i = 0; i < bestCount; i++)
        {
            char line[48];
            char t[16];
            HexScoresFormat(bestTimes[i], t, (int)sizeof(t));
            snprintf(line, sizeof(line), "%2d.  %s", i + 1, t);

            Color col = (i == 0)? (Color){ 255, 220, 70, 255 } : RAYWHITE;
            DrawText(line, (sw - MeasureText(line, 22))/2, y0 + i*lineH, 22, col);
        }
    }

    const char *hint = "ENTER / TAP  return to menu";
    DrawText(hint, (sw - MeasureText(hint, 18))/2, sh - 48, 18, LIGHTGRAY);
}

void UnloadEndingScreen(void)
{
    // Nothing to unload
}

int FinishEndingScreen(void)
{
    return finishScreen;
}
