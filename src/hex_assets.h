/**********************************************************************************************
*
*   Beehold - Global texture bag (loaded once at startup)
*
**********************************************************************************************/

#ifndef HEX_ASSETS_H
#define HEX_ASSETS_H

#include "raylib.h"

typedef struct HexAssets
{
    Texture2D hexfield;
    Texture2D hexpond;
    Texture2D wasp;
    Texture2D hearts;
    Texture2D flower;
    Texture2D bubbles;
    Texture2D star;
    Texture2D fire;
    Texture2D speaker;
    Texture2D ratingStar;
    Texture2D ratingStarEmpty;
    Texture2D bee;
    Texture2D iconDiscord;
    Texture2D iconX;
    Texture2D iconTwitch;
} HexAssets;

extern HexAssets assets;

// Load every game texture once. Call after InitWindow.
void HexAssetsLoad(void);

#endif // HEX_ASSETS_H
