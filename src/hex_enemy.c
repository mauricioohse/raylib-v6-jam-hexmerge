/**********************************************************************************************
*
*   Beehold - Wasp enemies implementation
*
**********************************************************************************************/

#include "hex_enemy.h"
#include "hex_assets.h"

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

static int FaceAxialDist(HexCoord c)
{
    int s = -c.q - c.r;
    int aq = (c.q < 0)? -c.q : c.q;
    int ar = (c.r < 0)? -c.r : c.r;
    int as_ = (s < 0)? -s : s;
    int m = (aq > ar)? aq : ar;
    return (m > as_)? m : as_;
}

// Resolve authored ring: 0 (or out of range) means the board's outer rim.
static int EffectiveRing(const HexGrid *grid, int ring)
{
    if ((ring <= 0) || (ring > grid->radius)) return grid->radius;
    return ring;
}

// Edge on the outer perimeter of the disk of faces with axial distance <= ring.
static bool IsRingEdge(const HexGrid *grid, int edge, int ring)
{
    if ((edge < 0) || (edge >= grid->edgeCount)) return false;

    int R = EffectiveRing(grid, ring);
    const HexEdge *e = &grid->edges[edge];

    if (R >= grid->radius)
    {
        return e->faceCount == 1;
    }

    if (e->faceCount != 2) return false;

    int d0 = FaceAxialDist(grid->faces[e->faces[0]].coord);
    int d1 = FaceAxialDist(grid->faces[e->faces[1]].coord);
    int lo = (d0 < d1)? d0 : d1;
    int hi = (d0 > d1)? d0 : d1;
    return (lo == R) && (hi == R + 1);
}

static bool VertexOnRing(const HexGrid *grid, int vertex, int ring)
{
    if ((vertex < 0) || (vertex >= grid->vertexCount)) return false;
    const HexVertex *v = &grid->vertices[vertex];
    for (int i = 0; i < v->edgeCount; i++)
    {
        if (IsRingEdge(grid, v->edges[i], ring)) return true;
    }
    return false;
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
    int ringExits[3] = { 0 };
    int n = 0;
    int ringN = 0;
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
        if (IsRingEdge(grid, e, enemy->ring)) ringExits[ringN++] = e;
    }

    // Black ring patrol: prefer ring edges; if none (e.g. leaving jail), take any exit
    if (!jailed && !flee && (enemy->type == HEX_ENEMY_BLACK_EDGE) && (ringN > 0))
    {
        n = ringN;
        for (int i = 0; i < ringN; i++) exits[i] = ringExits[i];
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
        case HEX_ENEMY_BLACK_EDGE: chase = false; break;   // patrol the ring randomly
        default: break;
    }

    if (chase) return PickChaseExit(grid, exits, n, vertex, beePos);
    return exits[GetRandomValue(0, n - 1)];
}

//----------------------------------------------------------------------------------
// Lifecycle
//----------------------------------------------------------------------------------
void HexEnemyInit(HexEnemy *enemy, HexEnemyType type, const HexGrid *grid, int startVertex,
                  float baseSpeed, int ring)
{
    memset(enemy, 0, sizeof(*enemy));
    enemy->type = type;
    enemy->ring = (type == HEX_ENEMY_BLACK_EDGE)? EffectiveRing(grid, ring) : 0;

    if (type == HEX_ENEMY_BLACK_EDGE) enemy->baseSpeed = baseSpeed*1.5f;
    else if (type == HEX_ENEMY_RED_RANDOM) enemy->baseSpeed = baseSpeed;
    else enemy->baseSpeed = baseSpeed*0.5f;
    enemy->speed = enemy->baseSpeed;

    enemy->fromVertex = startVertex;
    enemy->jailTimer = 0.0f;

    const HexVertex *v = &grid->vertices[startVertex];
    int ringChoices[3] = { 0 };
    int ringN = 0;
    for (int i = 0; i < v->edgeCount; i++)
    {
        if (IsRingEdge(grid, v->edges[i], enemy->ring)) ringChoices[ringN++] = v->edges[i];
    }

    if ((type == HEX_ENEMY_BLACK_EDGE) && (ringN > 0))
    {
        enemy->edge = ringChoices[GetRandomValue(0, ringN - 1)];
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

bool HexEnemyUpdate(HexEnemy *enemy, const HexGrid *grid, Vector2 beePos, float dt,
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

    if ((enemy->edge < 0) || (enemy->edge >= grid->edgeCount)) return false;

    float len = HexEdgeLength(grid, enemy->edge);
    if (len < 0.001f) return false;

    enemy->t += (enemy->speed*dt)/len;

    bool hopped = false;
    bool flee = starPower && (enemy->jailTimer <= 0.0f);
    while (enemy->t >= 1.0f)
    {
        float overflowPx = (enemy->t - 1.0f)*len;
        int arrived = HexEdgeOtherVertex(grid, enemy->edge, enemy->fromVertex);

        enemy->edge = PickExit(enemy, grid, arrived, enemy->edge, beePos, flee, jailFace);
        enemy->fromVertex = arrived;
        hopped = true;

        len = HexEdgeLength(grid, enemy->edge);
        enemy->t = (len > 0.001f)? overflowPx/len : 0.0f;
    }
    return hopped;
}

Vector2 HexEnemyPosition(const HexEnemy *enemy, const HexGrid *grid)
{
    return HexEdgePoint(grid, enemy->edge, enemy->fromVertex, enemy->t);
}

void HexEnemyDraw(const HexEnemy *enemy, const HexGrid *grid, bool starPower)
{
    Vector2 pos = HexEnemyPosition(enemy, grid);

    float frameW = (float)assets.wasp.width;
    float frameH = (float)assets.wasp.height/(float)ENEMY_FRAME_COUNT;
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

    DrawTexturePro(assets.wasp, src, dst, origin, rotation, tint);
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
    return HexEnemySpawnVertexAvoid(grid, spawn, NULL, 0, 0);
}

int HexEnemySpawnVertexAvoid(const HexGrid *grid, HexEnemySpawn spawn, const int *avoid, int avoidCount,
                             int ring)
{
    int preferRing = (ring > 0)? EffectiveRing(grid, ring) : 0;

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

    // If a ring is requested, directional extremes may not lie on it — pick among ring verts.
    if (preferRing > 0)
    {
        if ((preferred < 0) || VertexAvoided(preferred, avoid, avoidCount) ||
            !VertexOnRing(grid, preferred, preferRing))
        {
            preferred = -1;
            float bestScore = -1.0e30f;
            Vector2 origin = grid->origin;
            for (int i = 0; i < grid->vertexCount; i++)
            {
                if (VertexAvoided(i, avoid, avoidCount)) continue;
                if (!VertexOnRing(grid, i, preferRing)) continue;

                float dx = grid->vertices[i].pos.x - origin.x;
                float dy = grid->vertices[i].pos.y - origin.y;
                float score = 0.0f;
                switch (spawn)
                {
                    case HEX_SPAWN_LEFTMOST: score = -grid->vertices[i].pos.x; break;
                    case HEX_SPAWN_RIGHTMOST: score = grid->vertices[i].pos.x; break;
                    case HEX_SPAWN_TOPMOST: score = -grid->vertices[i].pos.y; break;
                    case HEX_SPAWN_BOTTOMMOST: score = grid->vertices[i].pos.y; break;
                    case HEX_SPAWN_INNER: score = -(dx*dx + dy*dy); break;
                    default: score = dx*dx + dy*dy; break;
                }
                if (score > bestScore)
                {
                    bestScore = score;
                    preferred = i;
                }
            }
        }
        if (preferred >= 0) return preferred;
    }
    else if ((preferred >= 0) && !VertexAvoided(preferred, avoid, avoidCount))
    {
        return preferred;
    }

    // Fallback: farthest vertex from origin that isn't avoided
    int best = -1;
    float bestScore = -1.0f;
    Vector2 origin = grid->origin;
    for (int i = 0; i < grid->vertexCount; i++)
    {
        if (VertexAvoided(i, avoid, avoidCount)) continue;
        if ((spawn == HEX_SPAWN_INNER) && (grid->vertices[i].edgeCount < 3)) continue;
        if ((preferRing > 0) && !VertexOnRing(grid, i, preferRing)) continue;
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
