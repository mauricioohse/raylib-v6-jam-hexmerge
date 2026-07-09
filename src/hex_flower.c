/**********************************************************************************************
*
*   Beehold - Flower / seed objectives implementation
*
**********************************************************************************************/

#include "hex_flower.h"

#include <math.h>
#include <string.h>

#define SPROUT_FRAME_TIME 0.22f
#define IDLE_FRAME_TIME 0.18f
#define IDLE_FRAME_FIRST 3
#define IDLE_FRAME_COUNT 3

static const Color TWIN_TINT = { 140, 220, 255, 255 };

int HexFindFace(const HexGrid *grid, int q, int r)
{
    for (int i = 0; i < grid->faceCount; i++)
    {
        if ((grid->faces[i].coord.q == q) && (grid->faces[i].coord.r == r)) return i;
    }
    return -1;
}

void HexFlowerFieldInit(HexFlowerField *field, Texture2D texture, Texture2D bubbleTexture,
                        const int *faces, const int *twinPairs, int faceCount)
{
    memset(field, 0, sizeof(*field));
    field->texture = texture;
    field->bubbleTexture = bubbleTexture;

    for (int i = 0; i < faceCount; i++)
    {
        int face = faces[i];
        if ((face < 0) || (field->count >= HEX_FLOWER_MAX)) continue;

        bool dup = false;
        for (int j = 0; j < field->count; j++)
        {
            if (field->flowers[j].face == face) { dup = true; break; }
        }
        if (dup) continue;

        HexFlower *f = &field->flowers[field->count++];
        f->face = face;
        f->twinPair = (twinPairs != NULL)? twinPairs[i] : -1;
        f->state = HEX_FLOWER_SEED;
        f->animTimer = 0.0f;
        f->animFrame = 0;
        f->failShake = 0.0f;
    }
}

void HexFlowerFieldOnFill(HexFlowerField *field, const HexGrid *grid)
{
    for (int i = 0; i < field->count; i++)
    {
        HexFlower *f = &field->flowers[i];
        if (f->state != HEX_FLOWER_SEED) continue;
        if ((f->face < 0) || (f->face >= grid->faceCount)) continue;
        if (!grid->faces[f->face].filled) continue;

        f->state = HEX_FLOWER_SPROUTING;
        f->animFrame = 1;
        f->animTimer = 0.0f;
    }
}

void HexFlowerFieldOnTwinFail(HexFlowerField *field, const bool enclosed[HEX_MAX_FACES])
{
    if (field == NULL) return;

    for (int i = 0; i < field->count; i++)
    {
        int pair = field->flowers[i].twinPair;
        if (pair < 0) continue;

        bool first = true;
        for (int j = 0; j < i; j++)
        {
            if (field->flowers[j].twinPair == pair) { first = false; break; }
        }
        if (!first) continue;

        int inEnclosed = 0;
        for (int j = 0; j < field->count; j++)
        {
            if (field->flowers[j].twinPair != pair) continue;
            int face = field->flowers[j].face;
            if ((face >= 0) && enclosed[face]) inEnclosed++;
        }
        if (inEnclosed != 1) continue;

        // Shake both twins of the broken pair
        for (int j = 0; j < field->count; j++)
        {
            if (field->flowers[j].twinPair == pair) field->flowers[j].failShake = 0.55f;
        }
    }
}

void HexFlowerFieldUpdate(HexFlowerField *field, float dt)
{
    field->bubbleTime += dt;

    for (int i = 0; i < field->count; i++)
    {
        HexFlower *f = &field->flowers[i];
        if (f->failShake > 0.0f)
        {
            f->failShake -= dt;
            if (f->failShake < 0.0f) f->failShake = 0.0f;
        }

        if (f->state == HEX_FLOWER_SEED) continue;

        f->animTimer += dt;

        if (f->state == HEX_FLOWER_SPROUTING)
        {
            while ((f->animTimer >= SPROUT_FRAME_TIME) && (f->state == HEX_FLOWER_SPROUTING))
            {
                f->animTimer -= SPROUT_FRAME_TIME;
                f->animFrame++;
                if (f->animFrame >= IDLE_FRAME_FIRST)
                {
                    f->state = HEX_FLOWER_BLOOMED;
                    f->animFrame = IDLE_FRAME_FIRST;
                    f->animTimer = 0.0f;
                }
            }
        }
        else if (f->state == HEX_FLOWER_BLOOMED)
        {
            while (f->animTimer >= IDLE_FRAME_TIME)
            {
                f->animTimer -= IDLE_FRAME_TIME;
                int idle = f->animFrame - IDLE_FRAME_FIRST;
                idle = (idle + 1)%IDLE_FRAME_COUNT;
                f->animFrame = IDLE_FRAME_FIRST + idle;
            }
        }
    }
}

bool HexFlowerTwinsAllowFill(const HexFlowerField *field, const HexGrid *grid, const bool enclosed[HEX_MAX_FACES])
{
    (void)grid;
    if (field == NULL) return true;

    for (int i = 0; i < field->count; i++)
    {
        int pair = field->flowers[i].twinPair;
        if (pair < 0) continue;

        // Only evaluate each pair once (from the first flower that owns it)
        bool first = true;
        for (int j = 0; j < i; j++)
        {
            if (field->flowers[j].twinPair == pair) { first = false; break; }
        }
        if (!first) continue;

        int inEnclosed = 0;
        for (int j = 0; j < field->count; j++)
        {
            if (field->flowers[j].twinPair != pair) continue;
            int face = field->flowers[j].face;
            if ((face >= 0) && enclosed[face]) inEnclosed++;
        }

        if (inEnclosed == 1) return false;   // one twin without its mate
    }

    return true;
}

//----------------------------------------------------------------------------------
// Twin bubble link (straight line between seed centers)
//----------------------------------------------------------------------------------
static void DrawBubbleTrail(const HexFlowerField *field, const HexGrid *grid, int faceA, int faceB)
{
    if (field->bubbleTexture.id == 0) return;
    if ((faceA < 0) || (faceB < 0) || (faceA >= grid->faceCount) || (faceB >= grid->faceCount)) return;

    Vector2 a = grid->faces[faceA].center;
    Vector2 b = grid->faces[faceB].center;
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 1.0f) return;

    float frameW = (float)field->bubbleTexture.width;
    float frameH = (float)field->bubbleTexture.height/(float)HEX_BUBBLE_FRAME_COUNT;
    float t = field->bubbleTime;

    int steps = (int)(len/14.0f);
    if (steps < 2) steps = 2;

    for (int s = 0; s <= steps; s++)
    {
        float u = (float)s/(float)steps;
        // Keep bubbles off the seed sprites at the ends
        if ((u < 0.12f) || (u > 0.88f)) continue;

        float px = a.x + dx*u;
        float py = a.y + dy*u;
        float phase = t*2.4f + (float)s*0.7f;
        py += sinf(phase)*3.0f;
        px += cosf(phase*0.7f)*1.5f;

        int frame = ((int)(phase*1.5f) + s)%HEX_BUBBLE_FRAME_COUNT;
        if (frame < 0) frame = 0;
        float scale = 1.2f + 0.25f*sinf(phase + 1.0f);
        float drawW = frameW*scale;
        float drawH = frameH*scale;

        Rectangle src = { 0, frameH*(float)frame, frameW, frameH };
        Rectangle dst = { px - drawW*0.5f, py - drawH*0.5f, drawW, drawH };
        DrawTexturePro(field->bubbleTexture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
    }
}

void HexFlowerFieldDraw(const HexFlowerField *field, const HexGrid *grid)
{
    // Bubble bonds under the seeds
    for (int i = 0; i < field->count; i++)
    {
        int pair = field->flowers[i].twinPair;
        if (pair < 0) continue;
        bool first = true;
        for (int j = 0; j < i; j++)
        {
            if (field->flowers[j].twinPair == pair) { first = false; break; }
        }
        if (!first) continue;

        int faceA = field->flowers[i].face;
        int faceB = -1;
        for (int j = i + 1; j < field->count; j++)
        {
            if (field->flowers[j].twinPair == pair) { faceB = field->flowers[j].face; break; }
        }
        if (faceB >= 0) DrawBubbleTrail(field, grid, faceA, faceB);
    }

    if (field->texture.id == 0) return;

    float frameW = (float)field->texture.width;
    float frameH = (float)field->texture.height/(float)HEX_FLOWER_FRAME_COUNT;
    float drawW = frameW*HEX_FLOWER_SCALE;
    float drawH = frameH*HEX_FLOWER_SCALE;

    for (int i = 0; i < field->count; i++)
    {
        const HexFlower *f = &field->flowers[i];
        if ((f->face < 0) || (f->face >= grid->faceCount)) continue;

        Vector2 c = grid->faces[f->face].center;
        if (f->failShake > 0.0f)
        {
            float shake = f->failShake*10.0f;
            c.x += sinf(field->bubbleTime*55.0f + (float)i*2.1f)*shake;
            c.y += cosf(field->bubbleTime*48.0f + (float)i*1.7f)*shake*0.7f;
        }

        Rectangle src = { 0, frameH*(float)f->animFrame, frameW, frameH };
        Rectangle dst = { c.x - drawW*0.5f, c.y - drawH*0.5f, drawW, drawH };
        Color tint = (f->twinPair >= 0)? TWIN_TINT : WHITE;
        if (f->failShake > 0.0f)
        {
            // Flash red while shaking
            float u = f->failShake/0.55f;
            tint = (Color){
                (unsigned char)(255),
                (unsigned char)(60 + (int)((1.0f - u)*40)),
                (unsigned char)(60 + (int)((1.0f - u)*40)),
                255
            };
        }
        DrawTexturePro(field->texture, src, dst, (Vector2){ 0, 0 }, 0.0f, tint);
    }
}

bool HexFlowerFieldWon(const HexFlowerField *field, const HexGrid *grid)
{
    if (field->count <= 0) return false;

    for (int i = 0; i < field->count; i++)
    {
        if (field->flowers[i].state != HEX_FLOWER_BLOOMED) return false;
        int face = field->flowers[i].face;
        if ((face < 0) || (face >= grid->faceCount) || !grid->faces[face].filled) return false;
    }

    // BFS over filled faces from the first flower; every flower face must be reached
    bool visited[HEX_MAX_FACES] = { false };
    int queue[HEX_MAX_FACES] = { 0 };
    int head = 0;
    int tail = 0;

    int start = field->flowers[0].face;
    visited[start] = true;
    queue[tail++] = start;

    while (head < tail)
    {
        int f = queue[head++];
        for (int c = 0; c < 6; c++)
        {
            int e = grid->faces[f].edges[c];
            if (e < 0) continue;

            const HexEdge *edge = &grid->edges[e];
            if (edge->faceCount < 2) continue;

            int other = (edge->faces[0] == f)? edge->faces[1] : edge->faces[0];
            if ((other < 0) || visited[other]) continue;
            if (!grid->faces[other].filled) continue;

            visited[other] = true;
            queue[tail++] = other;
        }
    }

    for (int i = 0; i < field->count; i++)
    {
        if (!visited[field->flowers[i].face]) return false;
    }

    return true;
}
