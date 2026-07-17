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
#include "hex_assets.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static HexRunResult viewRun = { 0 };
static HexRunResult submitRun = { 0 };   // snapshot of the finished run only (never best-run)
static bool viewingBestFromMenu = false;
static bool hasViewRun = false;
static bool canSubmitThisRun = false;

static bool wantGlobalSubmit = false;
static bool namePromptActive = false;
static char nameBuf[HEX_PLAYER_NAME_MAX + 1] = { 0 };
static char submittedName[HEX_PLAYER_NAME_MAX + 1] = { 0 };

static HexGlobalEntry globalTop[HEX_GLOBAL_TOP_MAX] = { 0 };
static int globalTopCount = 0;
static bool globalFetchStarted = false;
static int refetchDelayFrames = 0;  // wait after submit before first quiet refetch
static int pollFrames = 0;         // quiet auto-refresh timer

#define RATING_STAR_SCALE 1.25f
#define GLOBAL_REFETCH_DELAY_FRAMES 120  // ~2s at 60fps after submit
#define GLOBAL_POLL_FRAMES 180           // ~3s quiet refresh while on scores screen

//----------------------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------------------
// Only show loading when we have nothing to display yet (never blink a full table).
static bool IsGlobalBoardLoading(void)
{
    if (globalTopCount > 0) return false;
    return (refetchDelayFrames > 0) || HexScoresGlobalFetchPending();
}

static void StartGlobalFetch(bool clearDisplay)
{
    globalFetchStarted = true;
    if (clearDisplay) globalTopCount = 0;
    refetchDelayFrames = 0;
    pollFrames = 0;
    HexScoresFetchGlobalTop(HEX_GLOBAL_TOP_MAX);
}

static void PumpGlobalFetch(void)
{
    if (refetchDelayFrames > 0)
    {
        refetchDelayFrames--;
        if (refetchDelayFrames == 0) StartGlobalFetch(false);
        return;
    }

    if (!globalFetchStarted) return;

    // Keep showing the previous table while a refresh is in flight
    if (HexScoresGlobalFetchPending()) return;

    if (HexScoresGlobalFetchReady())
        globalTopCount = HexScoresCopyGlobalTop(globalTop, HEX_GLOBAL_TOP_MAX);

    pollFrames++;
    if (pollFrames >= GLOBAL_POLL_FRAMES)
        StartGlobalFetch(false);
}

static void TrySubmitName(void)
{
    char clean[HEX_PLAYER_NAME_MAX + 1];
#if defined(PLATFORM_WEB)
    HexScoresNamePromptRead(clean, (int)sizeof(clean));
#else
    // Desktop: sanitize whatever was typed into nameBuf
    int n = 0;
    for (int i = 0; nameBuf[i] && (n < HEX_PLAYER_NAME_MAX); i++)
    {
        char c = nameBuf[i];
        if (isalnum((unsigned char)c)) clean[n++] = c;
    }
    clean[n] = '\0';
#endif
    if (clean[0] == '\0') return;

    strncpy(submittedName, clean, HEX_PLAYER_NAME_MAX);
    submittedName[HEX_PLAYER_NAME_MAX] = '\0';
    // Always post the finished run snapshot — never the stored personal-best file
    HexScoresSubmitGlobalNamed(&submitRun, submittedName);
    HexScoresNamePromptHide();
    namePromptActive = false;
    // Keep current table visible; quiet-refetch after a short delay, then every 3s
    pollFrames = 0;
    refetchDelayFrames = GLOBAL_REFETCH_DELAY_FRAMES;
    PlaySound(assets.fxCoin);
}

static void DrawGlobalBoard(int sw, int sh)
{
    const char *title = "GLOBAL TOP 20";
    int boardX = sw/2 + 20;
    int y = 70;
    DrawText(title, boardX, y, 20, (Color){ 255, 179, 71, 255 });
    y += 28;

    PumpGlobalFetch();

    if (IsGlobalBoardLoading())
    {
        DrawText("Loading scores...", boardX, y, 20, LIGHTGRAY);
    }
    else if (globalTopCount <= 0)
    {
        DrawText("No scores yet", boardX, y, 20, LIGHTGRAY);
    }
    else
    {
        for (int i = 0; i < globalTopCount; i++)
        {
            char timeBuf[16];
            HexScoresFormat(globalTop[i].timeSec, timeBuf, (int)sizeof(timeBuf));
            char line[64];
            snprintf(line, sizeof(line), "%2d  %-10s  %s  *%d",
                     i + 1, globalTop[i].name, timeBuf, globalTop[i].stars);
            Color col = RAYWHITE;
            if (submittedName[0] && (strcmp(globalTop[i].name, submittedName) == 0))
                col = (Color){ 255, 220, 70, 255 };
            DrawText(line, boardX, y, 20, col);
            y += 22;
        }
    }

    // Your score comparison strip (always when we have a run to compare)
    if (hasViewRun && (viewRun.won || viewingBestFromMenu))
    {
        int by = sh - 100;
        const char *label = viewingBestFromMenu? "YOUR BEST" : "YOUR SCORE";
        DrawText(label, boardX, by, 20, (Color){ 180, 190, 200, 255 });
        char timeBuf[16];
        HexScoresFormat(viewRun.totalTime, timeBuf, (int)sizeof(timeBuf));
        char line[64];
        if (submittedName[0])
            snprintf(line, sizeof(line), "%-10s  %s  *%d",
                     submittedName, timeBuf, viewRun.totalStars);
        else
            snprintf(line, sizeof(line), "%s  *%d", timeBuf, viewRun.totalStars);
        DrawText(line, boardX, by + 24, 20, (Color){ 255, 220, 70, 255 });
    }
}

//----------------------------------------------------------------------------------
// Ending Screen Functions Definition
//----------------------------------------------------------------------------------
void InitEndingScreen(void)
{
    framesCounter = 0;

    viewingBestFromMenu = endingFromMenu;
    endingFromMenu = false;
    memset(&viewRun, 0, sizeof(viewRun));
    memset(&submitRun, 0, sizeof(submitRun));
    hasViewRun = false;
    canSubmitThisRun = false;
    wantGlobalSubmit = false;
    namePromptActive = false;
    nameBuf[0] = '\0';
    submittedName[0] = '\0';
    globalTopCount = 0;
    globalFetchStarted = false;
    refetchDelayFrames = 0;
    pollFrames = 0;
    HexScoresNamePromptHide();

    if (viewingBestFromMenu)
        hasViewRun = HexScoresLoadBestRun(&viewRun);
    else
    {
        // Freeze the run we just finished for both display and global submit
        submitRun = lastRun;
        viewRun = lastRun;
        hasViewRun = (viewRun.levelsRecorded > 0) || viewRun.won;
        canSubmitThisRun = HexScoresCanSubmitGlobal(&submitRun);
    }

    StartGlobalFetch(true);

    wantGlobalSubmit = canSubmitThisRun;
    if (wantGlobalSubmit)
    {
        HexScoresLoadPlayerName(nameBuf, (int)sizeof(nameBuf));
        namePromptActive = true;
        HexScoresNamePromptShow(nameBuf);
    }

    float sw = (float)GetScreenWidth();
    float iconSize = HEX_SOCIAL_ICON_SIZE;
    float gap = HEX_SOCIAL_ICON_GAP;
    float total = iconSize*3.0f + gap*2.0f;
    HexSocialLayoutAt(sw - 48.0f - total, (float)GetScreenHeight() - 56.0f);
}

void UpdateEndingScreen(void)
{
    framesCounter++;
    PumpGlobalFetch();

    if (namePromptActive)
    {
#if defined(PLATFORM_WEB)
        if (HexScoresNamePromptEnterPressed())
            TrySubmitName();
#else
        // Desktop name entry
        int ch = GetCharPressed();
        while (ch > 0)
        {
            if (isalnum(ch))
            {
                int len = (int)strlen(nameBuf);
                if (len < HEX_PLAYER_NAME_MAX)
                {
                    nameBuf[len] = (char)ch;
                    nameBuf[len + 1] = '\0';
                }
            }
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE))
        {
            int len = (int)strlen(nameBuf);
            if (len > 0) nameBuf[len - 1] = '\0';
        }
        if (IsKeyPressed(KEY_ENTER))
            TrySubmitName();
#endif
        return;  // don't leave screen while naming
    }

    if (HexSocialUpdate())
    {
        PlaySound(assets.fxCoin);
        return;
    }

    if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
    {
        PlaySound(assets.fxCoin);
        TransitionToScreen(TITLE);
    }
}

void DrawEndingScreen(void)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){ 24, 28, 36, 255 });

    const char *title = "BEST RUN";
    Color titleCol = (Color){ 255, 179, 71, 255 };
    if (!viewingBestFromMenu)
    {
        title = viewRun.won? "MEADOW COMPLETE" : "YOU LOSE! TRY AGAIN?";
        titleCol = viewRun.won? (Color){ 255, 179, 71, 255 } : (Color){ 255, 110, 100, 255 };
    }
    else if (!hasViewRun)
    {
        title = "SCORES";
    }

    int titleSize = 40;
    DrawText(title, (sw - MeasureText(title, titleSize))/2, 20, titleSize, titleCol);

    // Left: local run summary + per-level stars
    int listX = 24;
    int listTop = 70;
    if (hasViewRun)
    {
        char timeBuf[16];
        HexScoresFormat(viewRun.totalTime, timeBuf, (int)sizeof(timeBuf));
        char totalLine[64];
        snprintf(totalLine, sizeof(totalLine), "Time  %s", timeBuf);
        DrawText(totalLine, listX, listTop, 20, RAYWHITE);
        char starsLine[64];
        snprintf(starsLine, sizeof(starsLine), "Stars  %d", viewRun.totalStars);
        DrawText(starsLine, listX, listTop + 24, 20, (Color){ 255, 220, 70, 255 });

        DrawText("LEVEL", listX, listTop + 56, 20, (Color){ 160, 170, 180, 255 });
        int y = listTop + 80;
        int n = viewRun.levelsRecorded;
        int maxVisible = (sh - 160 - y)/22;
        if (maxVisible < 1) maxVisible = 1;
        int start = 0;
        if (n > maxVisible) start = n - maxVisible;

        float starSize = (float)((assets.ratingStar.id != 0)? assets.ratingStar.width : 16)*RATING_STAR_SCALE;
        for (int i = start; i < n; i++)
        {
            char label[16];
            snprintf(label, sizeof(label), "%2d", i + 1);
            DrawText(label, listX, y, 20, RAYWHITE);
            float starY = (float)y + (20.0f - starSize)*0.5f;
            HexScoresDrawLevelStars(assets.ratingStar, assets.ratingStarEmpty, viewRun.levels[i].stars,
                                    (float)(listX + 40), starY, RATING_STAR_SCALE);
            y += 22;
        }
    }
    else
    {
        const char *empty = viewingBestFromMenu? "No best run yet"
                                               : "No level data";
        DrawText(empty, listX, listTop, 20, LIGHTGRAY);
    }

    // Right: global board
    DrawGlobalBoard(sw, sh);

    // Name prompt UI chrome (HTML input sits on top on web)
    if (namePromptActive)
    {
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.45f));
        const char *prompt = "ENTER YOUR NAME";
        DrawText(prompt, (sw - MeasureText(prompt, 30))/2, sh/2 - 60, 30, (Color){ 255, 179, 71, 255 });
        const char *hint = "Letters & numbers only — ENTER to submit";
        DrawText(hint, (sw - MeasureText(hint, 20))/2, sh/2 - 20, 20, LIGHTGRAY);
#if !defined(PLATFORM_WEB)
        // Desktop typed name preview
        char shown[32];
        snprintf(shown, sizeof(shown), "%s_", nameBuf);
        DrawText(shown, (sw - MeasureText(shown, 30))/2, sh/2 + 20, 30, RAYWHITE);
#endif
    }
    else
    {
        const char *hint = "ENTER / TAP  return to menu";
        DrawText(hint, (sw - MeasureText(hint, 20))/2, sh - 40, 20, LIGHTGRAY);
        HexSocialDraw();
    }
}

void UnloadEndingScreen(void)
{
    HexScoresNamePromptHide();
}
