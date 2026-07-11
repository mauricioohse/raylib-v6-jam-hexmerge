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
#include "hex_background.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
#define PLAYER_MAX_LIVES 5
#define HIT_RADIUS 14.0f
#define HEART_SCALE 2.0f
#define BEE_SPEED 120.0f
#define VOLUME_MIN 0
#define VOLUME_MAX 10
#define SPEAKER_FRAME_COUNT 4
#define SPEAKER_SCALE 2.0f
#define LEVEL_CLEAR_DURATION 5.0f
#define LEVEL_CLEAR_SKIP_AFTER 1.0f
#define RATING_STAR_DRAW_SCALE 2.0f

static int framesCounter = 0;
static int finishScreen = 0;

static HexLevel level = { 0 };
static int currentLevelIndex = 0;
static Texture2D hexTexture = { 0 };
static Texture2D pondTexture = { 0 };
static Texture2D waspTexture = { 0 };
static Texture2D heartsTexture = { 0 };
static Texture2D flowerTexture = { 0 };
static Texture2D bubbleTexture = { 0 };
static Texture2D starTexture = { 0 };
static Texture2D fireTexture = { 0 };
static Texture2D speakerTexture = { 0 };
static Texture2D ratingStarTex = { 0 };
static Texture2D ratingStarEmptyTex = { 0 };
static int lives = PLAYER_MAX_LIVES;
static float levelTimer = 0.0f;     // resets each level
static bool runActive = false;
static bool levelPaused = true;     // wait for first steer before bee/wasps move
static bool gamePaused = false;     // pause menu (P / ESC)
static bool starMusicPlaying = false;
static bool moveModeRelative = false;   // false = WASD absolute (default), true = A/D relative
static int checkpointLevel = 0;
static bool levelTookDamage = false;
static int levelDeaths = 0;             // deaths this level attempt (for star rating)
static float checkpointBannerTimer = 0.0f;
static float fireFailDelay = 0.0f;   // show red shake before applying damage
static bool levelClearActive = false;
static float levelClearTimer = 0.0f;
static int levelClearStars = 0;
static int levelClearLevelNumber = 0;   // 1-based
static float levelClearTimeSec = 0.0f;
static bool levelClearIsFinal = false;
static HexBackground fallBg = { 0 };
static Rectangle pauseMainMenuBtn = { 0 };
static Rectangle pauseVolDownBtn = { 0 };
static Rectangle pauseVolUpBtn = { 0 };

//----------------------------------------------------------------------------------
// Pause menu helpers
//----------------------------------------------------------------------------------
static void ApplyVolume(void)
{
    if (volumeLevel < VOLUME_MIN) volumeLevel = VOLUME_MIN;
    if (volumeLevel > VOLUME_MAX) volumeLevel = VOLUME_MAX;
    SetMasterVolume((float)volumeLevel/10.0f);
}

static bool Clicked(Rectangle r)
{
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), r);
}

static int SpeakerFrame(void)
{
    if (volumeLevel <= 0) return 0;
    if (volumeLevel <= 3) return 1;
    if (volumeLevel <= 6) return 2;
    return 3;
}

static void DrawSpeakerIcon(float x, float y)
{
    float frameW = (float)speakerTexture.width;
    float frameH = (float)speakerTexture.height/(float)SPEAKER_FRAME_COUNT;
    float drawW = frameW*SPEAKER_SCALE;
    float drawH = frameH*SPEAKER_SCALE;
    int frame = SpeakerFrame();

    Rectangle src = { 0, frameH*(float)frame, frameW, frameH };
    Rectangle dst = { x, y, drawW, drawH };
    DrawTexturePro(speakerTexture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
}

static void DrawMenuButton(Rectangle r, const char *label, bool hovered)
{
    Color fill = hovered? (Color){ 255, 179, 71, 255 } : (Color){ 50, 58, 72, 255 };
    Color border = hovered? (Color){ 255, 220, 140, 255 } : (Color){ 90, 100, 120, 255 };
    Color text = hovered? (Color){ 30, 24, 16, 255 } : RAYWHITE;

    DrawRectangleRec(r, fill);
    DrawRectangleLinesEx(r, 2.0f, border);

    int fontSize = 30;
    int tw = MeasureText(label, fontSize);
    DrawText(label, (int)(r.x + (r.width - tw)*0.5f), (int)(r.y + (r.height - fontSize)*0.5f), fontSize, text);
}

static void LayoutPauseMenu(void)
{
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    pauseMainMenuBtn = (Rectangle){ sw*0.5f - 140.0f, sh*0.5f - 10.0f, 280.0f, 52.0f };

    float volY = pauseMainMenuBtn.y + pauseMainMenuBtn.height + 36.0f;
    pauseVolDownBtn = (Rectangle){ sw*0.5f - 20.0f, volY, 36.0f, 36.0f };
    pauseVolUpBtn = (Rectangle){ sw*0.5f + 52.0f, volY, 36.0f, 36.0f };
}

static void SetGamePaused(bool paused)
{
    if (gamePaused == paused) return;
    gamePaused = paused;
    if (paused)
    {
        PauseMusicStream(music);
        if (starMusicPlaying) PauseMusicStream(musicStarPower);
    }
    else
    {
        if (starMusicPlaying) ResumeMusicStream(musicStarPower);
        else ResumeMusicStream(music);
    }
}

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
        if (!gamePaused) ResumeMusicStream(music);
    }
}

static void SyncStarMusic(bool powered)
{
    if (powered)
    {
        if (!starMusicPlaying)
        {
            PauseMusicStream(music);
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

static void ResetRunStats(void)
{
    memset(&lastRun, 0, sizeof(lastRun));
    lastRunTime = 0.0f;
    levelTimer = 0.0f;
}

// Drop recorded times for levels at/after the checkpoint (we're replaying from there).
static void TruncateRunStatsToCheckpoint(void)
{
    if (checkpointLevel < 0) checkpointLevel = 0;
    if (lastRun.levelsRecorded > checkpointLevel)
        lastRun.levelsRecorded = checkpointLevel;

    lastRun.totalTime = 0.0f;
    lastRun.totalStars = 0;
    for (int i = 0; i < lastRun.levelsRecorded; i++)
    {
        lastRun.totalTime += lastRun.levels[i].timeSec;
        lastRun.totalStars += lastRun.levels[i].stars;
    }
    lastRunTime = lastRun.totalTime;
    levelTimer = 0.0f;
    levelDeaths = 0;
}

static void RecordCurrentLevelStats(void)
{
    if (lastRun.levelsRecorded >= HEX_RUN_MAX_LEVELS) return;

    int i = lastRun.levelsRecorded++;
    lastRun.levels[i].timeSec = levelTimer;
    lastRun.levels[i].stars = HexScoresStarsFromDeaths(levelDeaths);
    lastRun.totalTime += levelTimer;
    lastRun.totalStars += lastRun.levels[i].stars;
}

static void TryActivateCheckpoint(void)
{
    // Every level is a checkpoint — dying with 0 lives restarts this level.
    if (currentLevelIndex < checkpointLevel) return;

    checkpointLevel = currentLevelIndex;
    PlaySound(fxCheckpoint);
    checkpointBannerTimer = 2.0f;
}

static void LoadCurrentLevel(bool resetDeaths)
{
    StopStarMusic();
    HexLevelLoad(&level, currentLevelIndex, hexTexture, pondTexture, flowerTexture, bubbleTexture, starTexture, BEE_SPEED);
    levelPaused = true;
    levelTimer = 0.0f;
    if (resetDeaths) levelDeaths = 0;
    levelTookDamage = (levelDeaths > 0);
    fireFailDelay = 0.0f;
    beeAnim.rotation = HexBeeRotationDeg(&level.bee, &level.grid);
    UpdateAnimation(&beeAnim);
}

static void FinishRun(bool won)
{
    StopStarMusic();
    runActive = false;
    levelClearActive = false;

    // Capture the level we just finished if it wasn't recorded by the clear transition
    if (lastRun.levelsRecorded <= currentLevelIndex)
        RecordCurrentLevelStats();

    lastRun.won = won;
    lastRunTime = lastRun.totalTime;

    HexScoresAppendRunCsv(&lastRun);
    if (won) 
    {
        HexScoresSubmit(lastRun.totalTime);
        HexScoresSaveBestRunIfBetter(&lastRun);
    }

    endingFromMenu = false;
    finishScreen = 1;
    PlaySound(won? fxWin : fxLife);
}

static void BeginLevelClear(bool isFinal)
{
    RecordCurrentLevelStats();
    levelClearStars = lastRun.levels[lastRun.levelsRecorded - 1].stars;
    levelClearTimeSec = lastRun.levels[lastRun.levelsRecorded - 1].timeSec;
    levelClearLevelNumber = currentLevelIndex + 1;
    levelClearIsFinal = isFinal;
    levelClearTimer = 0.0f;
    levelClearActive = true;
    levelPaused = true;

    if (!isFinal) lives = PLAYER_MAX_LIVES;

    PlaySound(fxWin);
}

static void EndLevelClear(void)
{
    levelClearActive = false;
    levelClearTimer = 0.0f;

    if (levelClearIsFinal)
    {
        FinishRun(true);
        return;
    }

    currentLevelIndex++;
    LoadCurrentLevel(true);
    TryActivateCheckpoint();
}

static void SoftRespawnLevel(void)
{
    StopStarMusic();
    HexLevelRespawnKeepProgress(&level, BEE_SPEED);
    levelPaused = true;
    fireFailDelay = 0.0f;
    beeAnim.rotation = HexBeeRotationDeg(&level.bee, &level.grid);
    UpdateAnimation(&beeAnim);
}

// Returns true if the run ended (caller should return from Update).
static bool ApplyDamage(void)
{
    StopStarMusic();
    levelDeaths++;
    levelTookDamage = true;

    lives--;
    PlaySound(fxLife);

    if (lives <= 0)
    {
        // Soft fail: restart from last checkpoint with full lives
        currentLevelIndex = checkpointLevel;
        lives = PLAYER_MAX_LIVES;
        TruncateRunStatsToCheckpoint();
        LoadCurrentLevel(true);
        TryActivateCheckpoint();
        return false;
    }

    // Lives remain: keep filled hexes, pollen trails, and progress; reset bee + enemies
    SoftRespawnLevel();
    return false;
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
    gamePaused = false;
    moveModeRelative = controllerMode;  // title MOVE: A/D vs WASD
    // keep controllerMode so title remembers the choice after returning from a run
    lives = PLAYER_MAX_LIVES;
    currentLevelIndex = 0;
    checkpointLevel = 0;
    levelTookDamage = false;
    levelDeaths = 0;
    checkpointBannerTimer = 0.0f;
    fireFailDelay = 0.0f;
    levelClearActive = false;
    levelClearTimer = 0.0f;
    runActive = true;
    ResetRunStats();
    SetMusicPitch(music, 1.0f);

    hexTexture = LoadTexture("resources/hexfield.png");
    pondTexture = LoadTexture("resources/hexpond.png");
    waspTexture = LoadTexture("resources/wasp.png");
    heartsTexture = LoadTexture("resources/hearts.png");
    flowerTexture = LoadTexture("resources/flower.png");
    bubbleTexture = LoadTexture("resources/bubbles.png");
    starTexture = LoadTexture("resources/star.png");
    fireTexture = LoadTexture("resources/fire.png");
    speakerTexture = LoadTexture("resources/speaker.png");
    ratingStarTex = LoadTexture("resources/rating_star.png");
    ratingStarEmptyTex = LoadTexture("resources/rating_star_empty.png");
    SetTextureFilter(bubbleTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(starTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(fireTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(speakerTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(ratingStarTex, TEXTURE_FILTER_POINT);
    SetTextureFilter(ratingStarEmptyTex, TEXTURE_FILTER_POINT);

    LayoutPauseMenu();
    HexBackgroundInit(&fallBg, hexTexture, HEX_BG_GAMEPLAY);
    LoadCurrentLevel(true);
    TryActivateCheckpoint();
}

void UpdateGameplayScreen(void)
{
    float dt = GetFrameTime();
    HexBackgroundUpdate(&fallBg, dt);

    if (!levelClearActive && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)))
    {
        SetGamePaused(!gamePaused);
        PlaySound(fxCoin);
    }

    if (gamePaused)
    {
        if (Clicked(pauseMainMenuBtn))
        {
            SetGamePaused(false);
            StopStarMusic();
            runActive = false;
            levelClearActive = false;
            finishScreen = 2;   // abandon run → TITLE
            PlaySound(fxCoin);
            return;
        }

        if (Clicked(pauseVolDownBtn) || IsKeyPressed(KEY_LEFT))
        {
            if (volumeLevel > VOLUME_MIN)
            {
                volumeLevel--;
                ApplyVolume();
                PlaySound(fxCoin);
            }
        }
        else if (Clicked(pauseVolUpBtn) || IsKeyPressed(KEY_RIGHT))
        {
            if (volumeLevel < VOLUME_MAX)
            {
                volumeLevel++;
                ApplyVolume();
                PlaySound(fxCoin);
            }
        }
        return;
    }

    // Between-level (or final) star results: freeze the cleared board underneath
    if (levelClearActive)
    {
        levelClearTimer += dt;
        bool canSkip = (levelClearTimer >= LEVEL_CLEAR_SKIP_AFTER);
        bool skip = canSkip && (
            IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE) ||
            IsGestureDetected(GESTURE_TAP) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
            IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_S) ||
            IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN));

        if ((levelClearTimer >= LEVEL_CLEAR_DURATION) || skip)
            EndLevelClear();
        return;
    }

    if (checkpointBannerTimer > 0.0f)
    {
        checkpointBannerTimer -= dt;
        if (checkpointBannerTimer < 0.0f) checkpointBannerTimer = 0.0f;
    }

    if (fireFailDelay > 0.0f)
    {
        fireFailDelay -= dt;
        HexGridUpdate(&level.grid, dt);   // keep shake/fire anim ticking
        if (fireFailDelay <= 0.0f)
        {
            fireFailDelay = 0.0f;
            ApplyDamage();
        }
        return;
    }

#if defined(_DEBUG)
    for (int key = KEY_ONE; key <= KEY_NINE; key++)
    {
        if (IsKeyPressed(key))
        {
            currentLevelIndex = key - KEY_ONE;
            lives = PLAYER_MAX_LIVES;
            checkpointLevel = currentLevelIndex;
            finishScreen = 0;
            ResetRunStats();
            LoadCurrentLevel(true);
            TryActivateCheckpoint();
            PlaySound(fxCoin);
            return;
        }
    }
    if (IsKeyPressed(KEY_ZERO))
    {
        currentLevelIndex = 9;  // level 10
        lives = PLAYER_MAX_LIVES;
        checkpointLevel = currentLevelIndex;
        finishScreen = 0;
        ResetRunStats();
        LoadCurrentLevel(true);
        TryActivateCheckpoint();
        PlaySound(fxCoin);
        return;
    }
    // < / , previous   > / . next
    if (IsKeyPressed(KEY_COMMA))
    {
        if (currentLevelIndex > 0) currentLevelIndex--;
        else currentLevelIndex = HexLevelCount() - 1;
        lives = PLAYER_MAX_LIVES;
        checkpointLevel = currentLevelIndex;
        finishScreen = 0;
        ResetRunStats();
        LoadCurrentLevel(true);
        TryActivateCheckpoint();
        PlaySound(fxCoin);
        return;
    }
    if (IsKeyPressed(KEY_PERIOD))
    {
        currentLevelIndex = (currentLevelIndex + 1)%HexLevelCount();
        lives = PLAYER_MAX_LIVES;
        checkpointLevel = currentLevelIndex;
        finishScreen = 0;
        ResetRunStats();
        LoadCurrentLevel(true);
        TryActivateCheckpoint();
        PlaySound(fxCoin);
        return;
    }
    if (IsKeyPressed(KEY_G))
    {
        moveModeRelative = !moveModeRelative;
        PlaySound(fxCoin);
    }
    if (IsKeyPressed(KEY_H))
    {
        for (int f = 0; f < level.grid.faceCount; f++)
        {
            if (level.grid.faces[f].kind == HEX_FACE_FIRE)
                level.grid.faces[f].kind = HEX_FACE_NORMAL;
            level.grid.faces[f].filled = true;
        }
        for (int e = 0; e < level.grid.edgeCount; e++)
            level.grid.edges[e].painted = true;

        HexFlowerFieldOnFill(&level.flowers, &level.grid);
        for (int i = 0; i < level.flowers.count; i++)
        {
            level.flowers.flowers[i].state = HEX_FLOWER_BLOOMED;
            level.flowers.flowers[i].animFrame = 3;   // idle bloom frame
            level.flowers.flowers[i].animTimer = 0.0f;
        }
        levelPaused = false;
        PlaySound(fxPaint);
    }
#endif

    HexBeeInput steer = HEX_BEE_INPUT_NONE;
    if (moveModeRelative)
    {
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))
            steer = HEX_BEE_INPUT_TURN_LEFT;
        else if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT))
            steer = HEX_BEE_INPUT_TURN_RIGHT;
    }
    else
    {
        // Absolute screen directions (WASD / arrows). Invalid exits are ignored at the junction.
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
            steer = HEX_BEE_INPUT_DIR_UP;
        else if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
            steer = HEX_BEE_INPUT_DIR_DOWN;
        else if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))
            steer = HEX_BEE_INPUT_DIR_LEFT;
        else if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT))
            steer = HEX_BEE_INPUT_DIR_RIGHT;
    }

    if (levelPaused)
    {
        if (steer != HEX_BEE_INPUT_NONE)
        {
            HexBeeSetInput(&level.bee, steer);
            levelPaused = false;
        }
        else
        {
            // Idle anim only; bee/wasps/timer stay frozen until first steer
            UpdateAnimation(&beeAnim);
            return;
        }
    }
    else if (steer != HEX_BEE_INPUT_NONE)
    {
        HexBeeSetInput(&level.bee, steer);
    }

    if (runActive) levelTimer += dt;

    int filled = HexLevelUpdate(&level, dt);
    if (filled == HEX_TRAIL_TWIN_FAIL) PlaySound(fxFail);
    else if (filled == HEX_TRAIL_FIRE_FAIL)
    {
        PlaySound(fxFail);
        fireFailDelay = 0.55f;   // match twin failShake duration
        return;
    }
    else if (filled > 0) PlaySound(fxPaint);

    SyncStarMusic(HexLevelStarPowered(&level));

    if (HexLevelBeeHit(&level, HIT_RADIUS))
    {
        if (ApplyDamage()) return;
        return;
    }

    beeAnim.rotation = HexBeeRotationDeg(&level.bee, &level.grid);
    UpdateAnimation(&beeAnim);

    if (HexLevelWon(&level))
    {
        StopStarMusic();
        BeginLevelClear(currentLevelIndex + 1 >= HexLevelCount());
        return;
    }

#if defined(_DEBUG)
    // Skip to ending with current level stats
    if (IsKeyPressed(KEY_ENTER))
    {
        FinishRun(true);
    }
#endif
}

void DrawGameplayScreen(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 24, 28, 36, 255 });
    HexBackgroundDraw(&fallBg);

    HexLevelDraw(&level, waspTexture, fireTexture);

    Vector2 beePos = HexBeePosition(&level.bee, &level.grid);
    if (HexLevelStarPowered(&level))
        DrawAnimationTint(&beeAnim, beePos, RainbowTint(levelTimer));
    else
        DrawAnimation(&beeAnim, beePos);

    DrawLives();
    HexLevelDrawHint(&level);

    char levelLabel[32];
    snprintf(levelLabel, sizeof(levelLabel), "Level %d", currentLevelIndex + 1);
    DrawText(levelLabel, 16, 16, 20, LIGHTGRAY);
    // DrawText(levelPaused? (moveModeRelative? "A/D to start" : "WASD to start")
    //                     : (moveModeRelative? "A/D turn" : "WASD move"),
    //          16, 40, 20, (Color){ 160, 170, 180, 255 });

    char timeBuf[16];
    HexScoresFormat(levelTimer, timeBuf, (int)sizeof(timeBuf));
    int tw = MeasureText(timeBuf, 20);
    DrawText(timeBuf, (GetScreenWidth() - tw)/2, 16, 20, (Color){ 255, 220, 70, 255 });

    if (checkpointBannerTimer > 0.0f)
    {
        float t = checkpointBannerTimer/2.0f;
        if (t > 1.0f) t = 1.0f;
        unsigned char a = (unsigned char)(t*255.0f);
        const char *banner = "CHECKPOINT!";
        int fontSize = 30;
        int bw = MeasureText(banner, fontSize);
        DrawText(banner, (GetScreenWidth() - bw)/2, GetScreenHeight()/4 - fontSize/2, fontSize,
                 (Color){ 255, 220, 70, a });
    }

    if (levelPaused && !levelClearActive)
    {
        const char *prompt = moveModeRelative? "Press A/D or arrows to start"
                                             : "Press WASD or arrows to start";
        int pw = MeasureText(prompt, 20);
        DrawText(prompt, (GetScreenWidth() - pw)/2, GetScreenHeight() - 56, 20, (Color){ 255, 220, 70, 255 });
    }

    if (levelClearActive)
    {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.55f));

        const char *cleared = TextFormat("LEVEL %d CLEAR!", levelClearLevelNumber);
        int titleSize = 40;
        DrawText(cleared, (sw - MeasureText(cleared, titleSize))/2, sh/2 - 110, titleSize,
                 (Color){ 255, 179, 71, 255 });

        float starSize = (float)((ratingStarTex.id != 0)? ratingStarTex.width : 32)*RATING_STAR_DRAW_SCALE;
        float starGap = 8.0f;
        float starsW = HEX_LEVEL_STARS_MAX*starSize + (HEX_LEVEL_STARS_MAX - 1)*starGap;
        float starsX = ((float)sw - starsW)*0.5f;
        float starsY = (float)sh*0.5f - 40.0f;
        HexScoresDrawLevelStars(ratingStarTex, ratingStarEmptyTex, levelClearStars,
                                starsX, starsY, RATING_STAR_DRAW_SCALE);

        char clearTimeBuf[16];
        HexScoresFormat(levelClearTimeSec, clearTimeBuf, (int)sizeof(clearTimeBuf));
        const char *timeLine = TextFormat("Time  %s", clearTimeBuf);
        DrawText(timeLine, (sw - MeasureText(timeLine, 20))/2, sh/2 + 50, 20, RAYWHITE);

        if (levelClearTimer >= LEVEL_CLEAR_SKIP_AFTER)
        {
            const char *skip = "Press a movement key to continue";
            DrawText(skip, (sw - MeasureText(skip, 20))/2, sh/2 + 100, 20,
                     (Color){ 200, 210, 220, 255 });
        }
    }

#if defined(_DEBUG)
    DrawText(TextFormat("DEBUG: 1-9/0 jump  </, >/ . step  G=%s  H=fill",
                        moveModeRelative? "A/D" : "WASD"),
             16, GetScreenHeight() - 28, 10, (Color){ 120, 140, 160, 255 });
#endif

    if (gamePaused)
    {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.65f));

        const char *paused = "PAUSED";
        int pausedSize = 40;
        int pw = MeasureText(paused, pausedSize);
        DrawText(paused, (GetScreenWidth() - pw)/2, GetScreenHeight()/2 - 100, pausedSize,
                 (Color){ 255, 179, 71, 255 });

        Vector2 mouse = GetMousePosition();
        DrawMenuButton(pauseMainMenuBtn, "MAIN MENU", CheckCollisionPointRec(mouse, pauseMainMenuBtn));

        float volY = pauseVolDownBtn.y;
        float speakerDrawH = 16.0f*SPEAKER_SCALE;
        DrawSpeakerIcon((float)GetScreenWidth()*0.5f - 100.0f,
                        volY + (pauseVolDownBtn.height - speakerDrawH)*0.5f);

        bool downHover = CheckCollisionPointRec(mouse, pauseVolDownBtn);
        bool upHover = CheckCollisionPointRec(mouse, pauseVolUpBtn);
        Color downFill = downHover? (Color){ 70, 80, 100, 255 } : (Color){ 40, 46, 58, 255 };
        Color upFill = upHover? (Color){ 70, 80, 100, 255 } : (Color){ 40, 46, 58, 255 };

        DrawRectangleRec(pauseVolDownBtn, downFill);
        DrawRectangleLinesEx(pauseVolDownBtn, 2.0f, (Color){ 90, 100, 120, 255 });
        DrawText("<", (int)(pauseVolDownBtn.x + 10), (int)(pauseVolDownBtn.y + 4), 30, RAYWHITE);

        char volText[8];
        snprintf(volText, sizeof(volText), "%d", volumeLevel);
        int vw = MeasureText(volText, 30);
        DrawText(volText,
                 (int)(pauseVolDownBtn.x + pauseVolDownBtn.width +
                       (pauseVolUpBtn.x - (pauseVolDownBtn.x + pauseVolDownBtn.width) - vw)*0.5f),
                 (int)(volY + 4), 30, RAYWHITE);

        DrawRectangleRec(pauseVolUpBtn, upFill);
        DrawRectangleLinesEx(pauseVolUpBtn, 2.0f, (Color){ 90, 100, 120, 255 });
        DrawText(">", (int)(pauseVolUpBtn.x + 10), (int)(pauseVolUpBtn.y + 4), 30, RAYWHITE);

        const char *hint = "P / ESC to resume";
        int hw = MeasureText(hint, 20);
        DrawText(hint, (GetScreenWidth() - hw)/2,
                 (int)(pauseVolDownBtn.y + pauseVolDownBtn.height + 28.0f), 20,
                 (Color){ 160, 170, 180, 255 });
    }
}

void UnloadGameplayScreen(void)
{
    StopStarMusic();
    gamePaused = false;
    SetMusicPitch(music, 1.0f);
    ResumeMusicStream(music);
    UnloadTexture(hexTexture);
    hexTexture = (Texture2D){ 0 };
    UnloadTexture(pondTexture);
    pondTexture = (Texture2D){ 0 };
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
    UnloadTexture(fireTexture);
    fireTexture = (Texture2D){ 0 };
    UnloadTexture(speakerTexture);
    speakerTexture = (Texture2D){ 0 };
    UnloadTexture(ratingStarTex);
    ratingStarTex = (Texture2D){ 0 };
    UnloadTexture(ratingStarEmptyTex);
    ratingStarEmptyTex = (Texture2D){ 0 };
}

int FinishGameplayScreen(void)
{
    return finishScreen;
}
