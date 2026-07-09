/**********************************************************************************************
*
*   hexman - Wasp enemies implementation
*
**********************************************************************************************/

#include "hex_enemy.h"

#include <math.h>
#include <string.h>

#define ENEMY_FRAME_COUNT 4
#define ENEMY_FRAME_TIME 0.15f      // seconds per animation frame
#define ENEMY_SCALE 2.0f

static const Color ENEMY_TINTS[HEX_ENEMY_TYPE_COUNT] = {
    { 235, 90, 80, 255 },       // HEX_ENEMY_RED_RANDOM
    { 185, 105, 255, 255 },     // HEX_ENEMY_PURPLE_CHASER
    { 110, 220, 120, 255 },     // HEX_ENEMY_GREEN_MIXED
};

Color HexEnemyTint(HexEnemyType type)
{
    if ((type < 0) || (type >= HEX_ENEMY_TYPE_COUNT)) return WHITE;
    return ENEMY_TINTS[type];
}

static float Dist2(Vector2 a, Vector2 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx*dx + dy*dy;
}

//----------------------------------------------------------------------------------
// Junction behavior
//----------------------------------------------------------------------------------
static int PickChaseExit(const HexGrid *grid, const int *exits, int n, int vertex, Vector2 beePos)
{
    int best = exits[0];
    float bestD = -1.0f;
    for (int i = 0; i < n; i++)
    {
        int other = HexEdgeOtherVertex(grid, exits[i], vertex);
        float d = Dist2(grid->vertices[other].pos, beePos);
        if ((bestD < 0.0f) || (d < bestD))
        {
            bestD = d;
            best = exits[i];
        }
    }
    return best;
}

// Exits exclude the reverse edge, so enemies keep moving and never jitter in place
static int PickExit(const HexEnemy *enemy, const HexGrid *grid, int vertex, int fromEdge, Vector2 beePos)
{
    const HexVertex *v = &grid->vertices[vertex];
    int exits[3] = { 0 };
    int n = 0;
    for (int i = 0; i < v->edgeCount; i++)
    {
        if (v->edges[i] != fromEdge) exits[n++] = v->edges[i];
    }

    if (n == 0) return fromEdge;    // dead end: U-turn (cannot happen on this board)
    if (n == 1) return exits[0];    // rim: forced

    bool chase = false;
    switch (enemy->type)
    {
        case HEX_ENEMY_RED_RANDOM: chase = false; break;
        case HEX_ENEMY_PURPLE_CHASER: chase = true; break;
        case HEX_ENEMY_GREEN_MIXED: chase = (GetRandomValue(0, 1) == 0); break;
        default: break;
    }

    if (chase) return PickChaseExit(grid, exits, n, vertex, beePos);
    return exits[GetRandomValue(0, n - 1)];
}

//----------------------------------------------------------------------------------
// Lifecycle
//----------------------------------------------------------------------------------
void HexEnemyInit(HexEnemy *enemy, HexEnemyType type, const HexGrid *grid, int startVertex, float baseSpeed)
{
    memset(enemy, 0, sizeof(*enemy));
    enemy->type = type;
    enemy->speed = (type == HEX_ENEMY_RED_RANDOM)? baseSpeed : baseSpeed*0.5f;
    enemy->fromVertex = startVertex;

    const HexVertex *v = &grid->vertices[startVertex];
    enemy->edge = v->edges[GetRandomValue(0, v->edgeCount - 1)];
}

void HexEnemyUpdate(HexEnemy *enemy, const HexGrid *grid, Vector2 beePos, float dt)
{
    enemy->animTimer += dt;
    while (enemy->animTimer >= ENEMY_FRAME_TIME)
    {
        enemy->animTimer -= ENEMY_FRAME_TIME;
        enemy->animFrame = (enemy->animFrame + 1)%ENEMY_FRAME_COUNT;
    }

    if ((enemy->edge < 0) || (enemy->edge >= grid->edgeCount)) return;

    float len = HexEdgeLength(grid, enemy->edge);
    if (len < 0.001f) return;

    enemy->t += (enemy->speed*dt)/len;

    while (enemy->t >= 1.0f)
    {
        float overflowPx = (enemy->t - 1.0f)*len;
        int arrived = HexEdgeOtherVertex(grid, enemy->edge, enemy->fromVertex);

        enemy->edge = PickExit(enemy, grid, arrived, enemy->edge, beePos);
        enemy->fromVertex = arrived;

        len = HexEdgeLength(grid, enemy->edge);
        enemy->t = (len > 0.001f)? overflowPx/len : 0.0f;
    }
}

Vector2 HexEnemyPosition(const HexEnemy *enemy, const HexGrid *grid)
{
    return HexEdgePoint(grid, enemy->edge, enemy->fromVertex, enemy->t);
}

void HexEnemyDraw(const HexEnemy *enemy, const HexGrid *grid, Texture2D texture)
{
    Vector2 pos = HexEnemyPosition(enemy, grid);

    float frameW = (float)texture.width;
    float frameH = (float)texture.height/(float)ENEMY_FRAME_COUNT;
    Rectangle src = { 0, frameH*(float)enemy->animFrame, frameW, frameH };
    Rectangle dst = { pos.x, pos.y, frameW*ENEMY_SCALE, frameH*ENEMY_SCALE };
    Vector2 origin = { dst.width*0.5f, dst.height*0.5f };

    // Sprite faces up (-y)
    int toVertex = HexEdgeOtherVertex(grid, enemy->edge, enemy->fromVertex);
    Vector2 a = grid->vertices[enemy->fromVertex].pos;
    Vector2 b = grid->vertices[toVertex].pos;
    float rotation = atan2f(b.y - a.y, b.x - a.x)*RAD2DEG + 90.0f;

    DrawTexturePro(texture, src, dst, origin, rotation, HexEnemyTint(enemy->type));
}

bool HexEnemyTouches(const HexEnemy *enemy, const HexGrid *grid, Vector2 pos, float radius)
{
    return Dist2(HexEnemyPosition(enemy, grid), pos) <= radius*radius;
}

int HexEnemySpawnVertex(const HexGrid *grid, HexEnemySpawn spawn)
{
    switch (spawn)
    {
        case HEX_SPAWN_LEFTMOST: return HexFindLeftmostVertex(grid);
        case HEX_SPAWN_RIGHTMOST: return HexFindRightmostVertex(grid);
        case HEX_SPAWN_TOPMOST: return HexFindTopmostVertex(grid);
        case HEX_SPAWN_BOTTOMMOST: return HexFindBottommostVertex(grid);
        default: return HexFindRightmostVertex(grid);
    }
}
