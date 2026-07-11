/**********************************************************************************************
*
*   Beehold - Debug cover studio (DEBUG builds only)
*
*   Pose a level, overlay title + hero bee, export an itch.io cover (315:250).
*   Title menu: press Y to enter.
*     S = ../gdd-images/cover1.png (static, 1260x1000)
*     F = ../gdd-images/cover1.gif (hero flap loop, 630x500 — itch hover-plays GIF covers)
*
**********************************************************************************************/

#if defined(_DEBUG)

#include "raylib.h"
#include "screens.h"
#include "hex_level.h"
#include "hex_background.h"
#include "hex_trail.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Cover export: itch.io 315:250 (PNG at 2x, GIF at 1x for smaller upload)
//----------------------------------------------------------------------------------
#define COVER_EXPORT_W 1260
#define COVER_EXPORT_H 1000
#define COVER_GIF_W 630
#define COVER_GIF_H 500
#define COVER_GIF_FRAMES 4
#define COVER_GIF_DELAY_MS 110
#define COVER_BEE_SPEED 120.0f
#define COVER_DEFAULT_LEVEL 15   // level 16 — stars + mixed board
#define COVER_HERO_SCALE 15.5f   // slightly smaller so hex board still reads
#define COVER_WASP_SCALE (COVER_HERO_SCALE*0.5f)
#define COVER_TITLE_SIZE 88
#define COVER_TITLE_Y_OFFSET 155   // from vertical center; lower = closer to board
#define COVER_HERO_GAP 28          // pixels between title baseline and hero top
#define COVER_HERO_BOB_AMP 6.0f
#define COVER_HERO_BOB_SPEED 3.2f
#define COVER_WASP_MARGIN 78.0f
#define COVER_WASP_FRAMES 4
#define COVER_GRID_SCALE_DEFAULT 1.20f
#define COVER_GRID_SCALE_MIN 0.70f
#define COVER_GRID_SCALE_MAX 1.80f
#define COVER_GRID_SCALE_STEP 0.05f
#define HIT_RADIUS 14.0f

// Cover-only: mild green/bright lift on filled grass (gameplay stays WHITE)
static const Color COVER_HEX_FILLED = { 210, 255, 200, 255 };
static const Color COVER_HEX_DEAD = { 140, 120, 70, 255 };
// Brighter than in-game black wasps so they read on the dark cover
static const Color COVER_WASP_TINT = { 95, 95, 110, 255 };
static const Color COVER_BG = { 36, 40, 50, 255 };

typedef enum CoverSaveKind
{
    COVER_SAVE_NONE = 0,
    COVER_SAVE_PNG,
    COVER_SAVE_GIF
} CoverSaveKind;

//----------------------------------------------------------------------------------
// Module state
//----------------------------------------------------------------------------------
static int finishScreen = 0;
static HexLevel level = { 0 };
static int currentLevelIndex = COVER_DEFAULT_LEVEL;
static Texture2D hexTexture = { 0 };
static Texture2D pondTexture = { 0 };
static Texture2D waspTexture = { 0 };
static Texture2D flowerTexture = { 0 };
static Texture2D bubbleTexture = { 0 };
static Texture2D starTexture = { 0 };
static Texture2D fireTexture = { 0 };
static Texture2D fallHexTexture = { 0 };
static HexBackground fallBg = { 0 };
static Animation coverBeeAnim = { 0 };
static bool moveModeRelative = true;   // A/D so S is free for save
static bool levelPaused = true;
static bool showTitle = true;
static bool showHeroBee = true;
static CoverSaveKind pendingSave = COVER_SAVE_NONE;
static float saveFlash = 0.0f;
static char saveStatus[160] = { 0 };
static float heroBobPhase = 0.0f;
static int heroFrameOverride = -1;   // >=0 forces hero spritesheet frame (GIF capture)
static float gridScale = COVER_GRID_SCALE_DEFAULT;

static void DrawCoverArt(int canvasW, int canvasH);

//----------------------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------------------
static void LoadCoverLevel(void)
{
    HexLevelLoad(&level, currentLevelIndex, hexTexture, pondTexture,
                 flowerTexture, bubbleTexture, starTexture, COVER_BEE_SPEED);
    levelPaused = true;
    coverBeeAnim.rotation = HexBeeRotationDeg(&level.bee, &level.grid);
}

static void FillBoardShowcase(void)
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
        level.flowers.flowers[i].animFrame = 3;
        level.flowers.flowers[i].animTimer = 0.0f;
    }
    levelPaused = false;
}

static void DrawCoverGrid(const HexGrid *grid)
{
    for (int i = 0; i < grid->faceCount; i++)
    {
        const HexFace *face = &grid->faces[i];
        Vector2 c = face->center;

        Texture2D tex = grid->hexTexture;
        Color tint = WHITE;

        if (face->kind == HEX_FACE_WATER && grid->pondTexture.id != 0)
        {
            tex = grid->pondTexture;
            tint = face->filled? (Color){ 255, 240, 160, 255 } : WHITE;
        }
        else if (face->starJail)
            tint = face->filled? (Color){ 120, 130, 118, 255 } : (Color){ 70, 78, 72, 255 };
        else
            tint = face->filled? COVER_HEX_FILLED : COVER_HEX_DEAD;

        float texW = (float)tex.width;
        float texH = (float)tex.height;
        Rectangle src = { 0, 0, texW, texH };
        Rectangle dst = { c.x - texW*0.5f, c.y - texH*0.5f, texW, texH };
        DrawTexturePro(tex, src, dst, (Vector2){ 0, 0 }, 0.0f, tint);
    }

    for (int i = 0; i < grid->edgeCount; i++)
    {
        Vector2 a = grid->vertices[grid->edges[i].v0].pos;
        Vector2 b = grid->vertices[grid->edges[i].v1].pos;
        Color col = grid->edges[i].painted? (Color){ 255, 220, 70, 255 } : (Color){ 40, 40, 40, 80 };
        float thick = grid->edges[i].painted? 3.0f : 1.0f;
        DrawLineEx(a, b, thick, col);
    }
}

static void DrawCoverLevel(void)
{
    DrawCoverGrid(&level.grid);
    HexGridDrawFire(&level.grid, fireTexture);
    HexBeeDrawLiveTrail(&level.bee, &level.grid);
    HexFlowerFieldDraw(&level.flowers, &level.grid);
    HexStarFieldDraw(&level.stars, &level.grid);

    bool powered = HexLevelStarPowered(&level);
    for (int i = 0; i < level.enemyCount; i++)
        HexEnemyDraw(&level.enemies[i], &level.grid, waspTexture, powered);
}

static void DrawCoverTitle(int canvasW, int canvasH)
{
    if (!showTitle) return;

    const char *title = "BEEHOLD";
    int fontSize = COVER_TITLE_SIZE;
    int tw = MeasureText(title, fontSize);
    int x = (canvasW - tw)/2;
    int y = canvasH/2 - COVER_TITLE_Y_OFFSET;

    Color outline = BLACK;
    Color ink = { 255, 196, 64, 255 };   // honey yellow

    // Hard black outline around default raylib font (cover-only)
    const int o = 3;
    for (int dy = -o; dy <= o; dy++)
    {
        for (int dx = -o; dx <= o; dx++)
        {
            if ((dx == 0) && (dy == 0)) continue;
            DrawText(title, x + dx, y + dy, fontSize, outline);
        }
    }
    DrawText(title, x, y, fontSize, ink);
}

static Vector2 HeroBeeCenter(int canvasW, int canvasH)
{
    float frameH = (float)coverBeeAnim.text.height/(float)coverBeeAnim.frameCnt;
    float drawH = frameH*COVER_HERO_SCALE;
    float bob = sinf(heroBobPhase)*COVER_HERO_BOB_AMP;

    float titleY = (float)(canvasH/2 - COVER_TITLE_Y_OFFSET);
    float y = titleY + (float)COVER_TITLE_SIZE + (float)COVER_HERO_GAP + drawH*0.5f + bob;
    return (Vector2){ (float)canvasW*0.5f, y };
}

static void DrawHeroBee(int canvasW, int canvasH)
{
    if (!showHeroBee || (coverBeeAnim.text.id == 0)) return;

    float frameW = (float)coverBeeAnim.text.width;
    float frameH = (float)coverBeeAnim.text.height/(float)coverBeeAnim.frameCnt;
    float drawW = frameW*COVER_HERO_SCALE;
    float drawH = frameH*COVER_HERO_SCALE;

    int frame = (heroFrameOverride >= 0)? heroFrameOverride : coverBeeAnim.frame;
    if (frame < 0) frame = 0;
    if (frame >= coverBeeAnim.frameCnt) frame = coverBeeAnim.frameCnt - 1;

    Vector2 center = HeroBeeCenter(canvasW, canvasH);
    // Face slightly up-left so it reads as flying into the title
    float rotation = -12.0f;

    Rectangle src = { 0, frameH*(float)frame, frameW, frameH };
    Rectangle dst = { center.x, center.y, drawW, drawH };
    Vector2 origin = { drawW*0.5f, drawH*0.5f };
    DrawTexturePro(coverBeeAnim.text, src, dst, origin, rotation, WHITE);
}

static void DrawCornerWasps(int canvasW, int canvasH)
{
    if (waspTexture.id == 0) return;

    Vector2 target = HeroBeeCenter(canvasW, canvasH);

    // Place in the itch crop rectangle (center band of the square canvas)
    float cropH = (float)canvasW*(250.0f/315.0f);
    if (cropH > (float)canvasH) cropH = (float)canvasH;
    float cropTop = ((float)canvasH - cropH)*0.5f;
    float cropBot = cropTop + cropH;
    float m = COVER_WASP_MARGIN;

    Vector2 corners[4] = {
        { m, cropTop + m },
        { (float)canvasW - m, cropTop + m },
        { m, cropBot - m },
        { (float)canvasW - m, cropBot - m },
    };

    float frameW = (float)waspTexture.width;
    float frameH = (float)waspTexture.height/(float)COVER_WASP_FRAMES;
    float drawW = frameW*COVER_WASP_SCALE;
    float drawH = frameH*COVER_WASP_SCALE;

    int frame = (heroFrameOverride >= 0)? (heroFrameOverride % COVER_WASP_FRAMES)
                                        : (coverBeeAnim.frame % COVER_WASP_FRAMES);

    for (int i = 0; i < 4; i++)
    {
        Vector2 pos = corners[i];
        // Sprite faces up (-y); +90 aligns nose with travel direction toward hero
        float rotation = atan2f(target.y - pos.y, target.x - pos.x)*(180.0f/(float)PI) + 90.0f;

        Rectangle src = { 0, frameH*(float)frame, frameW, frameH };
        Rectangle dst = { pos.x, pos.y, drawW, drawH };
        Vector2 origin = { drawW*0.5f, drawH*0.5f };
        DrawTexturePro(waspTexture, src, dst, origin, rotation, COVER_WASP_TINT);
    }
}

static void DrawCoverArt(int canvasW, int canvasH)
{
    DrawRectangle(0, 0, canvasW, canvasH, COVER_BG);
    HexBackgroundDraw(&fallBg);

    // Zoom only the board (title / hero / corner wasps stay screen-space)
    Camera2D cam = { 0 };
    cam.target = (Vector2){ (float)canvasW*0.5f, (float)canvasH*0.5f };
    cam.offset = cam.target;
    cam.rotation = 0.0f;
    cam.zoom = gridScale;
    BeginMode2D(cam);
        DrawCoverLevel();
        Vector2 beePos = HexBeePosition(&level.bee, &level.grid);
        DrawAnimation(&coverBeeAnim, beePos);
    EndMode2D();

    DrawCoverTitle(canvasW, canvasH);
    DrawCornerWasps(canvasW, canvasH);
    DrawHeroBee(canvasW, canvasH);
}

static void DrawChrome(void)
{
    const int fontSize = 18;
    int y = GetScreenHeight() - 44;
    DrawRectangle(0, y - 8, GetScreenWidth(), 52, (Color){ 0, 0, 0, 180 });

    const char *line1 = TextFormat(
        "S = PNG   F = GIF   ESC/B = back   T/R = title/hero   -/= grid %.2f",
        gridScale);
    DrawText(line1, 12, y, fontSize, RAYWHITE);

    const char *line2 = TextFormat(
        "DEBUG: 1-9/0 jump  </> step  G=%s  H=fill  (pose with A/D)",
        moveModeRelative? "A/D" : "WASD");
    DrawText(line2, 12, y + 20, fontSize, (Color){ 180, 190, 210, 255 });

    if (saveFlash > 0.0f && saveStatus[0] != '\0')
    {
        int tw = MeasureText(saveStatus, 22);
        DrawText(saveStatus, (GetScreenWidth() - tw)/2, 24, 22, (Color){ 255, 220, 90, 255 });
    }
}

static Image CaptureCoverCropped(int outW, int outH)
{
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    Image empty = { 0 };

    RenderTexture2D rt = LoadRenderTexture(sw, sh);
    if (rt.id == 0) return empty;

    BeginTextureMode(rt);
        DrawCoverArt(sw, sh);
    EndTextureMode();

    Image img = LoadImageFromTexture(rt.texture);
    ImageFlipVertical(&img);
    UnloadRenderTexture(rt);

    float cropH = (float)sw*(250.0f/315.0f);
    if (cropH > (float)sh) cropH = (float)sh;
    int cropY = (int)(((float)sh - cropH)*0.5f);
    if (cropY < 0) cropY = 0;

    Image cropped = ImageFromImage(img, (Rectangle){ 0, (float)cropY, (float)sw, cropH });
    UnloadImage(img);
    ImageResize(&cropped, outW, outH);
    return cropped;
}

static const char *TryExportPaths(Image img, const char *const *paths, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (ExportImage(img, paths[i])) return paths[i];
    }
    return NULL;
}

static bool ExportCoverPng(void)
{
    Image cropped = CaptureCoverCropped(COVER_EXPORT_W, COVER_EXPORT_H);
    if (cropped.data == NULL)
    {
        snprintf(saveStatus, sizeof(saveStatus), "PNG capture failed");
        saveFlash = 3.0f;
        return false;
    }

    const char *paths[] = {
        "../gdd-images/cover1.png",
        "gdd-images/cover1.png",
        "cover1.png",
    };
    const char *used = TryExportPaths(cropped, paths, 3);
    UnloadImage(cropped);

    if (used)
        snprintf(saveStatus, sizeof(saveStatus), "Saved %s (%dx%d)", used, COVER_EXPORT_W, COVER_EXPORT_H);
    else
        snprintf(saveStatus, sizeof(saveStatus), "PNG save failed — check gdd-images path");

    saveFlash = 3.0f;
    return used != NULL;
}

// Assemble frame PNGs into an animated GIF via Pillow (already on this machine).
// framePrefix example: "../gdd-images/_cover_f"  → opens _cover_f0.png .. _cover_f3.png
static bool AssembleGifPython(const char *outGif, const char *framePrefix)
{
    char cmd[1024];
    // Use str(i) concat so paths never go through printf format again.
    snprintf(cmd, sizeof(cmd),
             "python -c \"from PIL import Image; "
             "fs=[Image.open(r'%s'+str(i)+'.png').convert('P',palette=Image.ADAPTIVE,colors=192) "
             "for i in range(%d)]; "
             "fs[0].save(r'%s',save_all=True,append_images=fs[1:],"
             "duration=%d,loop=0,optimize=True,disposal=2)\"",
             framePrefix, COVER_GIF_FRAMES, outGif, COVER_GIF_DELAY_MS);

    int rc = system(cmd);
    return rc == 0;
}

static bool ExportCoverGif(void)
{
    const char *gifPaths[] = {
        "../gdd-images/cover1.gif",
        "gdd-images/cover1.gif",
        "cover1.gif",
    };
    const char *framePrefixes[] = {
        "../gdd-images/_cover_f",
        "gdd-images/_cover_f",
        "_cover_f",
    };

    int slot = -1;
    for (int i = 0; i < 3; i++)
    {
        char probe[256];
        snprintf(probe, sizeof(probe), "%s0.png", framePrefixes[i]);
        FILE *f = fopen(probe, "wb");
        if (f) { fclose(f); remove(probe); slot = i; break; }
    }
    if (slot < 0)
    {
        snprintf(saveStatus, sizeof(saveStatus), "GIF: cannot write frame temp files");
        saveFlash = 3.0f;
        return false;
    }

    float savedBob = heroBobPhase;
    int savedOverride = heroFrameOverride;

    char framePath[256];
    bool framesOk = true;
    for (int i = 0; i < COVER_GIF_FRAMES; i++)
    {
        heroFrameOverride = i % coverBeeAnim.frameCnt;
        heroBobPhase = (float)i*(6.2831853f/(float)COVER_GIF_FRAMES);

        Image frame = CaptureCoverCropped(COVER_GIF_W, COVER_GIF_H);
        if (frame.data == NULL) { framesOk = false; break; }

        snprintf(framePath, sizeof(framePath), "%s%d.png", framePrefixes[slot], i);
        if (!ExportImage(frame, framePath)) framesOk = false;
        UnloadImage(frame);
        if (!framesOk) break;
    }

    heroFrameOverride = savedOverride;
    heroBobPhase = savedBob;

    if (!framesOk)
    {
        for (int i = 0; i < COVER_GIF_FRAMES; i++)
        {
            snprintf(framePath, sizeof(framePath), "%s%d.png", framePrefixes[slot], i);
            remove(framePath);
        }
        snprintf(saveStatus, sizeof(saveStatus), "GIF frame capture failed");
        saveFlash = 3.0f;
        return false;
    }

    bool ok = AssembleGifPython(gifPaths[slot], framePrefixes[slot]);

    for (int i = 0; i < COVER_GIF_FRAMES; i++)
    {
        snprintf(framePath, sizeof(framePath), "%s%d.png", framePrefixes[slot], i);
        remove(framePath);
    }

    if (ok)
        snprintf(saveStatus, sizeof(saveStatus), "Saved %s (%dx%d flap loop)",
                 gifPaths[slot], COVER_GIF_W, COVER_GIF_H);
    else
        snprintf(saveStatus, sizeof(saveStatus), "GIF assemble failed (need Python+Pillow)");

    saveFlash = 3.0f;
    return ok;
}

//----------------------------------------------------------------------------------
// Screen API
//----------------------------------------------------------------------------------
void InitCoverScreen(void)
{
    finishScreen = 0;
    pendingSave = COVER_SAVE_NONE;
    saveFlash = 0.0f;
    saveStatus[0] = '\0';
    showTitle = true;
    showHeroBee = true;
    moveModeRelative = true;
    currentLevelIndex = COVER_DEFAULT_LEVEL;
    heroBobPhase = 0.0f;
    heroFrameOverride = -1;
    gridScale = COVER_GRID_SCALE_DEFAULT;

    hexTexture = LoadTexture("resources/hexfield.png");
    pondTexture = LoadTexture("resources/hexpond.png");
    waspTexture = LoadTexture("resources/wasp.png");
    flowerTexture = LoadTexture("resources/flower.png");
    bubbleTexture = LoadTexture("resources/bubbles.png");
    starTexture = LoadTexture("resources/star.png");
    fireTexture = LoadTexture("resources/fire.png");
    fallHexTexture = LoadTexture("resources/hexfield.png");
    SetTextureFilter(waspTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(flowerTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(starTexture, TEXTURE_FILTER_POINT);

    HexBackgroundInit(&fallBg, fallHexTexture, HEX_BG_TITLE);
    coverBeeAnim = CreateAnimation("resources/bee.png", 2.0f, 4, 30);

    LoadCoverLevel();
}

void UpdateCoverScreen(void)
{
    float dt = GetFrameTime();
    HexBackgroundUpdate(&fallBg, dt);
    heroBobPhase += dt*COVER_HERO_BOB_SPEED;

    if (saveFlash > 0.0f)
    {
        saveFlash -= dt;
        if (saveFlash < 0.0f) saveFlash = 0.0f;
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_B))
    {
        finishScreen = 1;
        return;
    }

    if (IsKeyPressed(KEY_S))
    {
        pendingSave = COVER_SAVE_PNG;
        PlaySound(fxCoin);
        return;
    }
    if (IsKeyPressed(KEY_F))
    {
        pendingSave = COVER_SAVE_GIF;
        PlaySound(fxCoin);
        return;
    }

    if (IsKeyPressed(KEY_T))
    {
        showTitle = !showTitle;
        PlaySound(fxCoin);
    }
    if (IsKeyPressed(KEY_R))
    {
        showHeroBee = !showHeroBee;
        PlaySound(fxCoin);
    }
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT))
    {
        gridScale -= COVER_GRID_SCALE_STEP;
        if (gridScale < COVER_GRID_SCALE_MIN) gridScale = COVER_GRID_SCALE_MIN;
        PlaySound(fxCoin);
    }
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD))
    {
        // = is the unshifted + key on US layouts
        gridScale += COVER_GRID_SCALE_STEP;
        if (gridScale > COVER_GRID_SCALE_MAX) gridScale = COVER_GRID_SCALE_MAX;
        PlaySound(fxCoin);
    }

    for (int key = KEY_ONE; key <= KEY_NINE; key++)
    {
        if (IsKeyPressed(key))
        {
            currentLevelIndex = key - KEY_ONE;
            LoadCoverLevel();
            PlaySound(fxCoin);
            return;
        }
    }
    if (IsKeyPressed(KEY_ZERO))
    {
        currentLevelIndex = 9;
        LoadCoverLevel();
        PlaySound(fxCoin);
        return;
    }
    if (IsKeyPressed(KEY_COMMA))
    {
        if (currentLevelIndex > 0) currentLevelIndex--;
        else currentLevelIndex = HexLevelCount() - 1;
        LoadCoverLevel();
        PlaySound(fxCoin);
        return;
    }
    if (IsKeyPressed(KEY_PERIOD))
    {
        currentLevelIndex = (currentLevelIndex + 1)%HexLevelCount();
        LoadCoverLevel();
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
        FillBoardShowcase();
        PlaySound(fxPaint);
    }

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
            UpdateAnimation(&coverBeeAnim);
            return;
        }
    }
    else if (steer != HEX_BEE_INPUT_NONE)
    {
        HexBeeSetInput(&level.bee, steer);
    }

    HexLevelUpdate(&level, dt);
    (void)HexLevelBeeHit(&level, HIT_RADIUS);

    coverBeeAnim.rotation = HexBeeRotationDeg(&level.bee, &level.grid);
    UpdateAnimation(&coverBeeAnim);
}

void DrawCoverScreen(void)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    if (pendingSave != COVER_SAVE_NONE)
    {
        CoverSaveKind kind = pendingSave;
        pendingSave = COVER_SAVE_NONE;
        if (kind == COVER_SAVE_PNG) ExportCoverPng();
        else if (kind == COVER_SAVE_GIF) ExportCoverGif();
    }

    DrawCoverArt(sw, sh);
    DrawChrome();
}

void UnloadCoverScreen(void)
{
    UnloadTexture(hexTexture);
    UnloadTexture(pondTexture);
    UnloadTexture(waspTexture);
    UnloadTexture(flowerTexture);
    UnloadTexture(bubbleTexture);
    UnloadTexture(starTexture);
    UnloadTexture(fireTexture);
    UnloadTexture(fallHexTexture);
    UnloadTexture(coverBeeAnim.text);
    hexTexture = (Texture2D){ 0 };
    pondTexture = (Texture2D){ 0 };
    waspTexture = (Texture2D){ 0 };
    flowerTexture = (Texture2D){ 0 };
    bubbleTexture = (Texture2D){ 0 };
    starTexture = (Texture2D){ 0 };
    fireTexture = (Texture2D){ 0 };
    fallHexTexture = (Texture2D){ 0 };
    coverBeeAnim.text = (Texture2D){ 0 };
}

int FinishCoverScreen(void)
{
    return finishScreen;
}

#endif // _DEBUG
