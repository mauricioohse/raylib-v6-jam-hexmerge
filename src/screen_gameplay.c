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
static int lives = PLAYER_MAX_LIVES;
static float levelTimer = 0.0f;     // resets each level
static bool runActive = false;
static bool levelPaused = true;     // wait for first steer before bee/wasps move
static bool gamePaused = false;     // pause menu (P / ESC)
static bool starMusicPlaying = false;
static bool moveModeRelative = false;   // false = WASD absolute (default), true = A/D relative
static bool hardcore = false;
static int checkpointLevel = 0;
static bool levelTookDamage = false;
static float checkpointBannerTimer = 0.0f;
static float fireFailDelay = 0.0f;   // show red shake before applying damage
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

    int fontSize = 28;
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
    for (int i = 0; i < lastRun.levelsRecorded; i++)
        lastRun.totalTime += lastRun.levels[i].timeSec;
    lastRunTime = lastRun.totalTime;
    levelTimer = 0.0f;
}

static void RecordCurrentLevelStats(void)
{
    if (lastRun.levelsRecorded >= HEX_RUN_MAX_LEVELS) return;

    int i = lastRun.levelsRecorded++;
    lastRun.levels[i].timeSec = levelTimer;
    lastRun.totalTime += levelTimer;
}

static void TryActivateCheckpoint(void)
{
    if (hardcore) return;
    const HexLevelDef *def = HexLevelGetDef(currentLevelIndex);
    if (def == NULL || !def->checkpoint) return;
    if (currentLevelIndex < checkpointLevel) return;

    checkpointLevel = currentLevelIndex;
    PlaySound(fxCheckpoint);
    checkpointBannerTimer = 2.0f;
}

static void LoadCurrentLevel(void)
{
    StopStarMusic();
    HexLevelLoad(&level, currentLevelIndex, hexTexture, pondTexture, flowerTexture, bubbleTexture, starTexture, BEE_SPEED);
    levelPaused = true;
    levelTimer = 0.0f;
    levelTookDamage = false;
    fireFailDelay = 0.0f;
    beeAnim.rotation = HexBeeRotationDeg(&level.bee, &level.grid);
    UpdateAnimation(&beeAnim);
}

static void FinishRun(bool won)
{
    StopStarMusic();
    runActive = false;

    // Capture the level we just finished / died on (if not already recorded)
    RecordCurrentLevelStats();

    lastRun.won = won;
    lastRun.hardcore = hardcore;
    lastRunTime = lastRun.totalTime;

    HexScoresAppendRunCsv(&lastRun);
    if (won) HexScoresSubmit(lastRun.totalTime, hardcore);

    finishScreen = 1;
    PlaySound(won? fxWin : fxLife);
}

// Returns true if the run ended (caller should return from Update).
static bool ApplyDamage(void)
{
    StopStarMusic();
    levelTookDamage = true;

    if (hardcore)
    {
        FinishRun(false);
        return true;
    }

    lives--;
    PlaySound(fxLife);

    if (lives <= 0)
    {
        // Soft fail: restart from last checkpoint with full lives
        currentLevelIndex = checkpointLevel;
        lives = PLAYER_MAX_LIVES;
        TruncateRunStatsToCheckpoint();
        LoadCurrentLevel();
        TryActivateCheckpoint();
        return false;
    }

    // Lives remain: retry this level
    LoadCurrentLevel();
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
    hardcore = startHardcore;
    startHardcore = false;
    moveModeRelative = startHardMode;   // title MOVE: A/D vs WASD
    // keep startHardMode so title remembers the choice after returning from a run
    lives = hardcore? 1 : PLAYER_MAX_LIVES;
    currentLevelIndex = 0;
    checkpointLevel = 0;
    levelTookDamage = false;
    checkpointBannerTimer = 0.0f;
    fireFailDelay = 0.0f;
    runActive = true;
    ResetRunStats();
    lastRun.hardcore = hardcore;
    SetMusicPitch(music, hardcore? 1.10f : 1.0f);

    hexTexture = LoadTexture("resources/hexfield.png");
    pondTexture = LoadTexture("resources/hexpond.png");
    waspTexture = LoadTexture("resources/wasp.png");
    heartsTexture = LoadTexture("resources/hearts.png");
    flowerTexture = LoadTexture("resources/flower.png");
    bubbleTexture = LoadTexture("resources/bubbles.png");
    starTexture = LoadTexture("resources/star.png");
    fireTexture = LoadTexture("resources/fire.png");
    speakerTexture = LoadTexture("resources/speaker.png");
    SetTextureFilter(bubbleTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(starTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(fireTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(speakerTexture, TEXTURE_FILTER_POINT);

    LayoutPauseMenu();
    HexBackgroundInit(&fallBg, hexTexture, HEX_BG_GAMEPLAY);
    LoadCurrentLevel();
    TryActivateCheckpoint();
}

void UpdateGameplayScreen(void)
{
    float dt = GetFrameTime();
    HexBackgroundUpdate(&fallBg, dt);

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))
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
            lives = hardcore? 1 : PLAYER_MAX_LIVES;
            checkpointLevel = currentLevelIndex;
            finishScreen = 0;
            ResetRunStats();
            lastRun.hardcore = hardcore;
            LoadCurrentLevel();
            TryActivateCheckpoint();
            PlaySound(fxCoin);
            return;
        }
    }
    if (IsKeyPressed(KEY_ZERO))
    {
        currentLevelIndex = 9;  // level 10
        lives = hardcore? 1 : PLAYER_MAX_LIVES;
        checkpointLevel = currentLevelIndex;
        finishScreen = 0;
        ResetRunStats();
        lastRun.hardcore = hardcore;
        LoadCurrentLevel();
        TryActivateCheckpoint();
        PlaySound(fxCoin);
        return;
    }
    // < / , previous   > / . next
    if (IsKeyPressed(KEY_COMMA))
    {
        if (currentLevelIndex > 0) currentLevelIndex--;
        else currentLevelIndex = HexLevelCount() - 1;
        lives = hardcore? 1 : PLAYER_MAX_LIVES;
        checkpointLevel = currentLevelIndex;
        finishScreen = 0;
        ResetRunStats();
        lastRun.hardcore = hardcore;
        LoadCurrentLevel();
        TryActivateCheckpoint();
        PlaySound(fxCoin);
        return;
    }
    if (IsKeyPressed(KEY_PERIOD))
    {
        currentLevelIndex = (currentLevelIndex + 1)%HexLevelCount();
        lives = hardcore? 1 : PLAYER_MAX_LIVES;
        checkpointLevel = currentLevelIndex;
        finishScreen = 0;
        ResetRunStats();
        lastRun.hardcore = hardcore;
        LoadCurrentLevel();
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
        if (currentLevelIndex + 1 < HexLevelCount())
        {
            if (!hardcore && !levelTookDamage && lives < PLAYER_MAX_LIVES)
                lives++;

            RecordCurrentLevelStats();
            currentLevelIndex++;
            LoadCurrentLevel();
            if (!hardcore)
            {
                const HexLevelDef *def = HexLevelGetDef(currentLevelIndex);
                if (def != NULL && def->checkpoint && currentLevelIndex > checkpointLevel)
                {
                    checkpointLevel = currentLevelIndex;
                    PlaySound(fxCheckpoint);
                    checkpointBannerTimer = 2.0f;
                }
            }
            PlaySound(fxWin);
        }
        else
        {
            FinishRun(true);
        }
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

    if (hardcore)
    {
        const char *hc = "SPEEDRUN";
        int hw = MeasureText(hc, 18);
        DrawText(hc, GetScreenWidth() - 16 - hw, 16, 18, (Color){ 255, 110, 100, 255 });
    }
    else
    {
        DrawLives();
    }
    HexLevelDrawHint(&level);

    char levelLabel[32];
    snprintf(levelLabel, sizeof(levelLabel), "Level %d", currentLevelIndex + 1);
    DrawText(levelLabel, 16, 16, 20, LIGHTGRAY);
    DrawText(levelPaused? (moveModeRelative? "A/D to start" : "WASD to start")
                        : (moveModeRelative? "A/D turn" : "WASD move"),
             16, 40, 18, (Color){ 160, 170, 180, 255 });

    char timeBuf[16];
    HexScoresFormat(levelTimer, timeBuf, (int)sizeof(timeBuf));
    int tw = MeasureText(timeBuf, 22);
    DrawText(timeBuf, (GetScreenWidth() - tw)/2, 16, 22, (Color){ 255, 220, 70, 255 });

    if (checkpointBannerTimer > 0.0f)
    {
        float t = checkpointBannerTimer/2.0f;
        if (t > 1.0f) t = 1.0f;
        unsigned char a = (unsigned char)(t*255.0f);
        const char *banner = "CHECKPOINT!";
        int fontSize = 28;
        int bw = MeasureText(banner, fontSize);
        DrawText(banner, (GetScreenWidth() - bw)/2, GetScreenHeight()/4 - fontSize/2, fontSize,
                 (Color){ 255, 220, 70, a });
    }

    if (levelPaused)
    {
        const char *prompt = moveModeRelative? "Press A/D or arrows to start"
                                             : "Press WASD or arrows to start";
        int pw = MeasureText(prompt, 24);
        DrawText(prompt, (GetScreenWidth() - pw)/2, GetScreenHeight() - 56, 24, (Color){ 255, 220, 70, 255 });
    }

#if defined(_DEBUG)
    DrawText(TextFormat("DEBUG: 1-9/0 jump  </, >/ . step  G=%s  H=fill",
                        moveModeRelative? "A/D" : "WASD"),
             16, GetScreenHeight() - 28, 16, (Color){ 120, 140, 160, 255 });
#endif

    if (gamePaused)
    {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.65f));

        const char *paused = "PAUSED";
        int pausedSize = 48;
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
        DrawText("<", (int)(pauseVolDownBtn.x + 10), (int)(pauseVolDownBtn.y + 4), 28, RAYWHITE);

        char volText[8];
        snprintf(volText, sizeof(volText), "%d", volumeLevel);
        int vw = MeasureText(volText, 28);
        DrawText(volText,
                 (int)(pauseVolDownBtn.x + pauseVolDownBtn.width +
                       (pauseVolUpBtn.x - (pauseVolDownBtn.x + pauseVolDownBtn.width) - vw)*0.5f),
                 (int)(volY + 4), 28, RAYWHITE);

        DrawRectangleRec(pauseVolUpBtn, upFill);
        DrawRectangleLinesEx(pauseVolUpBtn, 2.0f, (Color){ 90, 100, 120, 255 });
        DrawText(">", (int)(pauseVolUpBtn.x + 10), (int)(pauseVolUpBtn.y + 4), 28, RAYWHITE);

        const char *hint = "P / ESC to resume";
        int hw = MeasureText(hint, 18);
        DrawText(hint, (GetScreenWidth() - hw)/2,
                 (int)(pauseVolDownBtn.y + pauseVolDownBtn.height + 28.0f), 18,
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
}

int FinishGameplayScreen(void)
{
    return finishScreen;
}
