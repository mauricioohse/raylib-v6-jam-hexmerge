/**********************************************************************************************
*
*   Beehold - Level definitions (fixed layouts, load/unload via HexLevelLoad)
*
*   Up to HEX_LEVEL_SLOT_COUNT slots (keys 1-9 in DEBUG). Only the first
*   HEX_LEVEL_IMPLEMENTED levels are fully authored; the rest fall back to level 1.
*
**********************************************************************************************/

#ifndef HEX_LEVEL_H
#define HEX_LEVEL_H

#include "raylib.h"
#include "hex_grid.h"
#include "hex_enemy.h"
#include "hex_flower.h"
#include "hex_trail.h"

#define HEX_LEVEL_SLOT_COUNT 9
#define HEX_LEVEL_IMPLEMENTED 9
#define HEX_LEVEL_MAX_SEEDS 16
#define HEX_LEVEL_MAX_ENEMIES 8

typedef struct HexLevelEnemyDef
{
    HexEnemyType type;
    HexEnemySpawn spawn;
} HexLevelEnemyDef;

typedef struct HexLevelDef
{
    int radius;
    HexCoord beeStart;                      // axial face; bee starts on that face's leftmost vertex
    HexCoord seeds[HEX_LEVEL_MAX_SEEDS];
    int seedCount;
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
    HexEnemy enemies[HEX_LEVEL_MAX_ENEMIES];
    int enemyCount;
} HexLevel;

const HexLevelDef *HexLevelGetDef(int index);   // clamps / falls back for unimplemented slots
int HexLevelCount(void);                        // HEX_LEVEL_IMPLEMENTED

// Build grid + bee + seeds + wasps from a definition. Textures must already be loaded.
void HexLevelLoad(HexLevel *level, int index, Texture2D hexTexture, Texture2D flowerTexture, float beeSpeed);

// Returns faces newly filled this frame (for SFX).
int HexLevelUpdate(HexLevel *level, float dt);
void HexLevelDraw(const HexLevel *level, Texture2D waspTexture);
bool HexLevelWon(const HexLevel *level);
bool HexLevelBeeHit(const HexLevel *level, float hitRadius);

// Wrap text into a right-side hint panel (no-op if hint is NULL/empty).
void HexLevelDrawHint(const HexLevel *level);

#endif // HEX_LEVEL_H
