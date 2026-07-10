/**********************************************************************************************
*
*   raylib - Advance Game template
*
*   Title Screen Functions Definitions (Init, Update, Draw, Unload)
*
*   Copyright (c) 2014-2022 Ramon Santamaria (@raysan5)
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "hex_background.h"

#include <math.h>
#include <stdio.h>

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
#define VOLUME_MIN 0
#define VOLUME_MAX 10
#define VOLUME_DEFAULT 5
#define SPEAKER_FRAME_COUNT 4
#define SPEAKER_SCALE 2.0f

#define FLY_BEE_MAX 6
#define FLY_BEE_FRAMES 4
#define FLY_BEE_FRAME_TIME 0.08f
#define FLY_BEE_SPAWN_MIN 1.5f
#define FLY_BEE_SPAWN_MAX 4.0f
#define FLY_BEE_SPEED_MIN 40.0f
#define FLY_BEE_SPEED_MAX 90.0f
#define FLY_BEE_SCALE_MIN 1.5f
#define FLY_BEE_SCALE_MAX 2.4f

typedef struct FlyBee
{
    bool active;
    Vector2 pos;
    float speedX;       // signed: left or right
    float bobAmp;
    float bobSpeed;
    float bobPhase;
    float baseY;
    float scale;
    float animTimer;
    int animFrame;
    Color tint;
} FlyBee;

static int framesCounter = 0;
static int finishScreen = 0;

static Rectangle startBtn = { 0 };
static Rectangle moveBtn = { 0 };
static Rectangle hardcoreBtn = { 0 };
static Rectangle volDownBtn = { 0 };
static Rectangle volUpBtn = { 0 };
static Texture2D speakerTexture = { 0 };
static Texture2D fallHexTexture = { 0 };
static Texture2D flyBeeTexture = { 0 };
static HexBackground fallBg = { 0 };

static FlyBee flyBees[FLY_BEE_MAX] = { 0 };
static float flyBeeSpawnTimer = 0.0f;

static const Color FLY_BEE_TINTS[] = {
    { 255, 220, 90, 160 },      // honey
    { 255, 240, 180, 140 },     // cream
    { 255, 200, 70, 150 },      // amber
    { 230, 230, 200, 130 },     // pale
    { 255, 170, 60, 145 },      // warm orange
};

//----------------------------------------------------------------------------------
// Helpers
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
    // speaker.png: 0 muted, 1 low, 2 mid, 3 high
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

static float RandRange(float a, float b)
{
    return a + (b - a)*((float)GetRandomValue(0, 10000)/10000.0f);
}

//----------------------------------------------------------------------------------
// Flying bees (background)
//----------------------------------------------------------------------------------
static int FlyBeeCount(void)
{
    int n = 0;
    for (int i = 0; i < FLY_BEE_MAX; i++)
    {
        if (flyBees[i].active) n++;
    }
    return n;
}

static void ResetFlyBeeSpawnTimer(void)
{
    flyBeeSpawnTimer = RandRange(FLY_BEE_SPAWN_MIN, FLY_BEE_SPAWN_MAX);
}

static void SpawnFlyBee(bool fromEdge)
{
    if (flyBeeTexture.id == 0) return;
    if (FlyBeeCount() >= FLY_BEE_MAX) return;

    int slot = -1;
    for (int i = 0; i < FLY_BEE_MAX; i++)
    {
        if (!flyBees[i].active) { slot = i; break; }
    }
    if (slot < 0) return;

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float scale = RandRange(FLY_BEE_SCALE_MIN, FLY_BEE_SCALE_MAX);
    float frameH = (float)flyBeeTexture.height/(float)FLY_BEE_FRAMES;
    float drawH = frameH*scale;
    float drawW = (float)flyBeeTexture.width*scale;

    bool goRight = (GetRandomValue(0, 1) == 0);
    float speed = RandRange(FLY_BEE_SPEED_MIN, FLY_BEE_SPEED_MAX);
    float y = RandRange(drawH, sh - drawH*2.0f);

    FlyBee *bee = &flyBees[slot];
    bee->active = true;
    bee->scale = scale;
    bee->speedX = goRight? speed : -speed;
    bee->bobAmp = RandRange(8.0f, 22.0f);
    bee->bobSpeed = RandRange(1.2f, 2.6f);
    bee->bobPhase = RandRange(0.0f, 6.28f);
    bee->baseY = y;
    bee->animTimer = 0.0f;
    bee->animFrame = GetRandomValue(0, FLY_BEE_FRAMES - 1);
    bee->tint = FLY_BEE_TINTS[GetRandomValue(0, (int)(sizeof(FLY_BEE_TINTS)/sizeof(FLY_BEE_TINTS[0])) - 1)];

    if (fromEdge)
    {
        bee->pos.x = goRight? (-drawW - 8.0f) : (sw + 8.0f);
    }
    else
    {
        bee->pos.x = RandRange(0.0f, sw - drawW);
    }
    bee->pos.y = y;
}

static void UpdateFlyBees(float dt)
{
    flyBeeSpawnTimer -= dt;
    if (flyBeeSpawnTimer <= 0.0f)
    {
        SpawnFlyBee(true);
        ResetFlyBeeSpawnTimer();
    }

    float sw = (float)GetScreenWidth();
    for (int i = 0; i < FLY_BEE_MAX; i++)
    {
        FlyBee *bee = &flyBees[i];
        if (!bee->active) continue;

        bee->pos.x += bee->speedX*dt;
        bee->bobPhase += bee->bobSpeed*dt;
        bee->pos.y = bee->baseY + sinf(bee->bobPhase)*bee->bobAmp;

        bee->animTimer += dt;
        while (bee->animTimer >= FLY_BEE_FRAME_TIME)
        {
            bee->animTimer -= FLY_BEE_FRAME_TIME;
            bee->animFrame = (bee->animFrame + 1)%FLY_BEE_FRAMES;
        }

        float drawW = (float)flyBeeTexture.width*bee->scale;
        if ((bee->speedX > 0.0f) && (bee->pos.x > sw + drawW)) bee->active = false;
        if ((bee->speedX < 0.0f) && (bee->pos.x < -drawW)) bee->active = false;
    }
}

static void DrawFlyBees(void)
{
    if (flyBeeTexture.id == 0) return;

    float frameW = (float)flyBeeTexture.width;
    float frameH = (float)flyBeeTexture.height/(float)FLY_BEE_FRAMES;

    for (int i = 0; i < FLY_BEE_MAX; i++)
    {
        const FlyBee *bee = &flyBees[i];
        if (!bee->active) continue;

        float drawW = frameW*bee->scale;
        float drawH = frameH*bee->scale;
        // Sprite faces up (-y); rotate so it points along flight direction
        float rotation = (bee->speedX >= 0.0f)? 90.0f : -90.0f;
        Rectangle src = { 0, frameH*(float)bee->animFrame, frameW, frameH };
        Rectangle dst = { bee->pos.x + drawW*0.5f, bee->pos.y + drawH*0.5f, drawW, drawH };
        Vector2 origin = { drawW*0.5f, drawH*0.5f };
        DrawTexturePro(flyBeeTexture, src, dst, origin, rotation, bee->tint);
    }
}

//----------------------------------------------------------------------------------
// Title Screen Functions Definition
//----------------------------------------------------------------------------------
void InitTitleScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;

    speakerTexture = LoadTexture("resources/speaker.png");
    SetTextureFilter(speakerTexture, TEXTURE_FILTER_POINT);

    fallHexTexture = LoadTexture("resources/hexfield.png");
    HexBackgroundInit(&fallBg, fallHexTexture, HEX_BG_TITLE);

    flyBeeTexture = LoadTexture("resources/bee.png");
    SetTextureFilter(flyBeeTexture, TEXTURE_FILTER_POINT);
    for (int i = 0; i < FLY_BEE_MAX; i++) flyBees[i].active = false;
    int beeSeed = GetRandomValue(2, 4);
    for (int i = 0; i < beeSeed; i++) SpawnFlyBee(false);
    ResetFlyBeeSpawnTimer();

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    startBtn = (Rectangle){ sw*0.5f - 140.0f, sh*0.5f - 56.0f, 280.0f, 52.0f };
    moveBtn = (Rectangle){ sw*0.5f - 140.0f, startBtn.y + startBtn.height + 14.0f, 280.0f, 44.0f };
    hardcoreBtn = (Rectangle){ sw*0.5f - 140.0f, moveBtn.y + moveBtn.height + 14.0f, 280.0f, 44.0f };

    float volY = hardcoreBtn.y + hardcoreBtn.height + 40.0f;
    volDownBtn = (Rectangle){ sw*0.5f - 20.0f, volY, 36.0f, 36.0f };
    volUpBtn = (Rectangle){ sw*0.5f + 52.0f, volY, 36.0f, 36.0f };

    ApplyVolume();
}

void UpdateTitleScreen(void)
{
    framesCounter++;
    float dt = GetFrameTime();
    HexBackgroundUpdate(&fallBg, dt);
    UpdateFlyBees(dt);

    if (Clicked(moveBtn))
    {
        startHardMode = !startHardMode;   // toggle WASD ↔ A/D
        PlaySound(fxCoin);
        return;
    }

    if (Clicked(startBtn) || IsKeyPressed(KEY_ENTER))
    {
        startHardcore = false;
        finishScreen = 2;   // GAMEPLAY (uses startHardMode for controls)
        PlaySound(fxCoin);
        return;
    }

    if (Clicked(hardcoreBtn))
    {
        startHardcore = true;
        finishScreen = 2;   // HARDCORE (uses startHardMode for controls)
        PlaySound(fxCoin);
        return;
    }

    if (Clicked(volDownBtn) || IsKeyPressed(KEY_LEFT))
    {
        if (volumeLevel > VOLUME_MIN)
        {
            volumeLevel--;
            ApplyVolume();
            PlaySound(fxCoin);
        }
    }
    else if (Clicked(volUpBtn) || IsKeyPressed(KEY_RIGHT))
    {
        if (volumeLevel < VOLUME_MAX)
        {
            volumeLevel++;
            ApplyVolume();
            PlaySound(fxCoin);
        }
    }
}

void DrawTitleScreen(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 24, 28, 36, 255 });
    HexBackgroundDraw(&fallBg);
    DrawFlyBees();

    const char *title = "BEEHOLD";
    int titleSize = 64;
    int tw = MeasureText(title, titleSize);
    DrawText(title, (GetScreenWidth() - tw)/2, GetScreenHeight()/2 - 160, titleSize, (Color){ 255, 179, 71, 255 });

    const char *subtitle = "paint the hexes, grow the flowers";
    int subSize = 18;
    int sw = MeasureText(subtitle, subSize);
    DrawText(subtitle, (GetScreenWidth() - sw)/2, GetScreenHeight()/2 - 90, subSize, LIGHTGRAY);

    Vector2 mouse = GetMousePosition();
    DrawMenuButton(startBtn, "START GAME", CheckCollisionPointRec(mouse, startBtn));
    DrawMenuButton(moveBtn, startHardMode? "MOVE: A/D" : "MOVE: WASD",
                   CheckCollisionPointRec(mouse, moveBtn));
    DrawMenuButton(hardcoreBtn, "HARDCORE", CheckCollisionPointRec(mouse, hardcoreBtn));

    // Volume row: speaker  <  N  >
    float volY = volDownBtn.y;
    float speakerDrawH = 16.0f*SPEAKER_SCALE;
    DrawSpeakerIcon((float)GetScreenWidth()*0.5f - 100.0f, volY + (volDownBtn.height - speakerDrawH)*0.5f);

    bool downHover = CheckCollisionPointRec(mouse, volDownBtn);
    bool upHover = CheckCollisionPointRec(mouse, volUpBtn);
    Color downFill = downHover? (Color){ 70, 80, 100, 255 } : (Color){ 40, 46, 58, 255 };
    Color upFill = upHover? (Color){ 70, 80, 100, 255 } : (Color){ 40, 46, 58, 255 };

    DrawRectangleRec(volDownBtn, downFill);
    DrawRectangleLinesEx(volDownBtn, 2.0f, (Color){ 90, 100, 120, 255 });
    DrawText("<", (int)(volDownBtn.x + 10), (int)(volDownBtn.y + 4), 28, RAYWHITE);

    char volText[8];
    snprintf(volText, sizeof(volText), "%d", volumeLevel);
    int vw = MeasureText(volText, 28);
    DrawText(volText, (int)(volDownBtn.x + volDownBtn.width + (volUpBtn.x - (volDownBtn.x + volDownBtn.width) - vw)*0.5f),
             (int)(volY + 4), 28, RAYWHITE);

    DrawRectangleRec(volUpBtn, upFill);
    DrawRectangleLinesEx(volUpBtn, 2.0f, (Color){ 90, 100, 120, 255 });
    DrawText(">", (int)(volUpBtn.x + 10), (int)(volUpBtn.y + 4), 28, RAYWHITE);

    // Tooltips last so they sit above the volume row
    if (CheckCollisionPointRec(mouse, moveBtn))
    {
        const int tipPad = 10;
        const int tipFont = 16;
        const int tipLineH = tipFont + 4;
        const char *line1 = startHardMode
            ? "A/D: turn left/right at each junction."
            : "WASD: move in absolute screen directions.";
        const char *line2 = "Click to switch. Applies to Start and Hardcore.";
        int tipW = MeasureText(line1, tipFont);
        int tipW2 = MeasureText(line2, tipFont);
        if (tipW2 > tipW) tipW = tipW2;
        int tipH = tipPad*2 + tipLineH*2;
        float tipX = moveBtn.x + (moveBtn.width - (float)(tipW + tipPad*2))*0.5f;
        float tipY = volDownBtn.y + volDownBtn.height + 12.0f;
        if (tipX < 8.0f) tipX = 8.0f;
        if (tipX + tipW + tipPad*2 > (float)GetScreenWidth() - 8.0f)
            tipX = (float)GetScreenWidth() - 8.0f - (float)(tipW + tipPad*2);

        DrawRectangle((int)tipX, (int)tipY, tipW + tipPad*2, tipH, (Color){ 36, 42, 54, 235 });
        DrawRectangleLinesEx((Rectangle){ tipX, tipY, (float)(tipW + tipPad*2), (float)tipH }, 2.0f,
                             (Color){ 255, 179, 71, 200 });
        DrawText(line1, (int)tipX + tipPad, (int)tipY + tipPad, tipFont, (Color){ 220, 225, 235, 255 });
        DrawText(line2, (int)tipX + tipPad, (int)tipY + tipPad + tipLineH, tipFont, (Color){ 220, 225, 235, 255 });
    }
    else if (CheckCollisionPointRec(mouse, hardcoreBtn))
    {
        const int tipPad = 10;
        const int tipFont = 16;
        const int tipLineH = tipFont + 4;
        const char *line1 = "No lives, no checkpoints. One death ends the run.";
        const char *line2 = "Separate speedrun scores.";
        int tipW = MeasureText(line1, tipFont);
        int tipW2 = MeasureText(line2, tipFont);
        if (tipW2 > tipW) tipW = tipW2;
        int tipH = tipPad*2 + tipLineH*2;
        float tipX = hardcoreBtn.x + (hardcoreBtn.width - (float)(tipW + tipPad*2))*0.5f;
        float tipY = volDownBtn.y + volDownBtn.height + 12.0f;
        if (tipX < 8.0f) tipX = 8.0f;
        if (tipX + tipW + tipPad*2 > (float)GetScreenWidth() - 8.0f)
            tipX = (float)GetScreenWidth() - 8.0f - (float)(tipW + tipPad*2);

        DrawRectangle((int)tipX, (int)tipY, tipW + tipPad*2, tipH, (Color){ 36, 42, 54, 235 });
        DrawRectangleLinesEx((Rectangle){ tipX, tipY, (float)(tipW + tipPad*2), (float)tipH }, 2.0f,
                             (Color){ 255, 110, 100, 200 });
        DrawText(line1, (int)tipX + tipPad, (int)tipY + tipPad, tipFont, (Color){ 220, 225, 235, 255 });
        DrawText(line2, (int)tipX + tipPad, (int)tipY + tipPad + tipLineH, tipFont, (Color){ 220, 225, 235, 255 });
    }
}

void UnloadTitleScreen(void)
{
    UnloadTexture(speakerTexture);
    speakerTexture = (Texture2D){ 0 };
    UnloadTexture(fallHexTexture);
    fallHexTexture = (Texture2D){ 0 };
    UnloadTexture(flyBeeTexture);
    flyBeeTexture = (Texture2D){ 0 };
}

int FinishTitleScreen(void)
{
    return finishScreen;
}
