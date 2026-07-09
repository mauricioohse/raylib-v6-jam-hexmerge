/**********************************************************************************************
*
*   raylib - Advance Game template
*
*   Gameplay Screen Functions Definitions (Init, Update, Draw, Unload)
*
*   Copyright (c) 2014-2022 Ramon Santamaria (@raysan5)
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "hex_level.h"
#include "hex_scores.h"
#include "hex_trail.h"

#include <math.h>
#include <stdio.h>

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
#define PLAYER_MAX_LIVES 3
#define HIT_RADIUS 14.0f
#define HEART_SCALE 2.0f
#define BEE_SPEED 120.0f

static int framesCounter = 0;
static int finishScreen = 0;

static HexLevel level = { 0 };
static int currentLevelIndex = 0;
static Texture2D hexTexture = { 0 };
static Texture2D waspTexture = { 0 };
static Texture2D heartsTexture = { 0 };
static Texture2D flowerTexture = { 0 };
static Texture2D bubbleTexture = { 0 };
static Texture2D starTexture = { 0 };
static int lives = PLAYER_MAX_LIVES;
static float runTimer = 0.0f;
static bool runActive = false;
static bool levelPaused = true;     // wait for A/D (or arrows) before bee/wasps move
static bool starMusicPlaying = false;

//----------------------------------------------------------------------------------
// Animation helpers
//----------------------------------------------------------------------------------
Animation CreateAnimation(const char *filepath, float scale, int frameCnt, int speed)
{
    Animation rtn = { 0 };
    rtn.filepath = filepath;
    rtn.scale = scale;
    rtn.frameCnt = frameCnt;
    rtn.speed = speed;
    rtn.text = LoadTexture(filepath);
    rtn.frame = 0;
    rtn.countdown = speed;

    // Vertical spritesheet
    float frameH = (float)rtn.text.height/(float)rtn.frameCnt;
    float frameW = (float)rtn.text.width;
    rtn.src = (Rectangle){ 0, 0, frameW, frameH };
    rtn.dst = (Rectangle){ 0, 0, frameW*scale, frameH*scale };
    rtn.origin = (Vector2){ rtn.dst.width*0.5f, rtn.dst.height*0.5f };
    rtn.rotation = 0.0f;

    return rtn;
}

void UpdateAnimation(Animation *anim)
{
    anim->countdown--;
    if (anim->countdown <= 0)
    {
        anim->frame = (anim->frame + 1)%anim->frameCnt;
        anim->countdown = anim->speed;
    }

    float frameH = (float)anim->text.height/(float)anim->frameCnt;
    float frameW = (float)anim->text.width;
    anim->src = (Rectangle){ 0, frameH*(float)anim->frame, frameW, frameH };
}

void DrawAnimation(Animation *anim, Vector2 position)
{
    anim->dst.x = position.x;
    anim->dst.y = position.y;
    DrawTexturePro(anim->text, anim->src, anim->dst, anim->origin, anim->rotation, WHITE);
}

void DrawAnimationTint(Animation *anim, Vector2 position, Color tint)
{
    anim->dst.x = position.x;
    anim->dst.y = position.y;
    DrawTexturePro(anim->text, anim->src, anim->dst, anim->origin, anim->rotation, tint);
}

//----------------------------------------------------------------------------------
// Round / lives helpers
//----------------------------------------------------------------------------------
static void StopStarMusic(void)
{
    if (starMusicPlaying)
    {
        StopMusicStream(musicStarPower);
        starMusicPlaying = false;
    }
}

static void SyncStarMusic(bool powered)
{
    if (powered)
    {
        if (!starMusicPlaying)
        {
            musicStarPower.looping = true;
            PlayMusicStream(musicStarPower);
            starMusicPlaying = true;
        }
        UpdateMusicStream(musicStarPower);
    }
    else
    {
        StopStarMusic();
    }
}

static Color RainbowTint(float t)
{
    // Cycle hue quickly while star-powered
    float h = t*2.5f;
    h = h - (float)((int)h);    // frac
    float s = 0.85f;
    float v = 1.0f;
    float c = v*s;
    float x = c*(1.0f - fabsf(fmodf(h*6.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r = 0, g = 0, b = 0;
    int sector = (int)(h*6.0f);
    switch (sector%6)
    {
        case 0: r = c; g = x; b = 0; break;
        case 1: r = x; g = c; b = 0; break;
        case 2: r = 0; g = c; b = x; break;
        case 3: r = 0; g = x; b = c; break;
        case 4: r = x; g = 0; b = c; break;
        default: r = c; g = 0; b = x; break;
    }
    return (Color){
        (unsigned char)((r + m)*255.0f),
        (unsigned char)((g + m)*255.0f),
        (unsigned char)((b + m)*255.0f),
        255
    };
}

static void LoadCurrentLevel(void)
{
    StopStarMusic();
    HexLevelLoad(&level, currentLevelIndex, hexTexture, flowerTexture, bubbleTexture, starTexture, BEE_SPEED);
    levelPaused = true;
    beeAnim.rotation = HexBeeRotationDeg(&level.bee, &level.grid);
    UpdateAnimation(&beeAnim);
}

static void FinishRun(void)
{
    StopStarMusic();
    runActive = false;
    lastRunTime = runTimer;
    HexScoresSubmit(lastRunTime);
    finishScreen = 1;
    PlaySound(fxWin);
}

static void DrawLives(void)
{
    float frameW = (float)heartsTexture.width;
    float frameH = (float)heartsTexture.height/2.0f;
    float drawW = frameW*HEART_SCALE;
    float drawH = frameH*HEART_SCALE;
    float gap = 4.0f;
    float totalW = PLAYER_MAX_LIVES*drawW + (PLAYER_MAX_LIVES - 1)*gap;
    float x0 = (float)GetScreenWidth() - 16.0f - totalW;
    float y = 16.0f;

    for (int i = 0; i < PLAYER_MAX_LIVES; i++)
    {
        int frame = (i < lives)? 0 : 1;     // 0 = full, 1 = empty/grey
        Rectangle src = { 0, frameH*(float)frame, frameW, frameH };
        Rectangle dst = { x0 + (float)i*(drawW + gap), y, drawW, drawH };
        DrawTexturePro(heartsTexture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
    }
}

//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------
void InitGameplayScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;
    lives = PLAYER_MAX_LIVES;
    currentLevelIndex = 0;
    runTimer = 0.0f;
    runActive = true;
    lastRunTime = 0.0f;

    hexTexture = LoadTexture("resources/hexfield.png");
    waspTexture = LoadTexture("resources/wasp.png");
    heartsTexture = LoadTexture("resources/hearts.png");
    flowerTexture = LoadTexture("resources/flower.png");
    bubbleTexture = LoadTexture("resources/bubbles.png");
    starTexture = LoadTexture("resources/star.png");
    SetTextureFilter(bubbleTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(starTexture, TEXTURE_FILTER_POINT);

    LoadCurrentLevel();
}

void UpdateGameplayScreen(void)
{
    float dt = GetFrameTime();

#if defined(_DEBUG)
    for (int key = KEY_ONE; key <= KEY_NINE; key++)
    {
        if (IsKeyPressed(key))
        {
            currentLevelIndex = key - KEY_ONE;
            lives = PLAYER_MAX_LIVES;
            finishScreen = 0;
            LoadCurrentLevel();
            PlaySound(fxCoin);
            return;
        }
    }
    if (IsKeyPressed(KEY_ZERO))
    {
        currentLevelIndex = 9;  // level 10
        lives = PLAYER_MAX_LIVES;
        finishScreen = 0;
        LoadCurrentLevel();
        PlaySound(fxCoin);
        return;
    }
    // < / , previous   > / . next
    if (IsKeyPressed(KEY_COMMA))
    {
        if (currentLevelIndex > 0) currentLevelIndex--;
        else currentLevelIndex = HexLevelCount() - 1;
        lives = PLAYER_MAX_LIVES;
        finishScreen = 0;
        LoadCurrentLevel();
        PlaySound(fxCoin);
        return;
    }
    if (IsKeyPressed(KEY_PERIOD))
    {
        currentLevelIndex = (currentLevelIndex + 1)%HexLevelCount();
        lives = PLAYER_MAX_LIVES;
        finishScreen = 0;
        LoadCurrentLevel();
        PlaySound(fxCoin);
        return;
    }
#endif

    bool turnLeft = IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT);
    bool turnRight = IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);

    if (levelPaused)
    {
        if (turnLeft || turnRight)
        {
            if (turnLeft) HexBeeSetTurn(&level.bee, -1);
            if (turnRight) HexBeeSetTurn(&level.bee, 1);
            levelPaused = false;
        }
        else
        {
            // Idle anim only; bee/wasps/timer stay frozen until first steer
            UpdateAnimation(&beeAnim);
            return;
        }
    }
    else
    {
        if (turnLeft) HexBeeSetTurn(&level.bee, -1);
        if (turnRight) HexBeeSetTurn(&level.bee, 1);
    }

    if (runActive) runTimer += dt;

    int filled = HexLevelUpdate(&level, dt);
    if (filled == HEX_TRAIL_TWIN_FAIL) PlaySound(fxFail);
    else if (filled > 0) PlaySound(fxPaint);

    SyncStarMusic(HexLevelStarPowered(&level));

    if (HexLevelBeeHit(&level, HIT_RADIUS))
    {
        StopStarMusic();
        lives--;
        PlaySound(fxLife);
        if (lives <= 0)
        {
            // All lives lost: restart campaign + timer
            lives = PLAYER_MAX_LIVES;
            currentLevelIndex = 0;
            runTimer = 0.0f;
            runActive = true;
        }
        LoadCurrentLevel();
        return;
    }

    beeAnim.rotation = HexBeeRotationDeg(&level.bee, &level.grid);
    UpdateAnimation(&beeAnim);

    if (HexLevelWon(&level))
    {
        StopStarMusic();
        if (currentLevelIndex + 1 < HexLevelCount())
        {
            currentLevelIndex++;
            LoadCurrentLevel();
            PlaySound(fxWin);
        }
        else
        {
            FinishRun();
        }
    }

#if defined(_DEBUG)
    // Skip to ending with current timer (does not count as a real win unless you want it to)
    if (IsKeyPressed(KEY_ENTER))
    {
        FinishRun();
    }
#endif
}

void DrawGameplayScreen(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 24, 28, 36, 255 });

    HexLevelDraw(&level, waspTexture);

    Vector2 beePos = HexBeePosition(&level.bee, &level.grid);
    if (HexLevelStarPowered(&level))
        DrawAnimationTint(&beeAnim, beePos, RainbowTint(runTimer));
    else
        DrawAnimation(&beeAnim, beePos);

    DrawLives();
    HexLevelDrawHint(&level);

    char levelLabel[32];
    snprintf(levelLabel, sizeof(levelLabel), "Level %d", currentLevelIndex + 1);
    DrawText(levelLabel, 16, 16, 20, LIGHTGRAY);
    DrawText(levelPaused? "A/D to start" : "A/D turn", 16, 40, 18, (Color){ 160, 170, 180, 255 });

    char timeBuf[16];
    HexScoresFormat(runTimer, timeBuf, (int)sizeof(timeBuf));
    int tw = MeasureText(timeBuf, 22);
    DrawText(timeBuf, (GetScreenWidth() - tw)/2, 16, 22, (Color){ 255, 220, 70, 255 });

    if (levelPaused)
    {
        const char *prompt = "Press A/D or arrows to start";
        int pw = MeasureText(prompt, 24);
        DrawText(prompt, (GetScreenWidth() - pw)/2, GetScreenHeight() - 56, 24, (Color){ 255, 220, 70, 255 });
    }

#if defined(_DEBUG)
    DrawText("DEBUG: 1-9/0 jump  </, >/ . step", 16, GetScreenHeight() - 28, 16, (Color){ 120, 140, 160, 255 });
#endif
}

void UnloadGameplayScreen(void)
{
    StopStarMusic();
    UnloadTexture(hexTexture);
    hexTexture = (Texture2D){ 0 };
    UnloadTexture(waspTexture);
    waspTexture = (Texture2D){ 0 };
    UnloadTexture(heartsTexture);
    heartsTexture = (Texture2D){ 0 };
    UnloadTexture(flowerTexture);
    flowerTexture = (Texture2D){ 0 };
    UnloadTexture(bubbleTexture);
    bubbleTexture = (Texture2D){ 0 };
    UnloadTexture(starTexture);
    starTexture = (Texture2D){ 0 };
}

int FinishGameplayScreen(void)
{
    return finishScreen;
}
