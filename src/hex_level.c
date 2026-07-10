/**********************************************************************************************
*
*   Beehold - Level definitions + load/unload
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
        .seeds = { { { 0, 0 }, -1 } },
        .seedCount = 1,
        .starCount = 0,
        .enemies = { { 0 } },
        .enemyCount = 0,
        .hint = "The plants are suffering and need bee pollen to sprout. Encircle the seed to help!",
    },
    // 2: radius 2, opposite seeds, no wasps
    {
        .radius = 2,
        .beeStart = { -2, 0 },
        .seeds = { { { -2, 1 }, -1 }, { { 2, -1 }, -1 } },
        .seedCount = 2,
        .starCount = 0,
        .enemies = { { 0 } },
        .enemyCount = 0,
        .hint = "Every seed must sprout, and their living hexes must connect into one shape.",
    },
    // 3: radius 2, two seeds, introduce one wasp
    {
        .radius = 2,
        .beeStart = { -2, 0 },
        .seeds = { { { -1, 2 }, -1 }, { { 1, -2 }, -1 } },
        .seedCount = 2,
        .starCount = 0,
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
        .seeds = { { { 2, -1 }, -1 }, { { -1, 2 }, -1 }, { { 0, -2 }, -1 } },
        .seedCount = 3,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_BOTTOMMOST },
        },
        .enemyCount = 3,
        .hint = NULL,
    },
    // 5: radius 3, black rim patrols + interior red/purple
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = { { { 2, -1 }, -1 }, { { -1, 2 }, -1 }, { { 0, -2 }, -1 } },
        .seedCount = 3,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_BOTTOMMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_INNER },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_RIGHTMOST },
        },
        .enemyCount = 4,
        .hint = NULL,
    },
    // 6: Tight squeeze
    {
        .radius = 2,
        .beeStart = { -2, 0 },
        .seeds = { { { -2, 1 }, -1 }, { { 2, -1 }, -1 } },
        .seedCount = 2,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_BOTTOMMOST },
        },
        .enemyCount = 3,
        .hint = NULL,
    },
    // 7: Twin seeds intro — no wasps
    {
        .radius = 2,
        .beeStart = { -2, 0 },
        .seeds = { { { -2, 1 }, 0 }, { { 2, -1 }, 0 } },
        .seedCount = 2,
        .starCount = 0,
        .enemies = { { 0 } },
        .enemyCount = 0,
        .hint = "Twin seeds share a bond! Encircle both together, painting only one fails.",
    },
    // 8: two twin pairs + black / chaser / random
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = {
            { { -2, 1 }, 0 }, { { 2, -1 }, 0 },
            { { 0, -2 }, 1 }, { { 0, 2 }, 1 },
        },
        .seedCount = 4,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_BOTTOMMOST },
        },
        .enemyCount = 3,
        .hint = NULL,
    },
    // 9: Full swarm (was old 9)
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = { { { 0, -2 }, -1 }, { { 2, -1 }, -1 }, { { 0, 2 }, -1 }, { { -2, 1 }, -1 } },
        .seedCount = 4,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_INNER },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_BOTTOMMOST },
        },
        .enemyCount = 4,
        .hint = NULL,
    },
    // 10: Star seed tutorial — radius 3, one star + seeds + wasps
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = { { { 2, -1 }, -1 }, { { -1, 2 }, -1 }, { { 0, -2 }, -1 } },
        .seedCount = 3,
        .stars = { { 1, -2 } },
        .starCount = 1,
        .enemies = {
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_BOTTOMMOST },
        },
        .enemyCount = 3,
        .hint = "Grab the star seed! For a few seconds you can eat wasps — they flee to the grey prison hex.",
    },
    // 11: two stars + normal seeds + 5 wasps (2 black, 1 chase, 1 random, 1 mixed)
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = { { { 2, -1 }, -1 }, { { -2, 1 }, -1 }, { { 0, 2 }, -1 }, { { 0, -2 }, -1 } },
        .seedCount = 4,
        .stars = { { 2, 0 }, { -2, 0 } },
        .starCount = 2,
        .enemies = {
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_BOTTOMMOST },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_INNER },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_INNER },
        },
        .enemyCount = 5,
        .hint = NULL,
    },
    // 12: radius 4 (was old 10)
    {
        .radius = 4,
        .beeStart = { -4, 0 },
        .seeds = {
            { { -3, 2 }, -1 }, { { 3, -2 }, -1 }, { { 0, -3 }, -1 },
            { { 2, 1 }, -1 }, { { -2, -1 }, -1 },
        },
        .seedCount = 5,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_BOTTOMMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_INNER },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_INNER },
        },
        .enemyCount = 5,
        .hint = NULL,
    },
    // 13: radius 4, twelve seeds + one twin pair + two stars (was old 11)
    {
        .radius = 4,
        .beeStart = { -4, 0 },
        .seeds = {
            { { -4, 0 }, -1 }, { { -4, 2 }, -1 }, { { -2, 4 }, -1 }, { { 0, 4 }, -1 },
            { { 2, 2 }, -1 }, { { 4, 0 }, -1 }, { { 4, -2 }, -1 }, { { 2, -4 }, -1 },
            { { 0, -4 }, -1 }, { { -2, -2 }, -1 },
            { { -3, 1 }, 0 }, { { 3, -1 }, 0 },
        },
        .seedCount = 12,
        .stars = { { 2, -2 }, { -2, 2 } },
        .starCount = 2,
        .enemies = {
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_BOTTOMMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_INNER },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_INNER },
        },
        .enemyCount = 5,
        .hint = NULL,
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
    if (index >= HEX_LEVEL_IMPLEMENTED) index = 0;
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

void HexLevelLoad(HexLevel *level, int index, Texture2D hexTexture, Texture2D flowerTexture,
                  Texture2D bubbleTexture, Texture2D starTexture, float beeSpeed)
{
    memset(level, 0, sizeof(*level));
    if (index < 0) index = 0;
    if (index >= HEX_LEVEL_SLOT_COUNT) index = HEX_LEVEL_SLOT_COUNT - 1;

    level->index = index;
    level->def = HexLevelGetDef(index);
    level->jailFace = -1;

    Vector2 origin = { (float)GetScreenWidth()*0.5f - 40.0f, (float)GetScreenHeight()*0.5f };
    HexGridInit(&level->grid, origin, hexTexture, level->def->radius);

    int beeFace = HexFindFace(&level->grid, level->def->beeStart.q, level->def->beeStart.r);
    int startVertex = FaceLeftmostVertex(&level->grid, beeFace);
    HexBeeInit(&level->bee, &level->grid, startVertex, beeSpeed);
    HexTrailInit(&level->trail, startVertex);

    int seedFaces[HEX_LEVEL_MAX_SEEDS] = { 0 };
    int twinPairs[HEX_LEVEL_MAX_SEEDS] = { 0 };
    int seedCount = 0;
    for (int i = 0; i < level->def->seedCount; i++)
    {
        int face = HexFindFace(&level->grid, level->def->seeds[i].coord.q, level->def->seeds[i].coord.r);
        if (face < 0) continue;
        seedFaces[seedCount] = face;
        twinPairs[seedCount] = level->def->seeds[i].twinPair;
        seedCount++;
    }
    HexFlowerFieldInit(&level->flowers, flowerTexture, bubbleTexture, seedFaces, twinPairs, seedCount);

    int starFaces[HEX_LEVEL_MAX_STARS] = { 0 };
    int starCount = 0;
    for (int i = 0; i < level->def->starCount; i++)
    {
        int face = HexFindFace(&level->grid, level->def->stars[i].q, level->def->stars[i].r);
        if (face < 0) continue;
        starFaces[starCount++] = face;
    }
    HexStarFieldInit(&level->stars, starTexture, starFaces, starCount);

    if (starCount > 0)
    {
        level->jailFace = HexFindFace(&level->grid, 0, 0);
        if (level->jailFace >= 0) level->grid.faces[level->jailFace].starJail = true;
    }

    level->enemyCount = level->def->enemyCount;
    if (level->enemyCount > HEX_LEVEL_MAX_ENEMIES) level->enemyCount = HEX_LEVEL_MAX_ENEMIES;

    int avoid[HEX_LEVEL_MAX_ENEMIES + 1];
    int avoidCount = 0;
    avoid[avoidCount++] = startVertex;     // never spawn on the bee

    for (int i = 0; i < level->enemyCount; i++)
    {
        int v = HexEnemySpawnVertexAvoid(&level->grid, level->def->enemies[i].spawn, avoid, avoidCount);
        avoid[avoidCount++] = v;
        HexEnemyInit(&level->enemies[i], level->def->enemies[i].type, &level->grid, v, beeSpeed);
    }
}

//----------------------------------------------------------------------------------
// Update / draw / win
//----------------------------------------------------------------------------------
int HexLevelUpdate(HexLevel *level, float dt)
{
    HexBeeUpdate(&level->bee, &level->grid, dt);

    int result = 0;
    for (int i = 0; i < level->bee.arrivalCount; i++)
    {
        int filled = HexTrailAdvance(&level->trail, &level->grid, &level->flowers,
                                     level->bee.arrivalEdges[i], level->bee.arrivalVerts[i]);
        if (filled == HEX_TRAIL_TWIN_FAIL) result = HEX_TRAIL_TWIN_FAIL;
        else if ((filled > 0) && (result != HEX_TRAIL_TWIN_FAIL))
        {
            result += filled;
            HexFlowerFieldOnFill(&level->flowers, &level->grid);
        }
    }

    HexStarFieldTryCollect(&level->stars, &level->grid, level->bee.edge);
    HexStarFieldUpdate(&level->stars, dt);

    bool powered = HexStarFieldPowered(&level->stars);
    Vector2 beePos = HexBeePosition(&level->bee, &level->grid);

    for (int i = 0; i < level->enemyCount; i++)
    {
        HexEnemy *enemy = &level->enemies[i];
        HexEnemyUpdate(enemy, &level->grid, beePos, dt, powered, level->jailFace);

        if (powered && !HexEnemyIsJailed(enemy) &&
            HexEnemyTouches(enemy, &level->grid, beePos, 14.0f))
        {
            HexEnemySendToJail(enemy, &level->grid, level->jailFace);
        }
    }

    HexFlowerFieldUpdate(&level->flowers, dt);
    return result;
}

bool HexLevelBeeHit(const HexLevel *level, float hitRadius)
{
    if (HexStarFieldPowered(&level->stars)) return false;

    Vector2 beePos = HexBeePosition(&level->bee, &level->grid);
    for (int i = 0; i < level->enemyCount; i++)
    {
        if (HexEnemyTouches(&level->enemies[i], &level->grid, beePos, hitRadius)) return true;
    }
    return false;
}

bool HexLevelStarPowered(const HexLevel *level)
{
    return HexStarFieldPowered(&level->stars);
}

void HexLevelDraw(const HexLevel *level, Texture2D waspTexture)
{
    HexGridDraw(&level->grid);
    HexBeeDrawLiveTrail(&level->bee, &level->grid);
    HexFlowerFieldDraw(&level->flowers, &level->grid);
    HexStarFieldDraw(&level->stars, &level->grid);

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

    bool powered = HexStarFieldPowered(&level->stars);
    for (int i = 0; i < level->enemyCount; i++)
    {
        HexEnemyDraw(&level->enemies[i], &level->grid, waspTexture, powered);
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

    const char *text = level->def->hint;
    char line[64];
    int lineCount = 0;
    int y = boxY + pad;
    int i = 0;
    int len = (int)strlen(text);

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
