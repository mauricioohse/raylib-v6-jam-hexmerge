/**********************************************************************************************
*
*   hexman - Hex grid mesh implementation
*
*   Mesh construction is deterministic: every vertex of the lattice is the East or West
*   corner of exactly one hex (possibly a hex just outside the board), so vertices are
*   identified by (q, r, side) and looked up in a small table -- no position matching.
*
**********************************************************************************************/

#include "hex_grid.h"

#include <math.h>
#include <string.h>

#define SIDE_E 0
#define SIDE_W 1

// Owner coords reach one hex beyond the board on each side
#define VMAP_OFF (HEX_RADIUS + 1)
#define VMAP_DIM (2*HEX_RADIUS + 3)

// Unpainted tiles are dry dirt; painted (alive) tiles show the sprite's true colors
static const Color HEX_DEAD_TINT = { 150, 105, 70, 255 };
static const Color EDGE_TRAIL_COLOR = { 80, 160, 255, 255 };
static const Color EDGE_BASE_COLOR = { 40, 40, 40, 80 };

//----------------------------------------------------------------------------------
// Geometry helpers
//----------------------------------------------------------------------------------
static Vector2 HexCenterPixel(HexCoord coord, float size, Vector2 origin)
{
    float x = size*(1.5f*coord.q);
    float y = size*(sqrtf(3.0f)*(coord.r + 0.5f*coord.q));
    return (Vector2){ origin.x + x, origin.y + y };
}

static Vector2 VertexPos(HexCoord owner, int side, float size, Vector2 origin)
{
    Vector2 center = HexCenterPixel(owner, size, origin);
    float dx = (side == SIDE_E)? size : -size;
    return (Vector2){ center.x + dx, center.y };
}

static float WrapAngle(float a)
{
    while (a <= -PI) a += 2.0f*PI;
    while (a > PI) a -= 2.0f*PI;
    return a;
}

// Direction (radians) leaving vertex along edge
static float EdgeAngleFromVertex(const HexGrid *grid, int edge, int vertex)
{
    int other = HexEdgeOtherVertex(grid, edge, vertex);
    Vector2 a = grid->vertices[vertex].pos;
    Vector2 b = grid->vertices[other].pos;
    return atan2f(b.y - a.y, b.x - a.x);
}

//----------------------------------------------------------------------------------
// Construction
//----------------------------------------------------------------------------------
static int GetOrCreateVertex(HexGrid *grid, int vmap[VMAP_DIM][VMAP_DIM][2], HexCoord owner, int side)
{
    int *slot = &vmap[owner.q + VMAP_OFF][owner.r + VMAP_OFF][side];
    if (*slot >= 0) return *slot;

    if (grid->vertexCount >= HEX_MAX_VERTICES) return -1;

    int index = grid->vertexCount++;
    HexVertex *v = &grid->vertices[index];
    v->pos = VertexPos(owner, side, grid->size, grid->origin);
    v->edgeCount = 0;
    v->edges[0] = v->edges[1] = v->edges[2] = -1;

    *slot = index;
    return index;
}

static int GetOrCreateEdge(HexGrid *grid, int va, int vb, int face)
{
    // An existing edge to vb is already in va's (at most 3) edge slots
    HexVertex *a = &grid->vertices[va];
    for (int i = 0; i < a->edgeCount; i++)
    {
        int e = a->edges[i];
        if (HexEdgeOtherVertex(grid, e, va) == vb)
        {
            if (grid->edges[e].faceCount < 2) grid->edges[e].faces[grid->edges[e].faceCount++] = face;
            return e;
        }
    }

    if (grid->edgeCount >= HEX_MAX_EDGES) return -1;

    int index = grid->edgeCount++;
    HexEdge *e = &grid->edges[index];
    e->v0 = va;
    e->v1 = vb;
    e->faces[0] = face;
    e->faces[1] = -1;
    e->faceCount = 1;
    e->painted = false;

    if (a->edgeCount < 3) a->edges[a->edgeCount++] = index;
    HexVertex *b = &grid->vertices[vb];
    if (b->edgeCount < 3) b->edges[b->edgeCount++] = index;
    return index;
}

void HexGridInit(HexGrid *grid, Vector2 origin, Texture2D hexTexture)
{
    memset(grid, 0, sizeof(*grid));
    grid->size = HEX_SIZE;
    grid->origin = origin;
    grid->hexTexture = hexTexture;

    int vmap[VMAP_DIM][VMAP_DIM][2];
    memset(vmap, -1, sizeof(vmap));

    // Corner c of face (q, r) is the E or W corner of the hex at (q+dq, r+dr),
    // in +x CCW order (angles 0, 60, ... 300)
    static const struct { int dq, dr, side; } CORNER_OWNER[6] = {
        {  0, 0, SIDE_E },      // angle 0
        {  1, 0, SIDE_W },      // angle 60
        { -1, 1, SIDE_E },      // angle 120
        {  0, 0, SIDE_W },      // angle 180
        { -1, 0, SIDE_E },      // angle 240
        {  1, -1, SIDE_W },     // angle 300
    };

    // Hex-shaped board of radius R around (0, 0)
    for (int q = -HEX_RADIUS; q <= HEX_RADIUS; q++)
    {
        int r1 = (-q - HEX_RADIUS > -HEX_RADIUS)? -q - HEX_RADIUS : -HEX_RADIUS;
        int r2 = (-q + HEX_RADIUS < HEX_RADIUS)? -q + HEX_RADIUS : HEX_RADIUS;
        for (int r = r1; r <= r2; r++)
        {
            if (grid->faceCount >= HEX_FACE_COUNT) break;

            int faceIndex = grid->faceCount++;
            HexFace *face = &grid->faces[faceIndex];
            face->coord = (HexCoord){ q, r };
            face->center = HexCenterPixel(face->coord, grid->size, grid->origin);
            face->filled = false;

            for (int c = 0; c < 6; c++)
            {
                HexCoord owner = { q + CORNER_OWNER[c].dq, r + CORNER_OWNER[c].dr };
                face->vertices[c] = GetOrCreateVertex(grid, vmap, owner, CORNER_OWNER[c].side);
            }

            for (int c = 0; c < 6; c++)
            {
                face->edges[c] = GetOrCreateEdge(grid, face->vertices[c], face->vertices[(c + 1)%6], faceIndex);
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Drawing
//----------------------------------------------------------------------------------
void HexGridDraw(const HexGrid *grid)
{
    // Face sprites: hexagon.png centered on each face, tinted when filled
    float texW = (float)grid->hexTexture.width;
    float texH = (float)grid->hexTexture.height;
    Rectangle src = { 0, 0, texW, texH };

    for (int i = 0; i < grid->faceCount; i++)
    {
        Vector2 c = grid->faces[i].center;
        Rectangle dst = { c.x - texW*0.5f, c.y - texH*0.5f, texW, texH };
        Color tint = grid->faces[i].filled? WHITE : HEX_DEAD_TINT;
        DrawTexturePro(grid->hexTexture, src, dst, (Vector2){ 0, 0 }, 0.0f, tint);
    }

    // Edge overlays: faint lattice + wet trail
    for (int i = 0; i < grid->edgeCount; i++)
    {
        Vector2 a = grid->vertices[grid->edges[i].v0].pos;
        Vector2 b = grid->vertices[grid->edges[i].v1].pos;
        Color col = grid->edges[i].painted? EDGE_TRAIL_COLOR : EDGE_BASE_COLOR;
        float thick = grid->edges[i].painted? 3.0f : 1.0f;
        DrawLineEx(a, b, thick, col);
    }
}

//----------------------------------------------------------------------------------
// Queries
//----------------------------------------------------------------------------------
int HexEdgeOtherVertex(const HexGrid *grid, int edge, int vertex)
{
    const HexEdge *e = &grid->edges[edge];
    if (e->v0 == vertex) return e->v1;
    if (e->v1 == vertex) return e->v0;
    return -1;
}

float HexEdgeLength(const HexGrid *grid, int edge)
{
    Vector2 a = grid->vertices[grid->edges[edge].v0].pos;
    Vector2 b = grid->vertices[grid->edges[edge].v1].pos;
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return sqrtf(dx*dx + dy*dy);
}

Vector2 HexEdgePoint(const HexGrid *grid, int edge, int fromVertex, float t)
{
    int toVertex = HexEdgeOtherVertex(grid, edge, fromVertex);
    Vector2 a = grid->vertices[fromVertex].pos;
    Vector2 b = grid->vertices[toVertex].pos;
    return (Vector2){ a.x + (b.x - a.x)*t, a.y + (b.y - a.y)*t };
}

int HexFindLeftmostVertex(const HexGrid *grid)
{
    int best = 0;
    for (int i = 1; i < grid->vertexCount; i++)
    {
        if (grid->vertices[i].pos.x < grid->vertices[best].pos.x) best = i;
    }
    return best;
}

//----------------------------------------------------------------------------------
// Bee on edges
//----------------------------------------------------------------------------------

// Pick the exit edge at a vertex for a turn input (-1 left, +1 right). Screen y grows
// down, so a positive angle delta from the incoming direction is a visual right turn.
// Interior vertices offer exactly two forks (reverse excluded); rim vertices force the
// single remaining edge whichever way was pressed.
static int ChooseExit(const HexGrid *grid, int vertex, int fromEdge, int turn)
{
    int prev = HexEdgeOtherVertex(grid, fromEdge, vertex);
    Vector2 a = grid->vertices[prev].pos;
    Vector2 b = grid->vertices[vertex].pos;
    float inAngle = atan2f(b.y - a.y, b.x - a.x);

    const HexVertex *v = &grid->vertices[vertex];
    int exits[3] = { 0 };
    float deltas[3] = { 0 };
    int n = 0;
    for (int i = 0; i < v->edgeCount; i++)
    {
        int e = v->edges[i];
        if (e == fromEdge) continue;
        float d = WrapAngle(EdgeAngleFromVertex(grid, e, vertex) - inAngle);
        int at = n++;
        while ((at > 0) && (deltas[at - 1] > d))    // keep sorted: leftmost first
        {
            exits[at] = exits[at - 1];
            deltas[at] = deltas[at - 1];
            at--;
        }
        exits[at] = e;
        deltas[at] = d;
    }

    if (n == 0) return fromEdge;    // dead end: U-turn (cannot happen on this board)

    return (turn < 0)? exits[0] : exits[n - 1];     // n == 1: both indices are the forced exit
}

void HexBeeInit(HexBee *bee, HexGrid *grid, int startVertex, float speed)
{
    memset(bee, 0, sizeof(*bee));
    bee->speed = speed;
    bee->fromVertex = startVertex;

    // Take the exit heading furthest into the board (rightward from leftmost start)
    const HexVertex *v = &grid->vertices[startVertex];
    int best = v->edges[0];
    float bestDx = -1000000.0f;
    for (int i = 0; i < v->edgeCount; i++)
    {
        int other = HexEdgeOtherVertex(grid, v->edges[i], startVertex);
        float dx = grid->vertices[other].pos.x - v->pos.x;
        if (dx > bestDx)
        {
            bestDx = dx;
            best = v->edges[i];
        }
    }

    bee->edge = best;
    grid->edges[best].painted = true;
}

void HexBeeSetTurn(HexBee *bee, int dir)
{
    bee->queuedTurn = (dir < 0)? -1 : 1;
}

int HexBeePeekNextEdge(const HexBee *bee, const HexGrid *grid)
{
    if (bee->queuedTurn == 0) return -1;    // no input buffered: bee will stop

    int toVertex = HexEdgeOtherVertex(grid, bee->edge, bee->fromVertex);
    if (toVertex < 0) return -1;

    return ChooseExit(grid, toVertex, bee->edge, bee->queuedTurn);
}

void HexBeeUpdate(HexBee *bee, HexGrid *grid, float dt)
{
    bee->arrivalCount = 0;

    if ((bee->edge < 0) || (bee->edge >= grid->edgeCount)) return;

    // Parked at a junction: wait for input, then launch onto the chosen fork
    if (bee->waiting)
    {
        if (bee->queuedTurn == 0) return;

        int vertex = HexEdgeOtherVertex(grid, bee->edge, bee->fromVertex);
        int nextEdge = ChooseExit(grid, vertex, bee->edge, bee->queuedTurn);
        bee->queuedTurn = 0;
        bee->waiting = false;

        bee->edge = nextEdge;
        bee->fromVertex = vertex;
        bee->t = 0.0f;
        grid->edges[nextEdge].painted = true;
    }

    float len = HexEdgeLength(grid, bee->edge);
    if (len < 0.001f) return;

    bee->t += (bee->speed*dt)/len;

    while (bee->t >= 1.0f)
    {
        float overflowPx = (bee->t - 1.0f)*len;
        int arrived = HexEdgeOtherVertex(grid, bee->edge, bee->fromVertex);
        int viaEdge = bee->edge;

        if (bee->arrivalCount < HEX_BEE_MAX_ARRIVALS)
        {
            bee->arrivalEdges[bee->arrivalCount] = viaEdge;
            bee->arrivalVerts[bee->arrivalCount] = arrived;
            bee->arrivalCount++;
        }

        // No buffered input: park exactly on the vertex until the player steers
        if (bee->queuedTurn == 0)
        {
            bee->waiting = true;
            bee->t = 1.0f;
            break;
        }

        int nextEdge = ChooseExit(grid, arrived, viaEdge, bee->queuedTurn);
        bee->queuedTurn = 0;

        bee->edge = nextEdge;
        bee->fromVertex = arrived;
        grid->edges[nextEdge].painted = true;

        len = HexEdgeLength(grid, bee->edge);
        bee->t = (len > 0.001f)? overflowPx/len : 0.0f;
    }
}

Vector2 HexBeePosition(const HexBee *bee, const HexGrid *grid)
{
    return HexEdgePoint(grid, bee->edge, bee->fromVertex, bee->t);
}

float HexBeeRotationDeg(const HexBee *bee, const HexGrid *grid)
{
    int toVertex = HexEdgeOtherVertex(grid, bee->edge, bee->fromVertex);
    Vector2 a = grid->vertices[bee->fromVertex].pos;
    Vector2 b = grid->vertices[toVertex].pos;
    // Sprite faces up (-y); atan2 gives angle from +x
    float rad = atan2f(b.y - a.y, b.x - a.x);
    return rad*RAD2DEG + 90.0f;
}
