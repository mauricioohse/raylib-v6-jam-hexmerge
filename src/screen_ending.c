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
#include "hex_social.h"

#include <stdio.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;
static float bestTimes[HEX_SCORES_MAX] = { 0 };
static int bestCount = 0;
static Texture2D ratingStarTex = { 0 };
static Texture2D ratingStarEmptyTex = { 0 };

#define RATING_STAR_SCALE 1.25f

//----------------------------------------------------------------------------------
// Ending Screen Functions Definition
//----------------------------------------------------------------------------------
void InitEndingScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;
    bestCount = HexScoresLoad(bestTimes, HEX_SCORES_MAX);

    ratingStarTex = LoadTexture("resources/rating_star.png");
    ratingStarEmptyTex = LoadTexture("resources/rating_star_empty.png");
    SetTextureFilter(ratingStarTex, TEXTURE_FILTER_POINT);
    SetTextureFilter(ratingStarEmptyTex, TEXTURE_FILTER_POINT);

    float sw = (float)GetScreenWidth();
    float iconSize = HEX_SOCIAL_ICON_SIZE;
    float gap = HEX_SOCIAL_ICON_GAP;
    float total = iconSize*3.0f + gap*2.0f;
    HexSocialLayoutAt(sw - 48.0f - total, (float)(112 + 28 + 40 + 24*2 + 16));
}

void UpdateEndingScreen(void)
{
    if (HexSocialUpdate())
    {
        PlaySound(fxCoin);
        return;
    }

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

    const char *title = lastRun.won? "MEADOW COMPLETE" : "YOU LOSE! TRY AGAIN?";
    Color titleCol = lastRun.won? (Color){ 255, 179, 71, 255 } : (Color){ 255, 110, 100, 255 };
    int titleSize = 40;
    DrawText(title, (sw - MeasureText(title, titleSize))/2, 20, titleSize, titleCol);

    char timeBuf[16];
    HexScoresFormat(lastRun.totalTime, timeBuf, (int)sizeof(timeBuf));

    char totalLine[64];
    snprintf(totalLine, sizeof(totalLine), "Time  %s", timeBuf);
    DrawText(totalLine, (sw - MeasureText(totalLine, 20))/2, 70, 20, RAYWHITE);

    char starsLine[64];
    snprintf(starsLine, sizeof(starsLine), "Stars  %d", lastRun.totalStars);
    DrawText(starsLine, (sw - MeasureText(starsLine, 20))/2, 96, 20, (Color){ 255, 220, 70, 255 });

    const char *header = "LEVEL";
    int headerY = 130;
    int listX = 48;
    DrawText(header, listX, headerY, 20, (Color){ 160, 170, 180, 255 });

    int n = lastRun.levelsRecorded;
    int listTop = headerY + 26;
    if (n <= 0)
    {
        const char *empty = "No level data";
        DrawText(empty, listX, sh/2, 20, LIGHTGRAY);
    }
    else
    {
        int listBottom = sh - 150;
        int lineH = 22;
        int maxVisible = (listBottom - listTop)/lineH;
        if (maxVisible < 1) maxVisible = 1;

        int start = 0;
        if (n > maxVisible) start = n - maxVisible;

        float starSize = (float)((ratingStarTex.id != 0)? ratingStarTex.width : 16)*RATING_STAR_SCALE;
        for (int i = start; i < n; i++)
        {
            int y = listTop + (i - start)*lineH;

            char label[16];
            snprintf(label, sizeof(label), "%2d", i + 1);
            Color tint = RAYWHITE;
            if (!lastRun.won && (i == n - 1)) tint = (Color){ 255, 160, 140, 255 };
            DrawText(label, listX, y, 20, tint);

            float starY = (float)y + (20.0f - starSize)*0.5f;
            HexScoresDrawLevelStars(ratingStarTex, ratingStarEmptyTex, lastRun.levels[i].stars,
                                    (float)(listX + 40), starY, RATING_STAR_SCALE);
        }
    }

    {
        const char *line1 = "Enjoyed the game?";
        const char *line2 = "follow mohselabs on X!";
        int fontSize = 20;
        int lineH = fontSize + 6;
        int rightPad = 48;
        int x1 = sw - rightPad - MeasureText(line1, fontSize);
        int x2 = sw - rightPad - MeasureText(line2, fontSize);
        int y = listTop + 40;
        Color yellow = (Color){ 255, 220, 70, 255 };
        DrawText(line1, x1, y, fontSize, yellow);
        DrawText(line2, x2, y + lineH, fontSize, yellow);

        float iconSize = HEX_SOCIAL_ICON_SIZE;
        float gap = HEX_SOCIAL_ICON_GAP;
        float total = iconSize*3.0f + gap*2.0f;
        HexSocialLayoutAt((float)sw - (float)rightPad - total, (float)(y + lineH*2 + 16));
    }

    HexSocialDraw();

    const char *boardTitle = "BEST TOTAL TIMES";
    int boardY = sh - 130;
    DrawText(boardTitle, (sw - MeasureText(boardTitle, 20))/2, boardY, 20, (Color){ 180, 190, 200, 255 });

    if (bestCount <= 0)
    {
        const char *empty = "No records yet";
        DrawText(empty, (sw - MeasureText(empty, 20))/2, boardY + 24, 20, LIGHTGRAY);
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
        DrawText(row, (sw - MeasureText(row, 20))/2, boardY + 24, 20, (Color){ 255, 220, 70, 255 });
    }

    const char *hint = lastRun.won? "ENTER / TAP  return to menu" : "ENTER / TAP  try again from menu";
    DrawText(hint, (sw - MeasureText(hint, 20))/2, sh - 40, 20, LIGHTGRAY);

    if (lastRun.won)
    {
        const char *promo = "got a good score? post in the comments!";
        DrawText(promo, (sw - MeasureText(promo, 20))/2, sh - 68, 20, (Color){ 255, 220, 70, 255 });
    }
}

void UnloadEndingScreen(void)
{
    UnloadTexture(ratingStarTex);
    UnloadTexture(ratingStarEmptyTex);
    ratingStarTex = (Texture2D){ 0 };
    ratingStarEmptyTex = (Texture2D){ 0 };
}

int FinishEndingScreen(void)
{
    return finishScreen;
}
