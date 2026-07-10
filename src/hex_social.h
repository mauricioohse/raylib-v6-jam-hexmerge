/**********************************************************************************************
*
*   Beehold - Clickable social icons (Discord / X)
*
**********************************************************************************************/

#ifndef HEX_SOCIAL_H
#define HEX_SOCIAL_H

#include "raylib.h"

#define HEX_SOCIAL_DISCORD_URL "https://discord.gg/SGXVEwzUh"
#define HEX_SOCIAL_X_URL       "https://x.com/mohselabs"

void HexSocialInit(void);
void HexSocialUnload(void);

// Place icons at bottom-center (title) or a custom anchor.
void HexSocialLayoutBottomCenter(float y);
void HexSocialLayoutAt(float x, float y);

// Returns true if a social icon was clicked (opens URL).
bool HexSocialUpdate(void);
void HexSocialDraw(void);

#endif // HEX_SOCIAL_H
