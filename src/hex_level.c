/**********************************************************************************************
*
*   hexman - Level definitions + load/unload
*
**********************************************************************************************/

#include "hex_level.h"

#include <string.h>

//----------------------------------------------------------------------------------
// Authored levels (1-based in UI; 0-based here)
//----------------------------------------------------------------------------------
static const HexLevelDef LEVELS[HEX_LEVEL_IMPLEMENTED] = {
    // 1: radius 1, center seed, no wasps
    {
        .radius = 1,
        .beeStart = { -1, 0 },
        .seeds = { { 0, 0 } },
        .seedCount = 1,
        .enemies = { { 0 } },
        .enemyCount = 0,
        .hint = "The plants are suffering and need bee pollen to sprout. Encircle the seed to help!",
    },
    // 2: radius 2, opposite seeds, no wasps
    {
        .radius = 2,
        .beeStart = { -2, 0 },
        .seeds = { { -2, 1 }, { 2, -1 } },
        .seedCount = 2,
        .enemies = { { 0 } },
        .enemyCount = 0,
        .hint = "Every seed must sprout, and their living hexes must connect into one shape.",
    },
    // 3: radius 2, two seeds, introduce one wasp
    {
        .radius = 2,
        .beeStart = { -2, 0 },
        .seeds = { { -1, 2 }, { 1, -2 } },
        .seedCount = 2,
        .enemies = {
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_RIGHTMOST },
        },
        .enemyCount = 1,
        .hint = "Watch out! Wasps hunt along the edges. Don't let them touch you.",
    },
    // 4: radius 3, three seeds, three wasps
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = { { 2, -1 }, { -1, 2 }, { 0, -2 } },
        .seedCount = 3,
        .enemies = {
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_BOTTOMMOST },
        },
        .enemyCount = 3,
        .hint = "Three wasps, three seeds. Sprout them all and merge their gardens.",
    },
};

//----------------------------------------------------------------------------------
// Queries
//----------------------------------------------------------------------------------
int HexLevelCount(void)
{
    return HEX_LEVEL_IMPLEMENTED;
}

const HexLevelDef *HexLevelGetDef(int index)
{
    if (index < 0) index = 0;
    if (index >= HEX_LEVEL_IMPLEMENTED) index = 0;   // unimplemented slots -> tutorial
    return &LEVELS[index];
}

//----------------------------------------------------------------------------------
// Load
//----------------------------------------------------------------------------------
static int FaceLeftmostVertex(const HexGrid *grid, int face)
{
    if ((face < 0) || (face >= grid->faceCount)) return HexFindLeftmostVertex(grid);

    int best = grid->faces[face].vertices[0];
    for (int c = 1; c < 6; c++)
    {
        int v = grid->faces[face].vertices[c];
        if (grid->vertices[v].pos.x < grid->vertices[best].pos.x) best = v;
    }
    return best;
}

void HexLevelLoad(HexLevel *level, int index, Texture2D hexTexture, Texture2D flowerTexture, float beeSpeed)
{
    memset(level, 0, sizeof(*level));
    if (index < 0) index = 0;
    if (index >= HEX_LEVEL_SLOT_COUNT) index = HEX_LEVEL_SLOT_COUNT - 1;

    level->index = index;
    level->def = HexLevelGetDef(index);

    Vector2 origin = { (float)GetScreenWidth()*0.5f - 40.0f, (float)GetScreenHeight()*0.5f };
    HexGridInit(&level->grid, origin, hexTexture, level->def->radius);

    int beeFace = HexFindFace(&level->grid, level->def->beeStart.q, level->def->beeStart.r);
    int startVertex = FaceLeftmostVertex(&level->grid, beeFace);
    HexBeeInit(&level->bee, &level->grid, startVertex, beeSpeed);
    HexTrailInit(&level->trail, startVertex);

    int seedFaces[HEX_LEVEL_MAX_SEEDS] = { 0 };
    int seedCount = 0;
    for (int i = 0; i < level->def->seedCount; i++)
    {
        int face = HexFindFace(&level->grid, level->def->seeds[i].q, level->def->seeds[i].r);
        if (face >= 0) seedFaces[seedCount++] = face;
    }
    HexFlowerFieldInit(&level->flowers, flowerTexture, seedFaces, seedCount);

    level->enemyCount = level->def->enemyCount;
    if (level->enemyCount > HEX_LEVEL_MAX_ENEMIES) level->enemyCount = HEX_LEVEL_MAX_ENEMIES;
    for (int i = 0; i < level->enemyCount; i++)
    {
        int v = HexEnemySpawnVertex(&level->grid, level->def->enemies[i].spawn);
        HexEnemyInit(&level->enemies[i], level->def->enemies[i].type, &level->grid, v, beeSpeed);
    }
}

//----------------------------------------------------------------------------------
// Update / draw / win
//----------------------------------------------------------------------------------
int HexLevelUpdate(HexLevel *level, float dt)
{
    HexBeeUpdate(&level->bee, &level->grid, dt);

    int filledTotal = 0;
    for (int i = 0; i < level->bee.arrivalCount; i++)
    {
        int filled = HexTrailAdvance(&level->trail, &level->grid,
                                     level->bee.arrivalEdges[i], level->bee.arrivalVerts[i]);
        if (filled > 0)
        {
            filledTotal += filled;
            HexFlowerFieldOnFill(&level->flowers, &level->grid);
        }
    }

    Vector2 beePos = HexBeePosition(&level->bee, &level->grid);
    for (int i = 0; i < level->enemyCount; i++)
    {
        HexEnemyUpdate(&level->enemies[i], &level->grid, beePos, dt);
    }

    HexFlowerFieldUpdate(&level->flowers, dt);
    return filledTotal;
}

bool HexLevelBeeHit(const HexLevel *level, float hitRadius)
{
    Vector2 beePos = HexBeePosition(&level->bee, &level->grid);
    for (int i = 0; i < level->enemyCount; i++)
    {
        if (HexEnemyTouches(&level->enemies[i], &level->grid, beePos, hitRadius)) return true;
    }
    return false;
}

void HexLevelDraw(const HexLevel *level, Texture2D waspTexture)
{
    HexGridDraw(&level->grid);
    HexFlowerFieldDraw(&level->flowers, &level->grid);

    int toVertex = HexEdgeOtherVertex(&level->grid, level->bee.edge, level->bee.fromVertex);
    if (toVertex >= 0)
    {
        DrawCircleV(level->grid.vertices[toVertex].pos, 4.0f, (Color){ 255, 220, 80, 200 });

        int nextEdge = HexBeePeekNextEdge(&level->bee, &level->grid);
        if (nextEdge >= 0)
        {
            int nextVert = HexEdgeOtherVertex(&level->grid, nextEdge, toVertex);
            DrawLineEx(level->grid.vertices[toVertex].pos, level->grid.vertices[nextVert].pos, 3.0f, YELLOW);
        }
    }

    for (int i = 0; i < level->enemyCount; i++)
    {
        HexEnemyDraw(&level->enemies[i], &level->grid, waspTexture);
    }
}

bool HexLevelWon(const HexLevel *level)
{
    return HexFlowerFieldWon(&level->flowers, &level->grid);
}

void HexLevelDrawHint(const HexLevel *level)
{
    if ((level->def == NULL) || (level->def->hint == NULL) || (level->def->hint[0] == '\0')) return;

    const int pad = 12;
    const int fontSize = 18;
    const int boxW = 200;
    const int boxX = GetScreenWidth() - boxW - 16;
    const int boxY = 56;
    const int maxChars = 22;

    // Word-wrap into a fixed-width panel
    const char *text = level->def->hint;
    char line[64];
    int lineCount = 0;
    int y = boxY + pad;
    int i = 0;
    int len = (int)strlen(text);

    // Measure height first
    int tmpI = 0;
    while (tmpI < len)
    {
        while ((tmpI < len) && (text[tmpI] == ' ')) tmpI++;
        if (tmpI >= len) break;
        int start = tmpI;
        int lastSpace = -1;
        int col = 0;
        while ((tmpI < len) && (col < maxChars))
        {
            if (text[tmpI] == ' ') lastSpace = tmpI;
            tmpI++;
            col++;
        }
        if ((tmpI < len) && (lastSpace >= start)) tmpI = lastSpace + 1;
        lineCount++;
        (void)start;
    }

    int boxH = pad*2 + lineCount*(fontSize + 4);
    DrawRectangle(boxX, boxY, boxW, boxH, (Color){ 36, 42, 54, 220 });
    DrawRectangleLinesEx((Rectangle){ (float)boxX, (float)boxY, (float)boxW, (float)boxH }, 2.0f,
                         (Color){ 90, 100, 120, 255 });

    i = 0;
    while (i < len)
    {
        while ((i < len) && (text[i] == ' ')) i++;
        if (i >= len) break;

        int start = i;
        int lastSpace = -1;
        int col = 0;
        while ((i < len) && (col < maxChars))
        {
            if (text[i] == ' ') lastSpace = i;
            i++;
            col++;
        }
        int end = i;
        if ((i < len) && (lastSpace >= start))
        {
            end = lastSpace;
            i = lastSpace + 1;
        }

        int n = end - start;
        if (n >= (int)sizeof(line)) n = (int)sizeof(line) - 1;
        memcpy(line, text + start, (size_t)n);
        line[n] = '\0';
        DrawText(line, boxX + pad, y, fontSize, (Color){ 220, 225, 235, 255 });
        y += fontSize + 4;
    }
}
