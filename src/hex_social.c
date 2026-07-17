/**********************************************************************************************
*
*   Beehold - Clickable social icons (Discord / X / Twitch)
*
**********************************************************************************************/

#include "hex_social.h"
#include "hex_assets.h"

static Rectangle discordBtn = { 0 };
static Rectangle xBtn = { 0 };
static Rectangle twitchBtn = { 0 };

void HexSocialInit(void)
{
    HexSocialLayoutTitle((float)GetScreenHeight() - 56.0f);
}

void HexSocialLayoutAt(float x, float y)
{
    discordBtn = (Rectangle){ x, y, HEX_SOCIAL_ICON_SIZE, HEX_SOCIAL_ICON_SIZE };
    xBtn = (Rectangle){ x + HEX_SOCIAL_ICON_SIZE + HEX_SOCIAL_ICON_GAP, y,
                        HEX_SOCIAL_ICON_SIZE, HEX_SOCIAL_ICON_SIZE };
    twitchBtn = (Rectangle){ x + (HEX_SOCIAL_ICON_SIZE + HEX_SOCIAL_ICON_GAP)*2.0f, y,
                             HEX_SOCIAL_ICON_SIZE, HEX_SOCIAL_ICON_SIZE };
}

void HexSocialLayoutBottomCenter(float y)
{
    float total = HEX_SOCIAL_ICON_SIZE*3.0f + HEX_SOCIAL_ICON_GAP*2.0f;
    float x = ((float)GetScreenWidth() - total) * 0.5f;
    HexSocialLayoutAt(x, y);
}

void HexSocialLayoutTitle(float y)
{
    twitchBtn = (Rectangle){ 16.0f, y, HEX_SOCIAL_ICON_SIZE, HEX_SOCIAL_ICON_SIZE };

    float pair = HEX_SOCIAL_ICON_SIZE*2.0f + HEX_SOCIAL_ICON_GAP;
    float x = (float)GetScreenWidth() - 16.0f - pair;
    discordBtn = (Rectangle){ x, y, HEX_SOCIAL_ICON_SIZE, HEX_SOCIAL_ICON_SIZE };
    xBtn = (Rectangle){ x + HEX_SOCIAL_ICON_SIZE + HEX_SOCIAL_ICON_GAP, y,
                        HEX_SOCIAL_ICON_SIZE, HEX_SOCIAL_ICON_SIZE };
}

Rectangle HexSocialTwitchRect(void)
{
    return twitchBtn;
}

bool HexSocialUpdate(void)
{
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return false;

    Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, discordBtn))
    {
        OpenURL(HEX_SOCIAL_DISCORD_URL);
        return true;
    }
    if (CheckCollisionPointRec(mouse, xBtn))
    {
        OpenURL(HEX_SOCIAL_X_URL);
        return true;
    }
    if (CheckCollisionPointRec(mouse, twitchBtn))
    {
        OpenURL(HEX_SOCIAL_TWITCH_URL);
        return true;
    }
    return false;
}

static void DrawIcon(Texture2D tex, Rectangle dst, bool hovered)
{
    Color tint = hovered? WHITE : (Color){ 220, 225, 235, 230 };
    if (hovered)
    {
        DrawRectangleRounded(
            (Rectangle){ dst.x - 4.0f, dst.y - 4.0f, dst.width + 8.0f, dst.height + 8.0f },
            0.25f, 6, (Color){ 50, 58, 72, 200 });
    }
    DrawTexturePro(tex,
                   (Rectangle){ 0, 0, (float)tex.width, (float)tex.height },
                   dst, (Vector2){ 0, 0 }, 0.0f, tint);
}

void HexSocialDraw(void)
{
    Vector2 mouse = GetMousePosition();
    DrawIcon(assets.iconDiscord, discordBtn, CheckCollisionPointRec(mouse, discordBtn));
    DrawIcon(assets.iconX, xBtn, CheckCollisionPointRec(mouse, xBtn));
    DrawIcon(assets.iconTwitch, twitchBtn, CheckCollisionPointRec(mouse, twitchBtn));
}
