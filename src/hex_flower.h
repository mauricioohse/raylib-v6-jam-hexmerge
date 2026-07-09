/**********************************************************************************************
*
*   hexman - Flower / seed objectives on hex faces
*
*   Seeds sit on faces. When a face is filled (encircled), the seed sprouts through a
*   short grow animation into an idle bloom. The level is won when every seed has
*   bloomed and all flower faces lie in one connected component of filled faces.
*
*   flower.png is a 16x96 vertical sheet (6 frames):
*     0     unsprouted seed
*     1..2  sprouting
*     3..5  idle bloom loop
*
**********************************************************************************************/

#ifndef HEX_FLOWER_H
#define HEX_FLOWER_H

#include "raylib.h"
#include "hex_grid.h"

#define HEX_FLOWER_MAX 16
#define HEX_FLOWER_FRAME_COUNT 6
#define HEX_FLOWER_SCALE 2.0f

typedef enum HexFlowerState
{
    HEX_FLOWER_SEED = 0,
    HEX_FLOWER_SPROUTING,
    HEX_FLOWER_BLOOMED
} HexFlowerState;

typedef struct HexFlower
{
    int face;               // index into HexGrid.faces
    HexFlowerState state;
    float animTimer;
    int animFrame;          // 0..5 into the spritesheet
} HexFlower;

typedef struct HexFlowerField
{
    HexFlower flowers[HEX_FLOWER_MAX];
    int count;
    Texture2D texture;
} HexFlowerField;

// Place flowers on the given face indices (duplicates / OOB skipped). Resets animation.
void HexFlowerFieldInit(HexFlowerField *field, Texture2D texture, const int *faces, int faceCount);

// After faces are filled: start sprouting any seed whose face is now filled.
void HexFlowerFieldOnFill(HexFlowerField *field, const HexGrid *grid);

void HexFlowerFieldUpdate(HexFlowerField *field, float dt);
void HexFlowerFieldDraw(const HexFlowerField *field, const HexGrid *grid);

// True when every flower has bloomed and all flower faces share one filled region.
bool HexFlowerFieldWon(const HexFlowerField *field, const HexGrid *grid);

int HexFindFace(const HexGrid *grid, int q, int r);

#endif // HEX_FLOWER_H
