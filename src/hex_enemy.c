/**********************************************************************************************
*
*   Beehold - Wasp enemies implementation
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
    { 40, 40, 48, 255 },        // HEX_ENEMY_BLACK_EDGE
};

static const Color ENEMY_FLEE_TINT = { 140, 220, 255, 255 };
static const Color ENEMY_JAIL_TINT = { 160, 190, 220, 200 };

Color HexEnemyTint(HexEnemyType type)
{
    if ((type < 0) || (type >= HEX_ENEMY_TYPE_COUNT)) return WHITE;
    return ENEMY_TINTS[type];
}

bool HexEnemyIsJailed(const HexEnemy *enemy)
{
    return (enemy != NULL) && (enemy->jailTimer > 0.0f);
}

static float Dist2(Vector2 a, Vector2 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx*dx + dy*dy;
}

static bool IsRimEdge(const HexGrid *grid, int edge)
{
    return (edge >= 0) && (edge < grid->edgeCount) && (grid->edges[edge].faceCount == 1);
}

static bool IsJailEdge(const HexGrid *grid, int edge, int jailFace)
{
    if ((jailFace < 0) || (jailFace >= grid->faceCount) || (edge < 0)) return false;
    for (int c = 0; c < 6; c++)
    {
        if (grid->faces[jailFace].edges[c] == edge) return true;
    }
    return false;
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

static int PickFleeExit(const HexGrid *grid, const int *exits, int n, int vertex, Vector2 beePos)
{
    int best = exits[0];
    float bestD = -1.0f;
    for (int i = 0; i < n; i++)
    {
        int other = HexEdgeOtherVertex(grid, exits[i], vertex);
        float d = Dist2(grid->vertices[other].pos, beePos);
        if (d > bestD)
        {
            bestD = d;
            best = exits[i];
        }
    }
    return best;
}

// Exits exclude the reverse edge, so enemies keep moving and never jitter in place
static int PickExit(const HexEnemy *enemy, const HexGrid *grid, int vertex, int fromEdge,
                    Vector2 beePos, bool flee, int jailFace)
{
    const HexVertex *v = &grid->vertices[vertex];
    int exits[3] = { 0 };
    int rimExits[3] = { 0 };
    int n = 0;
    int rimN = 0;
    bool jailed = enemy->jailTimer > 0.0f;

    for (int i = 0; i < v->edgeCount; i++)
    {
        int e = v->edges[i];
        if (e == fromEdge) continue;
        if (jailed)
        {
            if (!IsJailEdge(grid, e, jailFace)) continue;
        }
        exits[n++] = e;
        if (IsRimEdge(grid, e)) rimExits[rimN++] = e;
    }

    // Black rim patrol: prefer rim; if none (e.g. leaving jail), take any exit
    if (!jailed && !flee && (enemy->type == HEX_ENEMY_BLACK_EDGE) && (rimN > 0))
    {
        n = rimN;
        for (int i = 0; i < rimN; i++) exits[i] = rimExits[i];
    }

    if (n == 0) return fromEdge;    // dead end / no legal exit: U-turn
    if (n == 1) return exits[0];

    if (jailed) return exits[GetRandomValue(0, n - 1)];
    if (flee) return PickFleeExit(grid, exits, n, vertex, beePos);

    bool chase = false;
    switch (enemy->type)
    {
        case HEX_ENEMY_RED_RANDOM: chase = false; break;
        case HEX_ENEMY_PURPLE_CHASER: chase = true; break;
        case HEX_ENEMY_GREEN_MIXED: chase = (GetRandomValue(0, 1) == 0); break;
        case HEX_ENEMY_BLACK_EDGE: chase = false; break;   // patrol the rim randomly
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

    if (type == HEX_ENEMY_BLACK_EDGE) enemy->baseSpeed = baseSpeed*1.5f;
    else if (type == HEX_ENEMY_RED_RANDOM) enemy->baseSpeed = baseSpeed;
    else enemy->baseSpeed = baseSpeed*0.5f;
    enemy->speed = enemy->baseSpeed;

    enemy->fromVertex = startVertex;
    enemy->jailTimer = 0.0f;

    const HexVertex *v = &grid->vertices[startVertex];
    int rimChoices[3] = { 0 };
    int rimN = 0;
    for (int i = 0; i < v->edgeCount; i++)
    {
        if (IsRimEdge(grid, v->edges[i])) rimChoices[rimN++] = v->edges[i];
    }

    if ((type == HEX_ENEMY_BLACK_EDGE) && (rimN > 0))
    {
        enemy->edge = rimChoices[GetRandomValue(0, rimN - 1)];
    }
    else
    {
        enemy->edge = v->edges[GetRandomValue(0, v->edgeCount - 1)];
    }
}

void HexEnemySendToJail(HexEnemy *enemy, const HexGrid *grid, int jailFace)
{
    if ((enemy == NULL) || (grid == NULL) || (jailFace < 0) || (jailFace >= grid->faceCount)) return;

    enemy->jailTimer = HEX_ENEMY_JAIL_TIME;
    enemy->speed = enemy->baseSpeed*HEX_ENEMY_FLEE_SPEED_SCALE;
    enemy->t = 0.0f;

    int corner = GetRandomValue(0, 5);
    enemy->fromVertex = grid->faces[jailFace].vertices[corner];
    enemy->edge = grid->faces[jailFace].edges[corner];
}

void HexEnemyUpdate(HexEnemy *enemy, const HexGrid *grid, Vector2 beePos, float dt,
                    bool starPower, int jailFace)
{
    enemy->animTimer += dt;
    while (enemy->animTimer >= ENEMY_FRAME_TIME)
    {
        enemy->animTimer -= ENEMY_FRAME_TIME;
        enemy->animFrame = (enemy->animFrame + 1)%ENEMY_FRAME_COUNT;
    }

    if (enemy->jailTimer > 0.0f)
    {
        enemy->jailTimer -= dt;
        if (enemy->jailTimer < 0.0f) enemy->jailTimer = 0.0f;
        enemy->speed = enemy->baseSpeed*HEX_ENEMY_FLEE_SPEED_SCALE;
    }
    else if (starPower)
    {
        enemy->speed = enemy->baseSpeed*HEX_ENEMY_FLEE_SPEED_SCALE;
    }
    else
    {
        enemy->speed = enemy->baseSpeed;
    }

    if ((enemy->edge < 0) || (enemy->edge >= grid->edgeCount)) return;

    float len = HexEdgeLength(grid, enemy->edge);
    if (len < 0.001f) return;

    enemy->t += (enemy->speed*dt)/len;

    bool flee = starPower && (enemy->jailTimer <= 0.0f);
    while (enemy->t >= 1.0f)
    {
        float overflowPx = (enemy->t - 1.0f)*len;
        int arrived = HexEdgeOtherVertex(grid, enemy->edge, enemy->fromVertex);

        enemy->edge = PickExit(enemy, grid, arrived, enemy->edge, beePos, flee, jailFace);
        enemy->fromVertex = arrived;

        len = HexEdgeLength(grid, enemy->edge);
        enemy->t = (len > 0.001f)? overflowPx/len : 0.0f;
    }
}

Vector2 HexEnemyPosition(const HexEnemy *enemy, const HexGrid *grid)
{
    return HexEdgePoint(grid, enemy->edge, enemy->fromVertex, enemy->t);
}

void HexEnemyDraw(const HexEnemy *enemy, const HexGrid *grid, Texture2D texture, bool starPower)
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

    Color tint = HexEnemyTint(enemy->type);
    if (enemy->jailTimer > 0.0f) tint = ENEMY_JAIL_TINT;
    else if (starPower) tint = ENEMY_FLEE_TINT;

    DrawTexturePro(texture, src, dst, origin, rotation, tint);
}

bool HexEnemyTouches(const HexEnemy *enemy, const HexGrid *grid, Vector2 pos, float radius)
{
    if (HexEnemyIsJailed(enemy)) return false;
    return Dist2(HexEnemyPosition(enemy, grid), pos) <= radius*radius;
}

static int FindInnerVertexAvoid(const HexGrid *grid, const int *avoid, int avoidCount)
{
    int best = -1;
    float bestScore = 1.0e30f;
    Vector2 origin = grid->origin;

    for (int i = 0; i < grid->vertexCount; i++)
    {
        if (grid->vertices[i].edgeCount < 3) continue;
        bool skip = false;
        for (int a = 0; a < avoidCount; a++)
        {
            if (avoid[a] == i) { skip = true; break; }
        }
        if (skip) continue;

        float dx = grid->vertices[i].pos.x - origin.x;
        float dy = grid->vertices[i].pos.y - origin.y;
        float score = dx*dx + dy*dy;
        if (grid->vertices[i].pos.x < origin.x - grid->size) score += 100000.0f;
        if (score < bestScore)
        {
            bestScore = score;
            best = i;
        }
    }

    return best;
}

static bool VertexAvoided(int v, const int *avoid, int avoidCount)
{
    for (int a = 0; a < avoidCount; a++)
    {
        if (avoid[a] == v) return true;
    }
    return false;
}

int HexEnemySpawnVertex(const HexGrid *grid, HexEnemySpawn spawn)
{
    return HexEnemySpawnVertexAvoid(grid, spawn, NULL, 0);
}

int HexEnemySpawnVertexAvoid(const HexGrid *grid, HexEnemySpawn spawn, const int *avoid, int avoidCount)
{
    int preferred = -1;
    switch (spawn)
    {
        case HEX_SPAWN_LEFTMOST: preferred = HexFindLeftmostVertex(grid); break;
        case HEX_SPAWN_RIGHTMOST: preferred = HexFindRightmostVertex(grid); break;
        case HEX_SPAWN_TOPMOST: preferred = HexFindTopmostVertex(grid); break;
        case HEX_SPAWN_BOTTOMMOST: preferred = HexFindBottommostVertex(grid); break;
        case HEX_SPAWN_INNER: preferred = FindInnerVertexAvoid(grid, avoid, avoidCount); break;
        default: preferred = HexFindRightmostVertex(grid); break;
    }

    if ((preferred >= 0) && !VertexAvoided(preferred, avoid, avoidCount)) return preferred;

    // Fallback: farthest vertex from origin that isn't avoided (rim for black, any for others)
    int best = -1;
    float bestScore = -1.0f;
    Vector2 origin = grid->origin;
    for (int i = 0; i < grid->vertexCount; i++)
    {
        if (VertexAvoided(i, avoid, avoidCount)) continue;
        if ((spawn == HEX_SPAWN_INNER) && (grid->vertices[i].edgeCount < 3)) continue;
        float dx = grid->vertices[i].pos.x - origin.x;
        float dy = grid->vertices[i].pos.y - origin.y;
        float score = dx*dx + dy*dy;
        if (score > bestScore)
        {
            bestScore = score;
            best = i;
        }
    }
    return (best >= 0)? best : HexFindRightmostVertex(grid);
}
