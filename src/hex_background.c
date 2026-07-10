/**********************************************************************************************
*
*   Beehold - Shared falling-hex backdrop
*
**********************************************************************************************/

#include "hex_background.h"

#include <string.h>

typedef struct HexBgParams
{
    int maxActive;
    int seedMin;
    int seedMax;
    float spawnMin;
    float spawnMax;
    float speedMin;
    float speedMax;
    float scaleMin;
    float scaleMax;
    float rotSpeed;
    int alphaMin;
    int alphaMax;
} HexBgParams;

static const HexBgParams STYLE_PARAMS[] = {
    // HEX_BG_TITLE
    { 20, 6, 10, 0.5f, 2.5f, 28.0f, 55.0f, 0.70f, 1.30f, 18.0f, 40, 90 },
    // HEX_BG_GAMEPLAY
    { 14, 4, 7, 0.7f, 2.8f, 22.0f, 48.0f, 0.55f, 1.05f, 14.0f, 28, 55 },
};

static float RandRange(float a, float b)
{
    return a + (b - a)*((float)GetRandomValue(0, 10000)/10000.0f);
}

static const HexBgParams *Params(HexBgStyle style)
{
    if ((style < 0) || (style > HEX_BG_GAMEPLAY)) style = HEX_BG_TITLE;
    return &STYLE_PARAMS[style];
}

static int ActiveCount(const HexBackground *bg)
{
    int n = 0;
    for (int i = 0; i < HEX_BG_FALL_MAX; i++)
    {
        if (bg->hexes[i].active) n++;
    }
    return n;
}

static void Spawn(HexBackground *bg, bool scatterY)
{
    if (bg->texture.id == 0) return;
    if (ActiveCount(bg) >= bg->maxActive) return;

    int slot = -1;
    for (int i = 0; i < HEX_BG_FALL_MAX; i++)
    {
        if (!bg->hexes[i].active) { slot = i; break; }
    }
    if (slot < 0) return;

    const HexBgParams *p = Params(bg->style);
    float scale = RandRange(p->scaleMin, p->scaleMax);
    float w = (float)bg->texture.width*scale;
    float h = (float)bg->texture.height*scale;
    float sw = (float)GetScreenWidth();

    HexFallHex *hex = &bg->hexes[slot];
    hex->active = true;
    hex->scale = scale;
    hex->speed = RandRange(p->speedMin, p->speedMax);
    hex->rotation = RandRange(0.0f, 360.0f);
    hex->rotSpeed = RandRange(-p->rotSpeed, p->rotSpeed);
    hex->alpha = (unsigned char)GetRandomValue(p->alphaMin, p->alphaMax);
    hex->pos.x = RandRange(-w*0.25f, sw - w*0.75f);
    if (scatterY)
        hex->pos.y = RandRange(-40.0f, (float)GetScreenHeight()*0.75f);
    else
        hex->pos.y = -h - RandRange(0.0f, 40.0f);
}

static void ResetSpawnTimer(HexBackground *bg)
{
    const HexBgParams *p = Params(bg->style);
    bg->spawnTimer = RandRange(p->spawnMin, p->spawnMax);
}

void HexBackgroundInit(HexBackground *bg, Texture2D texture, HexBgStyle style)
{
    memset(bg, 0, sizeof(*bg));
    bg->texture = texture;
    bg->style = style;
    bg->maxActive = Params(style)->maxActive;
    if (bg->maxActive > HEX_BG_FALL_MAX) bg->maxActive = HEX_BG_FALL_MAX;

    const HexBgParams *p = Params(style);
    int seedCount = GetRandomValue(p->seedMin, p->seedMax);
    for (int i = 0; i < seedCount; i++) Spawn(bg, true);
    ResetSpawnTimer(bg);
}

void HexBackgroundUpdate(HexBackground *bg, float dt)
{
    if (bg == NULL) return;

    bg->spawnTimer -= dt;
    if (bg->spawnTimer <= 0.0f)
    {
        Spawn(bg, false);
        ResetSpawnTimer(bg);
    }

    float sh = (float)GetScreenHeight();
    for (int i = 0; i < HEX_BG_FALL_MAX; i++)
    {
        HexFallHex *hex = &bg->hexes[i];
        if (!hex->active) continue;

        hex->pos.y += hex->speed*dt;
        hex->rotation += hex->rotSpeed*dt;

        float h = (float)bg->texture.height*hex->scale;
        if (hex->pos.y > sh + h) hex->active = false;
    }
}

void HexBackgroundDraw(const HexBackground *bg)
{
    if ((bg == NULL) || (bg->texture.id == 0)) return;

    float tw = (float)bg->texture.width;
    float th = (float)bg->texture.height;
    Rectangle src = { 0, 0, tw, th };

    for (int i = 0; i < HEX_BG_FALL_MAX; i++)
    {
        const HexFallHex *hex = &bg->hexes[i];
        if (!hex->active) continue;

        float dw = tw*hex->scale;
        float dh = th*hex->scale;
        Rectangle dst = { hex->pos.x + dw*0.5f, hex->pos.y + dh*0.5f, dw, dh };
        Vector2 origin = { dw*0.5f, dh*0.5f };
        Color tint = (Color){ 150, 105, 70, hex->alpha };
        DrawTexturePro(bg->texture, src, dst, origin, hex->rotation, tint);
    }
}
