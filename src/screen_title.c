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

#include <stdio.h>

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
#define VOLUME_MIN 0
#define VOLUME_MAX 10
#define VOLUME_DEFAULT 5
#define SPEAKER_FRAME_COUNT 4
#define SPEAKER_SCALE 2.0f

static int framesCounter = 0;
static int finishScreen = 0;

static Rectangle startBtn = { 0 };
static Rectangle volDownBtn = { 0 };
static Rectangle volUpBtn = { 0 };
static Texture2D speakerTexture = { 0 };

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

//----------------------------------------------------------------------------------
// Title Screen Functions Definition
//----------------------------------------------------------------------------------
void InitTitleScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;

    speakerTexture = LoadTexture("resources/speaker.png");
    SetTextureFilter(speakerTexture, TEXTURE_FILTER_POINT);

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    startBtn = (Rectangle){ sw*0.5f - 140.0f, sh*0.5f - 28.0f, 280.0f, 56.0f };

    float volY = startBtn.y + startBtn.height + 48.0f;
    volDownBtn = (Rectangle){ sw*0.5f - 20.0f, volY, 36.0f, 36.0f };
    volUpBtn = (Rectangle){ sw*0.5f + 52.0f, volY, 36.0f, 36.0f };

    ApplyVolume();
}

void UpdateTitleScreen(void)
{
    framesCounter++;

    if (Clicked(startBtn) || IsKeyPressed(KEY_ENTER))
    {
        finishScreen = 2;   // GAMEPLAY
        PlaySound(fxCoin);
        return;
    }

    if (Clicked(volDownBtn) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
    {
        if (volumeLevel > VOLUME_MIN)
        {
            volumeLevel--;
            ApplyVolume();
            PlaySound(fxCoin);
        }
    }
    else if (Clicked(volUpBtn) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
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

    const char *title = "HEXMAN";
    int titleSize = 64;
    int tw = MeasureText(title, titleSize);
    DrawText(title, (GetScreenWidth() - tw)/2, GetScreenHeight()/2 - 160, titleSize, (Color){ 255, 179, 71, 255 });

    const char *subtitle = "paint the hexes, grow the flowers";
    int subSize = 18;
    int sw = MeasureText(subtitle, subSize);
    DrawText(subtitle, (GetScreenWidth() - sw)/2, GetScreenHeight()/2 - 90, subSize, LIGHTGRAY);

    Vector2 mouse = GetMousePosition();
    DrawMenuButton(startBtn, "START GAME", CheckCollisionPointRec(mouse, startBtn));

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
}

void UnloadTitleScreen(void)
{
    UnloadTexture(speakerTexture);
    speakerTexture = (Texture2D){ 0 };
}

int FinishTitleScreen(void)
{
    return finishScreen;
}
