/**********************************************************************************************
*
*   Beehold - Clickable social icons (Discord / X / Twitch)
*
**********************************************************************************************/

#include "hex_social.h"

static Texture2D discordTex = { 0 };
static Texture2D xTex = { 0 };
static Texture2D twitchTex = { 0 };
static Rectangle discordBtn = { 0 };
static Rectangle xBtn = { 0 };
static Rectangle twitchBtn = { 0 };

void HexSocialInit(void)
{
    discordTex = LoadTexture("resources/icon_discord.png");
    xTex = LoadTexture("resources/icon_x.png");
    twitchTex = LoadTexture("resources/icon_twitch.png");
    SetTextureFilter(discordTex, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(xTex, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(twitchTex, TEXTURE_FILTER_BILINEAR);
    HexSocialLayoutTitle((float)GetScreenHeight() - 56.0f);
}

void HexSocialUnload(void)
{
    UnloadTexture(discordTex);
    discordTex = (Texture2D){ 0 };
    UnloadTexture(xTex);
    xTex = (Texture2D){ 0 };
    UnloadTexture(twitchTex);
    twitchTex = (Texture2D){ 0 };
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
    DrawIcon(discordTex, discordBtn, CheckCollisionPointRec(mouse, discordBtn));
    DrawIcon(xTex, xBtn, CheckCollisionPointRec(mouse, xBtn));
    DrawIcon(twitchTex, twitchBtn, CheckCollisionPointRec(mouse, twitchBtn));
}
