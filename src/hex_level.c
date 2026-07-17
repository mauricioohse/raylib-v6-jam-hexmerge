/**********************************************************************************************
*
*   Beehold - Level definitions + load/unload
*
*   Level summary (1-based):
*     1  — radius 1, teaches one seed — center seed, no wasps
*     2  — radius 3, teaches seeds must merge — opposite seeds, no wasps (checkpoint)
*     3  — radius 2, teaches red wasp — two seeds, two reds
*     4  — radius 2, teaches purple chaser — three seeds, two purple
*     5  — radius 2, teaches green wasp — three seeds, three green
*     6  — radius 3, combines red/purple/green — six seeds (checkpoint)
*     7  — radius 3, outer black patrols + interior red/purple
*     8  — radius 2, tight squeeze: black / purple / green
*     9  — radius 2, twin seeds intro, no wasps (checkpoint)
*    10  — radius 3, two twin pairs + black / chaser / random
*    11  — radius 3, full swarm: one of each wasp color, 4 seeds
*    12  — radius 3, black ring intro: one black per ring R=1,2,3, 2 seeds (checkpoint)
*    13  — radius 3, chaser + random + black on each ring, twin pair + 3 seeds
*    14  — radius 2, seedAll paint-the-board intro, one red wasp
*    15  — radius 1, star seed tutorial, three reds (checkpoint)
*    16  — radius 3, two stars + seeds + mixed wasps
*    17  — radius 3, three stars + normal seeds + 5 wasps
*    18  — radius 4, twelve seeds + twin pair + two stars
*    19  — radius 1, fire + water tutorial (checkpoint)
*    20  — radius 2, twin pair + seed + fire/water, black + chaser
*    21  — radius 3, fire without water + 4 seeds + star
*    22  — radius 3, seedAll paint-the-board, mixed swarm
*    23  — radius 3, seedAll + stars + fire/water, 6 enemies
*    24  — radius 3, six seeds + twin pair + two fire — mixed wasp finale
*
**********************************************************************************************/

#include "hex_level.h"
#include "hex_enemy.h"
#include "screens.h"

#include <string.h>

//----------------------------------------------------------------------------------
// Authored levels (1-based in UI; 0-based here)
//----------------------------------------------------------------------------------
static const HexLevelDef LEVELS[HEX_LEVEL_IMPLEMENTED] = {
    // 1: teaches one seed — center seed, no wasps
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
    // 2: teaches seeds must merge — opposite seeds, no wasps
    {
        .radius = 3,
        .beeStart = { -2, 0 },
        .seeds = { { { -2, 1 }, -1 }, { { 2, -1 }, -1 } },
        .seedCount = 2,
        .starCount = 0,
        .enemies = { { 0 } },
        .enemyCount = 0,
        .hint = "Every seed must sprout, and their living hexes must connect into one shape.",
        .checkpoint = true,
    },
    // 3: teaches red wasp — two seeds, two reds
    {
        .radius = 2,
        .beeStart = { -2, 0 },
        .seeds = { { { -1, 2 }, -1 }, { { 1, -2 }, -1 } },
        .seedCount = 2,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_BOTTOMMOST },
        },
        .enemyCount = 2,
        .hint = "Watch out! Red wasps walk randomly, but fast!.",
    },
    // 4: teaches purple chaser — three seeds, two purple
    {
        .radius = 2,
        .beeStart = { -3, 0 },
        .seeds = { { { 2, -1 }, -1 }, { { -1, 2 }, -1 }, { { 0, -2 }, -1 } },
        .seedCount = 3,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_BOTTOMMOST },
        },
        .enemyCount = 2,
        .hint = "Purple wasps chase you down!",
    },
    // 5: teaches green wasp — three seeds, three green
    {
        .radius = 2,
        .beeStart = { -3, 0 },
        .seeds = { { { 2, -1 }, -1 }, { { -1, 2 }, -1 }, { { 0, -2 }, -1 } },
        .seedCount = 3,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_BOTTOMMOST },
        },
        .enemyCount = 3,
        .hint = "Green ones sometimes chase, some times wander",
    },
    // 6: combines red/purple/green — six seeds
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = {
            { { 2, -1 }, -1 },
            { { -1, 2 }, -1 },
            { { 0, -2 }, -1 },
            { { 1, 1 }, -1 },
            { { -2, 1 }, -1 },
            { { 0, 2 }, -1 },
        },
        .seedCount = 6,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_LEFTMOST },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_BOTTOMMOST },
        },
        .enemyCount = 4,
        .hint = NULL,
        .checkpoint = true,
    },
    // 7: black ring patrols (R=3 = outer) + interior red/purple
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = { { { 2, -1 }, -1 }, { { -1, 2 }, -1 }, { { 0, -2 }, -1 } },
        .seedCount = 3,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_TOPMOST, 3 },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_BOTTOMMOST, 3 },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_INNER },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_RIGHTMOST },
        },
        .enemyCount = 4,
        .hint = NULL,
    },
    // 8: tight squeeze — black / purple / green
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
    // 9: twin seeds intro — no wasps
    {
        .radius = 2,
        .beeStart = { -2, 0 },
        .seeds = { { { -2, 1 }, 0 }, { { 2, -1 }, 0 } },
        .seedCount = 2,
        .starCount = 0,
        .enemies = { { 0 } },
        .enemyCount = 0,
        .hint = "Twin seeds share a bond! Encircle both together, painting only one fails.",
        .checkpoint = true,
    },
    // 10: two twin pairs + black / chaser / random
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
    // 11: full swarm — one of each wasp color, 4 seeds
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
    // 12: black ring intro — one black per ring (R=1,2,3), two seeds
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = { { { -2, 1 }, -1 }, { { 2, -1 }, -1 } },
        .seedCount = 2,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_INNER, 1 },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_RIGHTMOST, 2 },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_TOPMOST, 3 },
        },
        .enemyCount = 3,
        .hint = "Black wasps each patrol their own ring.",
        .checkpoint = true,
    },
    // 13: mixed rings — chaser + random + one black per ring, twin pair + 3 seeds
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = {
            { { 0, -2 }, -1 }, { { 2, -1 }, -1 }, { { -2, 1 }, -1 },
            { { 1, 1 }, 0 }, { { -1, -1 }, 0 },
        },
        .seedCount = 5,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_INNER },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_LEFTMOST, 1 },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_BOTTOMMOST, 2 },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_TOPMOST, 3 },
        },
        .enemyCount = 5,
        .hint = NULL,
    },
    // 14: seedAll intro — paint every hex
    {
        .radius = 2,
        .beeStart = { -2, 0 },
        .seedCount = 0,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_RIGHTMOST },
        },
        .enemyCount = 1,
        .hint = "Every hex has a seed! Paint the entire board to finish.",
        .seedAll = true,
    },
    // 15: star seed tutorial — radius 1, one seed, two stars, five wanderers
    {
        .radius = 1,
        .beeStart = { -1, 0 },
        .seeds = { { { 1, 0 }, -1 } },
        .seedCount = 1,
        .stars = { { 1, -1 }, { -1, 1 } },
        .starCount = 2,
        .enemies = {
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_BOTTOMMOST },
        },
        .enemyCount = 3,
        .hint = "Grab a star seed! For a few seconds you can eat wasps — they flee to the grey prison hex.",
        .checkpoint = true,
    },
    // 16: one star + seeds + mixed wasps
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = { { { 2, -1 }, -1 }, { { -1, 2 }, -1 }, { { 0, -2 }, -1 } },
        .seedCount = 3,
        .stars = { { 1, -2 }, { -2, 1 } }, 
 
        .starCount = 1,
        .enemies = {
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_BOTTOMMOST },
        },
        .enemyCount = 3,
        .hint = NULL,
    },
    // 17: three stars + normal seeds + 5 wasps
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = { { { 2, -1 }, -1 }, { { -2, 1 }, -1 }, { { 0, 2 }, -1 }, { { 0, -2 }, -1 } },
        .seedCount = 4,
        .stars = { { 2, 0 }, { -2, 0 }},
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
    // 18: radius 4 — twelve seeds + twin pair + two stars
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
    // 19: fire + water tutorial — radius 1
    {
        .radius = 1,
        .beeStart = { -1, 0 },
        .seeds = { { { 0, 0 }, -1 } },
        .seedCount = 1,
        .starCount = 0,
        .specials = {
            { { 1, -1 }, HEX_FACE_FIRE },
            { { -1, 1 }, HEX_FACE_WATER },
        },
        .specialCount = 2,
        .enemies = { { 0 } },
        .enemyCount = 0,
        .hint = "Red fire hexes kill if painted alone! Encircle fire with a water hex to put it out.",
        .checkpoint = true,
    },
    // 20: twin pair + one seed + fire and water — black edge + chaser
    {
        .radius = 2,
        .beeStart = { -2, 0 },
        .seeds = { { { -2, 1 }, 0 }, { { 2, -1 }, 0 }, { { 0, -2 }, -1 } },
        .seedCount = 3,
        .starCount = 0,
        .specials = {
            { { 1, 0 }, HEX_FACE_FIRE },
            { { -1, 0 }, HEX_FACE_WATER },
        },
        .specialCount = 2,
        .enemies = {
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_RIGHTMOST },
        },
        .enemyCount = 2,
        .hint = NULL,
    },
    // 21: fire without water + 4 seeds + star — radius 3
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = {
            { { 2, -1 }, -1 }, { { -1, 2 }, -1 }, { { 0, -2 }, -1 }, { { -2, 1 }, -1 },
        },
        .seedCount = 4,
        .stars = { { 2, 0 } },
        .starCount = 1,
        .specials = {
            { { 0, 1 }, HEX_FACE_FIRE },
        },
        .specialCount = 1,
        .enemies = {
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_BOTTOMMOST },
        },
        .enemyCount = 2,
        .hint = NULL,
    },
    // 22: seedAll — paint every hex under a mixed swarm
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seedCount = 0,
        .starCount = 0,
        .enemies = {
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_TOPMOST, 3 },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_BOTTOMMOST, 2 },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_INNER },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_LEFTMOST },
        },
        .enemyCount = 5,
        .hint = NULL,
        .seedAll = true,
    },
    // 23: seedAll — paint every hex; 2 stars, 2 fire, 1 water, 6 enemies (no chaser)
    {
        .radius = 3,
        .beeStart = { -4, 0 },
        .seedCount = 0,
        .stars = { { 2, -2 }, { -2, 2 } },
        .starCount = 2,
        .specials = {
            { { 1, 1 }, HEX_FACE_FIRE },
            { { -1, -1 }, HEX_FACE_FIRE },
            { { 3, 0 }, HEX_FACE_WATER },
        },
        .specialCount = 3,
        .enemies = {
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_BOTTOMMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_INNER },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_INNER },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_LEFTMOST },
        },
        .enemyCount = 6,
        .hint = NULL,
        .seedAll = true,
    },
    // 24: six seeds + twin pair + two fire — 2 red / 1 purple / 2 green + black on R=3 and R=1
    {
        .radius = 3,
        .beeStart = { -3, 0 },
        .seeds = {
            { { 2, -1 }, -1 }, { { -1, 2 }, -1 }, { { 0, -2 }, -1 },
            { { 1, 1 }, -1 }, { { -2, 1 }, -1 }, { { 0, 2 }, -1 },
            { { 3, -3 }, 0 }, { { -3, 3 }, 0 },
        },
        .seedCount = 8,
        .starCount = 0,
        .specials = {
            { { 2, 0 }, HEX_FACE_FIRE },
            { { -2, 0 }, HEX_FACE_FIRE },
        },
        .specialCount = 2,
        .enemies = {
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_RIGHTMOST },
            { HEX_ENEMY_RED_RANDOM, HEX_SPAWN_INNER },
            { HEX_ENEMY_PURPLE_CHASER, HEX_SPAWN_TOPMOST },
            { HEX_ENEMY_GREEN_MIXED, HEX_SPAWN_BOTTOMMOST },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_TOPMOST, 3 },
            { HEX_ENEMY_BLACK_EDGE, HEX_SPAWN_INNER, 1 },
        },
        .enemyCount = 6,
        .hint = "Final challenge!",
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

void HexLevelLoad(HexLevel *level, int index, float beeSpeed)
{
    memset(level, 0, sizeof(*level));
    if (index < 0) index = 0;
    if (index >= HEX_LEVEL_SLOT_COUNT) index = HEX_LEVEL_SLOT_COUNT - 1;

    level->index = index;
    level->def = HexLevelGetDef(index);
    level->jailFace = -1;

    Vector2 origin = { (float)GetScreenWidth()*0.5f, (float)GetScreenHeight()*0.5f };
    HexGridInit(&level->grid, origin, level->def->radius);

    int beeFace = HexFindFace(&level->grid, level->def->beeStart.q, level->def->beeStart.r);
    int startVertex = FaceLeftmostVertex(&level->grid, beeFace);
    HexBeeInit(&level->bee, &level->grid, startVertex, beeSpeed);
    HexTrailInit(&level->trail, startVertex);

    // Specials first so seedAll can skip fire/water faces
    for (int i = 0; i < level->def->specialCount; i++)
    {
        int face = HexFindFace(&level->grid, level->def->specials[i].coord.q, level->def->specials[i].coord.r);
        if (face < 0) continue; // skipps silently if the def is done wrong...
        level->grid.faces[face].kind = level->def->specials[i].kind;
        level->grid.faces[face].fireAnimFrame = GetRandomValue(0, 3);
    }

    int seedFaces[HEX_MAX_FACES] = { 0 };
    int twinPairs[HEX_MAX_FACES] = { 0 };
    int seedCount = 0;
    if (level->def->seedAll)
    {
        for (int i = 0; i < level->grid.faceCount; i++)
        {
            if (level->grid.faces[i].kind != HEX_FACE_NORMAL) continue;
            if (seedCount >= HEX_FLOWER_MAX) break;
            seedFaces[seedCount] = i;
            twinPairs[seedCount] = -1;
            seedCount++;
        }
    }
    else
    {
        for (int i = 0; i < level->def->seedCount; i++)
        {
            int face = HexFindFace(&level->grid, level->def->seeds[i].coord.q, level->def->seeds[i].coord.r);
            if (face < 0) continue;
            if (seedCount >= HEX_FLOWER_MAX) break;
            seedFaces[seedCount] = face;
            twinPairs[seedCount] = level->def->seeds[i].twinPair;
            seedCount++;
        }
    }
    HexFlowerFieldInit(&level->flowers, seedFaces, twinPairs, seedCount);

    int starFaces[HEX_LEVEL_MAX_STARS] = { 0 };
    int starCount = 0;
    for (int i = 0; i < level->def->starCount; i++)
    {
        int face = HexFindFace(&level->grid, level->def->stars[i].q, level->def->stars[i].r);
        if (face < 0) continue;
        starFaces[starCount++] = face;
    }
    HexStarFieldInit(&level->stars, starFaces, starCount);

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
        const HexLevelEnemyDef *ed = &level->def->enemies[i];
        int ring = (ed->type == HEX_ENEMY_BLACK_EDGE)? ed->ring : 0;
        int v = HexEnemySpawnVertexAvoid(&level->grid, ed->spawn, avoid, avoidCount, ring);
        avoid[avoidCount++] = v;
        HexEnemyInit(&level->enemies[i], ed->type, &level->grid, v, beeSpeed, ring);
    }
}

void HexLevelRespawnKeepProgress(HexLevel *level, float beeSpeed)
{
    if ((level == NULL) || (level->def == NULL)) return;

    // Keep filled faces / flower progress / collected stars; clear wet pollen trail
    for (int e = 0; e < level->grid.edgeCount; e++)
        level->grid.edges[e].painted = false;

    for (int f = 0; f < level->grid.faceCount; f++)
        level->grid.faces[f].failShake = 0.0f;

    for (int i = 0; i < level->flowers.count; i++)
        level->flowers.flowers[i].failShake = 0.0f;

    level->stars.powerTimer = 0.0f;

    int beeFace = HexFindFace(&level->grid, level->def->beeStart.q, level->def->beeStart.r);
    int startVertex = FaceLeftmostVertex(&level->grid, beeFace);
    HexBeeInit(&level->bee, &level->grid, startVertex, beeSpeed);
    HexTrailInit(&level->trail, startVertex);

    level->enemyCount = level->def->enemyCount;
    if (level->enemyCount > HEX_LEVEL_MAX_ENEMIES) level->enemyCount = HEX_LEVEL_MAX_ENEMIES;

    int avoid[HEX_LEVEL_MAX_ENEMIES + 1];
    int avoidCount = 0;
    avoid[avoidCount++] = startVertex;

    for (int i = 0; i < level->enemyCount; i++)
    {
        const HexLevelEnemyDef *ed = &level->def->enemies[i];
        int ring = (ed->type == HEX_ENEMY_BLACK_EDGE)? ed->ring : 0;
        int v = HexEnemySpawnVertexAvoid(&level->grid, ed->spawn, avoid, avoidCount, ring);
        avoid[avoidCount++] = v;
        HexEnemyInit(&level->enemies[i], ed->type, &level->grid, v, beeSpeed, ring);
    }
}

//----------------------------------------------------------------------------------
// Update / draw / win
//----------------------------------------------------------------------------------
int HexLevelUpdate(HexLevel *level, float dt)
{
    if (HexBeeUpdate(&level->bee, &level->grid, dt))
        PlayBeeZoom();
    HexGridUpdate(&level->grid, dt);

    int result = 0;
    for (int i = 0; i < level->bee.arrivalCount; i++)
    {
        int filled = HexTrailAdvance(&level->trail, &level->grid, &level->flowers,
                                     level->bee.arrivalEdges[i], level->bee.arrivalVerts[i]);
        if (filled == HEX_TRAIL_TWIN_FAIL) result = HEX_TRAIL_TWIN_FAIL;
        else if (filled == HEX_TRAIL_FIRE_FAIL) result = HEX_TRAIL_FIRE_FAIL;
        else if ((filled > 0) && (result != HEX_TRAIL_TWIN_FAIL) && (result != HEX_TRAIL_FIRE_FAIL))
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
        if (HexEnemyUpdate(enemy, &level->grid, beePos, dt, powered, level->jailFace))
            PlayWaspZoom();

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

void HexLevelDraw(const HexLevel *level)
{
    HexGridDraw(&level->grid);
    HexGridDrawFire(&level->grid);
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
        HexEnemyDraw(&level->enemies[i], &level->grid, powered);
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
    const int fontSize = 20;
    const int boxW = 220;
    const int boxX = GetScreenWidth() - boxW - 16;
    const int boxY = 56;
    const int maxChars = 20;

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
