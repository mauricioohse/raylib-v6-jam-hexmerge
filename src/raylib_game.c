/*******************************************************************************************
*
*   raylib game template
*
*
*   Code licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2021-2026 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "screens.h"    // NOTE: Declares global (extern) variables and screens functions
#include "hex_assets.h"
#include "hex_social.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>      // Emscripten library
#endif

#include <stdio.h>                          // Required for: printf()
#include <stdlib.h>                         // Required for: 
#include <string.h>                         // Required for:

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

//----------------------------------------------------------------------------------
// Shared Variables Definition (global)
// NOTE: Those variables are shared between modules through screens.h
//----------------------------------------------------------------------------------
GameScreen currentScreen = TITLE;
Font font = { 0 };
Music music = { 0 };
Music musicStarPower = { 0 };
Sound fxCoin = { 0 };
Sound fxFail = { 0 };
Sound fxPaint = { 0 };
Sound fxWin = { 0 };
Sound fxLife = { 0 };
Sound fxCheckpoint = { 0 };
Sound fxZoom = { 0 };
Sound fxBeeZoom = { 0 };
#define ZOOM_ALIAS_COUNT 4
static Sound fxZoomAlias[ZOOM_ALIAS_COUNT] = { 0 };
static int fxZoomAliasIndex = 0;
Animation beeAnim = {0};
int volumeLevel = 5;
float lastRunTime = 0.0f;
HexRunResult lastRun = { 0 };
bool controllerMode = true;
bool endingFromMenu = false;

//----------------------------------------------------------------------------------
// Global Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 720;
static const int screenHeight = 720;

// Required variables to manage screen transitions (fade-in, fade-out)
static float transAlpha = 0.0f;
static bool onTransition = false;
static bool transFadeOut = false;
static int transFromScreen = -1;
static GameScreen transToScreen = UNKNOWN;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void ChangeToScreen(int screen);     // Change to screen, no transition effect
static void UpdateTransition(void);         // Update transition effect
static void DrawTransition(void);           // Draw transition effect (full-screen rectangle)

static void UpdateDrawFrame(void);          // Update and draw one frame

//----------------------------------------------------------------------------------
// Program main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "Beehold");

    InitAudioDevice();      // Initialize audio device

    HexAssetsLoad();

    // Load global data (assets that must be available in all screens, i.e. font)
    font = LoadFont("resources/mecha.png");
    music = LoadMusicStream("resources/beehold_theme.wav");
    music.looping = true;
    musicStarPower = LoadMusicStream("resources/star_power.wav");
    fxCoin = LoadSound("resources/coin.wav");
    fxFail = LoadSound("resources/fail.wav");
    fxPaint = LoadSound("resources/paint.wav");
    fxWin = LoadSound("resources/win.wav");
    fxLife = LoadSound("resources/life.wav");
    fxCheckpoint = LoadSound("resources/checkpoint.wav");
    fxZoom = LoadSound("resources/zoom.wav");
    SetSoundVolume(fxZoom, 0.08f);
    for (int i = 0; i < ZOOM_ALIAS_COUNT; i++)
    {
        fxZoomAlias[i] = LoadSoundAlias(fxZoom);
        SetSoundVolume(fxZoomAlias[i], 0.08f);
    }
    fxBeeZoom = LoadSound("resources/bee_zoom.wav");
    SetSoundVolume(fxBeeZoom, 0.18f);
    beeAnim = CreateAnimation(assets.bee, 2, 4, 30);

    HexSocialInit();

    SetMasterVolume((float)volumeLevel/10.0f);
    SetMusicVolume(music, 0.20f);
    SetMusicVolume(musicStarPower, 0.55f);
    PlayMusicStream(music);
    SetExitKey(KEY_NULL);       // ESC used for pause menu, not window close

    // Setup and init first screen
    currentScreen = TITLE;
    InitTitleScreen();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif

    // Process exit: OS reclaims resources; no explicit unload.
    return 0;
}

void PlayWaspZoom(void)
{
    // Soft rate-limit so crowded levels don't become a constant buzz
    static double lastPlay = -1.0;
    double now = GetTime();
    if ((lastPlay >= 0.0) && ((now - lastPlay) < 0.07)) return;
    lastPlay = now;

    PlaySound(fxZoomAlias[fxZoomAliasIndex]);
    fxZoomAliasIndex = (fxZoomAliasIndex + 1)%ZOOM_ALIAS_COUNT;
}

void PlayBeeZoom(void)
{
    PlaySound(fxBeeZoom);
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
// Change to next screen, no transition
static void ChangeToScreen(int screen)
{
    // Unload current screen
    switch (currentScreen)
    {
        case TITLE: UnloadTitleScreen(); break;
        case OPTIONS: UnloadOptionsScreen(); break;
        case GAMEPLAY: UnloadGameplayScreen(); break;
        case ENDING: UnloadEndingScreen(); break;
#if defined(_DEBUG)
        case COVER: UnloadCoverScreen(); break;
#endif
        default: break;
    }

    // Init next screen
    switch (screen)
    {
        case TITLE: InitTitleScreen(); break;
        case OPTIONS: InitOptionsScreen(); break;
        case GAMEPLAY: InitGameplayScreen(); break;
        case ENDING: InitEndingScreen(); break;
#if defined(_DEBUG)
        case COVER: InitCoverScreen(); break;
#endif
        default: break;
    }

    currentScreen = screen;
}

// Request transition to next screen (callable from any screen Update)
void TransitionToScreen(GameScreen screen)
{
    if (onTransition) return;

    onTransition = true;
    transFadeOut = false;
    transFromScreen = currentScreen;
    transToScreen = screen;
    transAlpha = 0.0f;
}

// Update transition effect (fade-in, fade-out)
static void UpdateTransition(void)
{
    if (!transFadeOut)
    {
        transAlpha += 0.05f;

        // NOTE: Due to float internal representation, condition jumps on 1.0f instead of 1.05f
        // For that reason we compare against 1.01f, to avoid last frame loading stop
        if (transAlpha > 1.01f)
        {
            transAlpha = 1.0f;

            // Unload current screen
            switch (transFromScreen)
            {
                case TITLE: UnloadTitleScreen(); break;
                case OPTIONS: UnloadOptionsScreen(); break;
                case GAMEPLAY: UnloadGameplayScreen(); break;
                case ENDING: UnloadEndingScreen(); break;
#if defined(_DEBUG)
                case COVER: UnloadCoverScreen(); break;
#endif
                default: break;
            }

            // Load next screen
            switch (transToScreen)
            {
                case TITLE: InitTitleScreen(); break;
                case OPTIONS: InitOptionsScreen(); break;
                case GAMEPLAY: InitGameplayScreen(); break;
                case ENDING: InitEndingScreen(); break;
#if defined(_DEBUG)
                case COVER: InitCoverScreen(); break;
#endif
                default: break;
            }

            currentScreen = transToScreen;

            // Activate fade out effect to next loaded screen
            transFadeOut = true;
        }
    }
    else  // Transition fade out logic
    {
        transAlpha -= 0.02f;

        if (transAlpha < -0.01f)
        {
            transAlpha = 0.0f;
            transFadeOut = false;
            onTransition = false;
            transFromScreen = -1;
            transToScreen = UNKNOWN;
        }
    }
}

// Draw transition effect (full-screen rectangle)
static void DrawTransition(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, transAlpha));
}

// Update and draw game frame
static void UpdateDrawFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    UpdateMusicStream(music);       // NOTE: Music keeps playing between screens

    if (!onTransition)
    {
        switch (currentScreen)
        {
            case TITLE: UpdateTitleScreen(); break;
            case OPTIONS: UpdateOptionsScreen(); break;
            case GAMEPLAY: UpdateGameplayScreen(); break;
            case ENDING: UpdateEndingScreen(); break;
#if defined(_DEBUG)
            case COVER: UpdateCoverScreen(); break;
#endif
            default: break;
        }
    }
    else UpdateTransition();    // Update transition (fade-in, fade-out)
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(RAYWHITE);

        switch(currentScreen)
        {
            case TITLE: DrawTitleScreen(); break;
            case OPTIONS: DrawOptionsScreen(); break;
            case GAMEPLAY: DrawGameplayScreen(); break;
            case ENDING: DrawEndingScreen(); break;
#if defined(_DEBUG)
            case COVER: DrawCoverScreen(); break;
#endif
            default: break;
        }

        // Draw full screen rectangle in front of everything
        if (onTransition) DrawTransition();

        //DrawFPS(10, 10);

    EndDrawing();
    //----------------------------------------------------------------------------------
}
