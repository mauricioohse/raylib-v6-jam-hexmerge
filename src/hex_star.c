/**********************************************************************************************
*
*   Beehold - Star seed powerups implementation
*
**********************************************************************************************/

#include "hex_star.h"
#include "hex_assets.h"

#include <string.h>

#define STAR_FRAME_TIME 0.12f

void HexStarFieldInit(HexStarField *field, const int *faces, int faceCount)
{
    memset(field, 0, sizeof(*field));

    for (int i = 0; i < faceCount; i++)
    {
        int face = faces[i];
        if ((face < 0) || (field->count >= HEX_STAR_MAX)) continue;

        bool dup = false;
        for (int j = 0; j < field->count; j++)
        {
            if (field->stars[j].face == face) { dup = true; break; }
        }
        if (dup) continue;

        HexStar *s = &field->stars[field->count++];
        s->face = face;
        s->collected = false;
        s->animTimer = 0.0f;
        s->animFrame = 0;
    }
}

void HexStarFieldUpdate(HexStarField *field, float dt)
{
    if (field->powerTimer > 0.0f)
    {
        field->powerTimer -= dt;
        if (field->powerTimer < 0.0f) field->powerTimer = 0.0f;
    }

    for (int i = 0; i < field->count; i++)
    {
        HexStar *s = &field->stars[i];
        if (s->collected) continue;

        s->animTimer += dt;
        while (s->animTimer >= STAR_FRAME_TIME)
        {
            s->animTimer -= STAR_FRAME_TIME;
            s->animFrame = (s->animFrame + 1)%HEX_STAR_FRAME_COUNT;
        }
    }
}

bool HexStarFieldTryCollect(HexStarField *field, const HexGrid *grid, int beeEdge)
{
    if ((field == NULL) || (grid == NULL) || (beeEdge < 0)) return false;

    for (int i = 0; i < field->count; i++)
    {
        HexStar *s = &field->stars[i];
        if (s->collected) continue;
        if ((s->face < 0) || (s->face >= grid->faceCount)) continue;

        bool onFace = false;
        for (int c = 0; c < 6; c++)
        {
            if (grid->faces[s->face].edges[c] == beeEdge) { onFace = true; break; }
        }
        if (!onFace) continue;

        s->collected = true;
        field->powerTimer = HEX_STAR_POWER_TIME;
        return true;
    }
    return false;
}

void HexStarFieldDraw(const HexStarField *field, const HexGrid *grid)
{
    if ((field == NULL) || (grid == NULL) || (assets.star.id == 0)) return;

    float frameW = (float)assets.star.width;
    float frameH = (float)assets.star.height/(float)HEX_STAR_FRAME_COUNT;
    float drawW = frameW*HEX_STAR_SCALE;
    float drawH = frameH*HEX_STAR_SCALE;

    for (int i = 0; i < field->count; i++)
    {
        const HexStar *s = &field->stars[i];
        if (s->collected) continue;
        if ((s->face < 0) || (s->face >= grid->faceCount)) continue;

        Vector2 c = grid->faces[s->face].center;
        Rectangle src = { 0, frameH*(float)s->animFrame, frameW, frameH };
        Rectangle dst = { c.x - drawW*0.5f, c.y - drawH*0.5f, drawW, drawH };
        DrawTexturePro(assets.star, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
    }
}

bool HexStarFieldPowered(const HexStarField *field)
{
    return (field != NULL) && (field->powerTimer > 0.0f);
}
