/**********************************************************************************************
*
*   Beehold - Trail path + encircle fill implementation
*
**********************************************************************************************/

#include "hex_trail.h"

#include <string.h>

//----------------------------------------------------------------------------------
// Fill
//----------------------------------------------------------------------------------

// Compute which faces would be enclosed by loopEdges (without mutating the grid).
static void ComputeEnclosed(const HexGrid *grid, const bool loopEdges[HEX_MAX_EDGES],
                            bool enclosed[HEX_MAX_FACES])
{
    bool reachable[HEX_MAX_FACES] = { false };
    int queue[HEX_MAX_FACES] = { 0 };
    int head = 0;
    int tail = 0;

    for (int f = 0; f < grid->faceCount; f++)
    {
        const HexFace *face = &grid->faces[f];
        for (int c = 0; c < 6; c++)
        {
            int e = face->edges[c];
            if ((e < 0) || loopEdges[e]) continue;
            if (grid->edges[e].faceCount == 1)
            {
                reachable[f] = true;
                queue[tail++] = f;
                break;
            }
        }
    }

    while (head < tail)
    {
        int f = queue[head++];
        for (int c = 0; c < 6; c++)
        {
            int e = grid->faces[f].edges[c];
            if ((e < 0) || loopEdges[e]) continue;

            const HexEdge *edge = &grid->edges[e];
            if (edge->faceCount < 2) continue;

            int other = (edge->faces[0] == f)? edge->faces[1] : edge->faces[0];
            if ((other < 0) || reachable[other]) continue;

            reachable[other] = true;
            queue[tail++] = other;
        }
    }

    for (int f = 0; f < grid->faceCount; f++)
    {
        enclosed[f] = !reachable[f] && !grid->faces[f].filled;
    }
}

static int ApplyEnclosed(HexGrid *grid, const bool enclosed[HEX_MAX_FACES])
{
    int filled = 0;
    for (int f = 0; f < grid->faceCount; f++)
    {
        if (enclosed[f])
        {
            grid->faces[f].filled = true;
            filled++;
        }
    }
    return filled;
}

//----------------------------------------------------------------------------------
// Trail
//----------------------------------------------------------------------------------
void HexTrailInit(HexTrail *trail, int startVertex)
{
    memset(trail, 0, sizeof(*trail));
    trail->vertices[0] = startVertex;
    trail->vertexCount = 1;
}

int HexTrailAdvance(HexTrail *trail, HexGrid *grid, HexFlowerField *flowers,
                    int viaEdge, int toVertex)
{
    if ((viaEdge < 0) || (viaEdge >= grid->edgeCount)) return 0;
    if ((toVertex < 0) || (toVertex >= grid->vertexCount)) return 0;
    if (trail->vertexCount <= 0) { HexTrailInit(trail, toVertex); return 0; }

    int loopStart = -1;
    for (int i = 0; i < trail->vertexCount; i++)
    {
        if (trail->vertices[i] == toVertex) { loopStart = i; break; }
    }

    if (loopStart < 0)
    {
        if (trail->vertexCount >= HEX_TRAIL_MAX) return 0;
        trail->edges[trail->vertexCount - 1] = viaEdge;
        trail->vertices[trail->vertexCount++] = toVertex;
        return 0;
    }

    bool loopEdges[HEX_MAX_EDGES] = { false };
    for (int i = loopStart; i < trail->vertexCount - 1; i++)
    {
        int e = trail->edges[i];
        if ((e >= 0) && (e < grid->edgeCount)) loopEdges[e] = true;
    }
    loopEdges[viaEdge] = true;

    bool enclosed[HEX_MAX_FACES] = { false };
    ComputeEnclosed(grid, loopEdges, enclosed);

    int result = 0;
    if (HexFlowerTwinsAllowFill(flowers, grid, enclosed))
    {
        result = ApplyEnclosed(grid, enclosed);
    }
    else
    {
        HexFlowerFieldOnTwinFail(flowers, enclosed);
        result = HEX_TRAIL_TWIN_FAIL;
    }

    for (int e = 0; e < grid->edgeCount; e++)
    {
        if (loopEdges[e]) grid->edges[e].painted = false;
    }
    trail->vertexCount = loopStart + 1;

    return result;
}

//----------------------------------------------------------------------------------
// Drawing
//----------------------------------------------------------------------------------
void HexTrailDrawDebug(const HexTrail *trail, const HexGrid *grid)
{
    for (int i = 0; i < trail->vertexCount - 1; i++)
    {
        Vector2 a = grid->vertices[trail->vertices[i]].pos;
        Vector2 b = grid->vertices[trail->vertices[i + 1]].pos;
        DrawLineEx(a, b, 2.0f, (Color){ 255, 80, 160, 200 });
    }

    for (int i = 0; i < trail->vertexCount; i++)
    {
        Vector2 p = grid->vertices[trail->vertices[i]].pos;
        DrawCircleV(p, 3.0f, (i == 0)? (Color){ 80, 255, 120, 255 } : (Color){ 255, 80, 160, 255 });
    }
}
