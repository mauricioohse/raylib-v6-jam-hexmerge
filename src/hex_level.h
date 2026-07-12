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

#define HEX_LEVEL_SLOT_COUNT 24
#define HEX_LEVEL_IMPLEMENTED 24
#define HEX_LEVEL_MAX_SEEDS 16
#define HEX_LEVEL_MAX_STARS 4
#define HEX_LEVEL_MAX_SPECIAL 8
#define HEX_LEVEL_MAX_ENEMIES 12

typedef struct HexLevelEnemyDef
{
    HexEnemyType type;
    HexEnemySpawn spawn;
    int ring;           // black wasps only: 0 = outer rim; 1..radius = patrol that ring
} HexLevelEnemyDef;

typedef struct HexLevelSeedDef
{
    HexCoord coord;
    int twinPair;       // -1 = normal; matching ids form a twin bond
} HexLevelSeedDef;

typedef struct HexLevelSpecialDef
{
    HexCoord coord;
    HexFaceKind kind;   // HEX_FACE_FIRE or HEX_FACE_WATER
} HexLevelSpecialDef;

typedef struct HexLevelDef
{
    int radius;
    HexCoord beeStart;                      // axial face; bee starts on that face's leftmost vertex
    HexLevelSeedDef seeds[HEX_LEVEL_MAX_SEEDS];
    int seedCount;
    HexCoord stars[HEX_LEVEL_MAX_STARS];
    int starCount;
    HexLevelSpecialDef specials[HEX_LEVEL_MAX_SPECIAL];
    int specialCount;
    HexLevelEnemyDef enemies[HEX_LEVEL_MAX_ENEMIES];
    int enemyCount;
    const char *hint;                       // sidebar help text (may be NULL)
    bool checkpoint;                        // unused: every level is a checkpoint
    bool seedAll;                           // seed every non-special face (paint the whole board)
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

void HexLevelLoad(HexLevel *level, int index, Texture2D hexTexture, Texture2D pondTexture,
                  Texture2D flowerTexture, Texture2D bubbleTexture, Texture2D starTexture,
                  float beeSpeed);

// Soft death: keep filled faces / flower progress / collected stars; clear wet trail,
// respawn bee at start, and re-place enemies. powerTimer is cleared.
void HexLevelRespawnKeepProgress(HexLevel *level, float beeSpeed);

// Returns faces filled this frame, HEX_TRAIL_TWIN_FAIL, or HEX_TRAIL_FIRE_FAIL.
int HexLevelUpdate(HexLevel *level, float dt);
void HexLevelDraw(const HexLevel *level, Texture2D waspTexture, Texture2D fireTexture);
bool HexLevelWon(const HexLevel *level);
bool HexLevelBeeHit(const HexLevel *level, float hitRadius);
bool HexLevelStarPowered(const HexLevel *level);
void HexLevelDrawHint(const HexLevel *level);

#endif // HEX_LEVEL_H
