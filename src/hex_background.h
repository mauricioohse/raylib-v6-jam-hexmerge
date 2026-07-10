/**********************************************************************************************
*
*   Beehold - Shared falling-hex backdrop (title + gameplay)
*
**********************************************************************************************/

#ifndef HEX_BACKGROUND_H
#define HEX_BACKGROUND_H

#include "raylib.h"

#define HEX_BG_FALL_MAX 20

typedef enum HexBgStyle
{
    HEX_BG_TITLE = 0,   // denser / brighter (menu)
    HEX_BG_GAMEPLAY     // quieter behind the board
} HexBgStyle;

typedef struct HexFallHex
{
    bool active;
    Vector2 pos;
    float speed;
    float scale;
    float rotation;
    float rotSpeed;
    unsigned char alpha;
} HexFallHex;

typedef struct HexBackground
{
    Texture2D texture;
    HexBgStyle style;
    HexFallHex hexes[HEX_BG_FALL_MAX];
    float spawnTimer;
    int maxActive;
} HexBackground;

void HexBackgroundInit(HexBackground *bg, Texture2D texture, HexBgStyle style);
void HexBackgroundUpdate(HexBackground *bg, float dt);
void HexBackgroundDraw(const HexBackground *bg);

#endif // HEX_BACKGROUND_H
