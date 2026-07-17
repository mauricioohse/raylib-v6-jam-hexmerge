/**********************************************************************************************
*
*   Beehold - Star seed powerups
*
*   Star seeds sit on faces (separate from flower objectives). Walking onto a star's
*   perimeter collects it and grants HEX_STAR_POWER_TIME seconds of star power: the bee
*   can eat wasps, wasps flee slowly, and star_power music plays.
*
*   star.png is a 16x64 vertical sheet (4 sparkle frames).
*
**********************************************************************************************/

#ifndef HEX_STAR_H
#define HEX_STAR_H

#include "raylib.h"
#include "hex_grid.h"

#define HEX_STAR_MAX 8
#define HEX_STAR_FRAME_COUNT 4
#define HEX_STAR_SCALE 2.0f
#define HEX_STAR_POWER_TIME 6.0f

typedef struct HexStar
{
    int face;
    bool collected;
    float animTimer;
    int animFrame;
} HexStar;

typedef struct HexStarField
{
    HexStar stars[HEX_STAR_MAX];
    int count;
    float powerTimer;       // >0 while bee is star-powered
} HexStarField;

void HexStarFieldInit(HexStarField *field, const int *faces, int faceCount);
void HexStarFieldUpdate(HexStarField *field, float dt);

// If bee is on an edge of an uncollected star face, collect it and start/refresh power.
// Returns true if a star was collected this call.
bool HexStarFieldTryCollect(HexStarField *field, const HexGrid *grid, int beeEdge);

void HexStarFieldDraw(const HexStarField *field, const HexGrid *grid);

bool HexStarFieldPowered(const HexStarField *field);

#endif // HEX_STAR_H
