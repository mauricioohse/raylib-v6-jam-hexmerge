/**********************************************************************************************
*
*   Beehold - Hex grid mesh (flat-top faces, walkable edges/vertices) + bee movement
*
*   Faces are drawn as hexagon.png sprites centered on face.center. Vertices and edges
*   form the lattice the bee walks on; the trail/encircle mechanic lives in
*   hex_trail.h and operates on the same indices.
*
*   Board radius is chosen per level (see hex_level.h). Arrays are sized for HEX_MAX_RADIUS.
*
**********************************************************************************************/

#ifndef hex_grid_H
#define hex_grid_H

#include "raylib.h"

#define HEX_MAX_RADIUS 4
#define HEX_MAX_FACES (3*HEX_MAX_RADIUS*(HEX_MAX_RADIUS + 1) + 1)   // 61
#define HEX_MAX_VERTICES 192
#define HEX_MAX_EDGES 288
#define HEX_TEX_WIDTH 73.0f
#define HEX_SIZE (HEX_TEX_WIDTH*0.5f)   // center-to-vertex radius

#define HEX_BEE_MAX_ARRIVALS 8

typedef struct HexCoord
{
    int q;
    int r;
} HexCoord;

typedef struct HexVertex
{
    Vector2 pos;
    int edges[3];       // -1 when unused; rim vertices have 2 edges
    int edgeCount;
} HexVertex;

typedef struct HexEdge
{
    int v0;
    int v1;
    int faces[2];
    int faceCount;      // 1 = board rim edge
    bool painted;       // wet trail from the bee
} HexEdge;

typedef struct HexFace
{
    HexCoord coord;     // axial
    Vector2 center;     // pixel center: draw the hex png here
    int vertices[6];    // corners in +x CCW order (angle 0, 60, ... 300)
    int edges[6];       // edges[c] connects vertices[c] -> vertices[(c+1)%6]
    bool filled;        // fertilized/painted territory
} HexFace;

typedef struct HexGrid
{
    HexVertex vertices[HEX_MAX_VERTICES];
    HexEdge edges[HEX_MAX_EDGES];
    HexFace faces[HEX_MAX_FACES];
    int vertexCount;
    int edgeCount;
    int faceCount;
    int radius;
    float size;
    Vector2 origin;
    Texture2D hexTexture;
} HexGrid;

typedef struct HexBee
{
    int edge;
    int fromVertex;     // travel from this endpoint toward the other
    float t;            // 0..1 along current edge
    float speed;        // pixels per second
    int queuedTurn;     // buffered input: -1 = left fork (A), +1 = right fork (D), 0 = none
    bool waiting;       // stopped at a junction (t == 1 on edge) until input arrives

    // Vertices crossed during the last HexBeeUpdate, in order, for the trail module
    int arrivalVerts[HEX_BEE_MAX_ARRIVALS];
    int arrivalEdges[HEX_BEE_MAX_ARRIVALS];
    int arrivalCount;
} HexBee;

void HexGridInit(HexGrid *grid, Vector2 origin, Texture2D hexTexture, int radius);
void HexGridDraw(const HexGrid *grid);

int HexEdgeOtherVertex(const HexGrid *grid, int edge, int vertex);
float HexEdgeLength(const HexGrid *grid, int edge);
Vector2 HexEdgePoint(const HexGrid *grid, int edge, int fromVertex, float t);

int HexFindLeftmostVertex(const HexGrid *grid);
int HexFindRightmostVertex(const HexGrid *grid);
int HexFindTopmostVertex(const HexGrid *grid);
int HexFindBottommostVertex(const HexGrid *grid);

void HexBeeInit(HexBee *bee, HexGrid *grid, int startVertex, float speed);
void HexBeeSetTurn(HexBee *bee, int dir);   // -1 left, +1 right; consumed at next junction
void HexBeeUpdate(HexBee *bee, HexGrid *grid, float dt);
Vector2 HexBeePosition(const HexBee *bee, const HexGrid *grid);
float HexBeeRotationDeg(const HexBee *bee, const HexGrid *grid);
int HexBeePeekNextEdge(const HexBee *bee, const HexGrid *grid);

#endif // hex_grid_H
