/**********************************************************************************************
*
*   Beehold - Wasp enemies walking the hex lattice edges
*
*   Enemies move like the bee (edge to edge, continuous) but never stop and never
*   paint. Each type is a color + junction behavior pairing; the tint is applied to
*   the shared wasp.png spritesheet (16x64, 4 vertical frames, faces up).
*
**********************************************************************************************/

#ifndef HEX_ENEMY_H
#define HEX_ENEMY_H

#include "raylib.h"
#include "hex_grid.h"

typedef enum HexEnemyType
{
    HEX_ENEMY_RED_RANDOM = 0,   // full speed, picks a random fork at every junction
    HEX_ENEMY_PURPLE_CHASER,    // half speed, always takes the fork nearest the bee
    HEX_ENEMY_GREEN_MIXED,      // half speed, coin flip per junction: chase or wander
    HEX_ENEMY_BLACK_EDGE,       // 1.5x bee speed, only walks rim edges (faceCount == 1)
    HEX_ENEMY_TYPE_COUNT
} HexEnemyType;

typedef enum HexEnemySpawn
{
    HEX_SPAWN_LEFTMOST = 0,
    HEX_SPAWN_RIGHTMOST,
    HEX_SPAWN_TOPMOST,
    HEX_SPAWN_BOTTOMMOST,
    HEX_SPAWN_INNER             // interior vertex (3 edges), away from the leftmost rim
} HexEnemySpawn;

typedef struct HexEnemy
{
    HexEnemyType type;
    int edge;
    int fromVertex;     // travel from this endpoint toward the other
    float t;            // 0..1 along current edge
    float speed;        // pixels per second
    float animTimer;
    int animFrame;
} HexEnemy;

Color HexEnemyTint(HexEnemyType type);

// baseSpeed is the bee's speed; chaser and mixed run at half of it
void HexEnemyInit(HexEnemy *enemy, HexEnemyType type, const HexGrid *grid, int startVertex, float baseSpeed);
void HexEnemyUpdate(HexEnemy *enemy, const HexGrid *grid, Vector2 beePos, float dt);
Vector2 HexEnemyPosition(const HexEnemy *enemy, const HexGrid *grid);
void HexEnemyDraw(const HexEnemy *enemy, const HexGrid *grid, Texture2D texture);

// Touch check against a point (the bee), for kill-on-touch
bool HexEnemyTouches(const HexEnemy *enemy, const HexGrid *grid, Vector2 pos, float radius);

int HexEnemySpawnVertex(const HexGrid *grid, HexEnemySpawn spawn);

// Like HexEnemySpawnVertex, but skips any vertex in avoid[] (e.g. bee start / other wasps).
int HexEnemySpawnVertexAvoid(const HexGrid *grid, HexEnemySpawn spawn, const int *avoid, int avoidCount);

#endif // HEX_ENEMY_H
