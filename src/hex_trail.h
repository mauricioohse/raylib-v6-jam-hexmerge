/**********************************************************************************************
*
*   Beehold - Trail path + encircle fill (loop detection on the hex mesh)
*
*   Tracks the ordered path of vertices the bee has walked. When the bee arrives at a
*   vertex already on the path, the slice from that vertex to the end plus the closing
*   edge forms a closed loop; every face the loop encloses gets marked filled -- unless
*   twin-seed rules reject the fill (exactly one twin of a pair enclosed).
*
**********************************************************************************************/

#ifndef hex_trail_H
#define hex_trail_H

#include "raylib.h"
#include "hex_grid.h"
#include "hex_flower.h"

// A path can never be longer than vertexCount + 1 entries: appending a vertex that is
// already on the path immediately closes (and consumes) a loop, so vertices never
// repeat within the stored path.
#define HEX_TRAIL_MAX (HEX_MAX_VERTICES + 1)

typedef struct HexTrail
{
    int vertices[HEX_TRAIL_MAX];    // ordered vertex indices, [0] = trail start
    int edges[HEX_TRAIL_MAX];       // edges[i] connects vertices[i] -> vertices[i+1]
    int vertexCount;
} HexTrail;

void HexTrailInit(HexTrail *trail, int startVertex);

// Register that the bee traveled viaEdge and arrived at toVertex.
// flowers may be NULL (no twin checks).
// Returns: >0 faces filled, 0 no loop / empty / rejected without twin fail,
//          HEX_TRAIL_TWIN_FAIL if a twin pair was split by the loop.
#define HEX_TRAIL_TWIN_FAIL (-1)
int HexTrailAdvance(HexTrail *trail, HexGrid *grid, HexFlowerField *flowers,
                    int viaEdge, int toVertex);

void HexTrailDrawDebug(const HexTrail *trail, const HexGrid *grid);

#endif // hex_trail_H
