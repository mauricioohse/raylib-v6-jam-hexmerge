/**********************************************************************************************
*
*   Beehold - Level definitions (fixed layouts, load/unload via HexLevelLoad)
*
*   Up to HEX_LEVEL_SLOT_COUNT slots. DEBUG: 1-9 jump to levels 1-9, 0 = level 10,
*   ,/< previous level, ./> next level.
*
**********************************************************************************************/

#ifndef HEX_LEVEL_H
#define HEX_LEVEL_H

#include "raylib.h"
#include "hex_grid.h"
#include "hex_enemy.h"
#include "hex_flower.h"
#include "hex_star.h"
#include "hex_trail.h"

#define HEX_LEVEL_SLOT_COUNT 13
#define HEX_LEVEL_IMPLEMENTED 13
#define HEX_LEVEL_MAX_SEEDS 16
#define HEX_LEVEL_MAX_STARS 4
#define HEX_LEVEL_MAX_ENEMIES 8

typedef struct HexLevelEnemyDef
{
    HexEnemyType type;
    HexEnemySpawn spawn;
} HexLevelEnemyDef;

typedef struct HexLevelSeedDef
{
    HexCoord coord;
    int twinPair;       // -1 = normal; matching ids form a twin bond
} HexLevelSeedDef;

typedef struct HexLevelDef
{
    int radius;
    HexCoord beeStart;                      // axial face; bee starts on that face's leftmost vertex
    HexLevelSeedDef seeds[HEX_LEVEL_MAX_SEEDS];
    int seedCount;
    HexCoord stars[HEX_LEVEL_MAX_STARS];
    int starCount;
    HexLevelEnemyDef enemies[HEX_LEVEL_MAX_ENEMIES];
    int enemyCount;
    const char *hint;                       // sidebar help text (may be NULL)
} HexLevelDef;

typedef struct HexLevel
{
    const HexLevelDef *def;
    int index;                              // 0-based
    HexGrid grid;
    HexBee bee;
    HexTrail trail;
    HexFlowerField flowers;
    HexStarField stars;
    HexEnemy enemies[HEX_LEVEL_MAX_ENEMIES];
    int enemyCount;
    int jailFace;                           // center hex for star-jail (-1 if none)
} HexLevel;

const HexLevelDef *HexLevelGetDef(int index);
int HexLevelCount(void);

void HexLevelLoad(HexLevel *level, int index, Texture2D hexTexture, Texture2D flowerTexture,
                  Texture2D bubbleTexture, Texture2D starTexture, float beeSpeed);

// Returns faces filled this frame, or HEX_TRAIL_TWIN_FAIL on twin-rule reject.
int HexLevelUpdate(HexLevel *level, float dt);
void HexLevelDraw(const HexLevel *level, Texture2D waspTexture);
bool HexLevelWon(const HexLevel *level);
bool HexLevelBeeHit(const HexLevel *level, float hitRadius);
bool HexLevelStarPowered(const HexLevel *level);
void HexLevelDrawHint(const HexLevel *level);

#endif // HEX_LEVEL_H
