/**********************************************************************************************
*
*   hexman - Trail path + encircle fill (loop detection on the hex mesh)
*
*   Tracks the ordered path of vertices the bee has walked. When the bee arrives at a
*   vertex already on the path, the slice from that vertex to the end plus the closing
*   edge forms a closed loop; every face the loop encloses gets marked filled.
*   Filled faces are rendered by HexGridDraw (tinted hexagon.png).
*
**********************************************************************************************/

#ifndef hex_trail_H
#define hex_trail_H

#include "raylib.h"
#include "hex_grid.h"

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

// Register that the bee traveled viaEdge and arrived at toVertex (feed it each entry
// of bee.arrivalEdges/arrivalVerts after HexBeeUpdate).
// If this closes a loop: fills enclosed faces, unpaints the loop's edges, and
// truncates the path back to the closing vertex (the stem before the loop survives).
// Returns the number of faces newly filled (0 when no loop closed, or a degenerate
// loop enclosed nothing).
int HexTrailAdvance(HexTrail *trail, HexGrid *grid, int viaEdge, int toVertex);

// Debug overlay: path polyline + vertex dots.
void HexTrailDrawDebug(const HexTrail *trail, const HexGrid *grid);

#endif // hex_trail_H
