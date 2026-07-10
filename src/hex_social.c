/**********************************************************************************************
*
*   Beehold - Clickable social icons (Discord / X)
*
**********************************************************************************************/

#include "hex_social.h"

#define SOCIAL_ICON_SIZE 32.0f
#define SOCIAL_ICON_GAP 16.0f

static Texture2D discordTex = { 0 };
static Texture2D xTex = { 0 };
static Rectangle discordBtn = { 0 };
static Rectangle xBtn = { 0 };

void HexSocialInit(void)
{
    discordTex = LoadTexture("resources/icon_discord.png");
    xTex = LoadTexture("resources/icon_x.png");
    // Smooth logos at small size
    SetTextureFilter(discordTex, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(xTex, TEXTURE_FILTER_BILINEAR);
    HexSocialLayoutBottomCenter((float)GetScreenHeight() - 56.0f);
}

void HexSocialUnload(void)
{
    UnloadTexture(discordTex);
    discordTex = (Texture2D){ 0 };
    UnloadTexture(xTex);
    xTex = (Texture2D){ 0 };
}

void HexSocialLayoutAt(float x, float y)
{
    discordBtn = (Rectangle){ x, y, SOCIAL_ICON_SIZE, SOCIAL_ICON_SIZE };
    xBtn = (Rectangle){ x + SOCIAL_ICON_SIZE + SOCIAL_ICON_GAP, y, SOCIAL_ICON_SIZE, SOCIAL_ICON_SIZE };
}

void HexSocialLayoutBottomCenter(float y)
{
    float total = SOCIAL_ICON_SIZE*2.0f + SOCIAL_ICON_GAP;
    float x = ((float)GetScreenWidth() - total) * 0.5f;
    HexSocialLayoutAt(x, y);
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
}
