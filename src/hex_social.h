/**********************************************************************************************
*
*   Beehold - Clickable social icons (Discord / X / Twitch)
*
**********************************************************************************************/

#ifndef HEX_SOCIAL_H
#define HEX_SOCIAL_H

#include "raylib.h"

#define HEX_SOCIAL_DISCORD_URL "https://discord.gg/SGXVEwzUh"
#define HEX_SOCIAL_X_URL       "https://x.com/mohselabs"
#define HEX_SOCIAL_TWITCH_URL  "https://www.twitch.tv/mohselabs"
#define HEX_SOCIAL_ICON_SIZE   32.0f
#define HEX_SOCIAL_ICON_GAP    16.0f

void HexSocialInit(void);
void HexSocialUnload(void);

// Title: Twitch bottom-left; Discord + X bottom-center.
void HexSocialLayoutTitle(float y);
// Row of Discord, X, Twitch starting at (x, y) — used on ending screen.
void HexSocialLayoutAt(float x, float y);
void HexSocialLayoutBottomCenter(float y);

// Twitch icon rect after layout (for placing promo text beside it).
Rectangle HexSocialTwitchRect(void);

// Returns true if a social icon was clicked (opens URL).
bool HexSocialUpdate(void);
void HexSocialDraw(void);

#endif // HEX_SOCIAL_H
