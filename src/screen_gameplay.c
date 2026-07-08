/**********************************************************************************************
*
*   raylib - Advance Game template
*
*   Gameplay Screen Functions Definitions (Init, Update, Draw, Unload)
*
*   Copyright (c) 2014-2022 Ramon Santamaria (@raysan5)
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "hex_grid.h"
#include "hex_trail.h"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

static HexGrid hexGrid = { 0 };
static HexBee bee = { 0 };
static HexTrail trail = { 0 };
static Texture2D hexTexture = { 0 };

//----------------------------------------------------------------------------------
// Animation helpers
//----------------------------------------------------------------------------------
Animation CreateAnimation(const char *filepath, float scale, int frameCnt, int speed)
{
    Animation rtn = { 0 };
    rtn.filepath = filepath;
    rtn.scale = scale;
    rtn.frameCnt = frameCnt;
    rtn.speed = speed;
    rtn.text = LoadTexture(filepath);
    rtn.frame = 0;
    rtn.countdown = speed;

    // Vertical spritesheet
    float frameH = (float)rtn.text.height/(float)rtn.frameCnt;
    float frameW = (float)rtn.text.width;
    rtn.src = (Rectangle){ 0, 0, frameW, frameH };
    rtn.dst = (Rectangle){ 0, 0, frameW*scale, frameH*scale };
    rtn.origin = (Vector2){ rtn.dst.width*0.5f, rtn.dst.height*0.5f };
    rtn.rotation = 0.0f;

    return rtn;
}

void UpdateAnimation(Animation *anim)
{
    anim->countdown--;
    if (anim->countdown <= 0)
    {
        anim->frame = (anim->frame + 1)%anim->frameCnt;
        anim->countdown = anim->speed;
    }

    float frameH = (float)anim->text.height/(float)anim->frameCnt;
    float frameW = (float)anim->text.width;
    anim->src = (Rectangle){ 0, frameH*(float)anim->frame, frameW, frameH };
}

void DrawAnimation(Animation *anim, Vector2 position)
{
    anim->dst.x = position.x;
    anim->dst.y = position.y;
    DrawTexturePro(anim->text, anim->src, anim->dst, anim->origin, anim->rotation, WHITE);
}

//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------
void InitGameplayScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;

    hexTexture = LoadTexture("resources/hexagon.png");
    HexGridInit(&hexGrid, (Vector2){ 360.0f, 360.0f }, hexTexture);

    int startVertex = HexFindLeftmostVertex(&hexGrid);
    HexBeeInit(&bee, &hexGrid, startVertex, 120.0f);
    HexTrailInit(&trail, startVertex);

    UpdateAnimation(&beeAnim);
}

void UpdateGameplayScreen(void)
{
    float dt = GetFrameTime();

    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) HexBeeSetTurn(&bee, -1);
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) HexBeeSetTurn(&bee, 1);

    HexBeeUpdate(&bee, &hexGrid, dt);

    for (int i = 0; i < bee.arrivalCount; i++)
    {
        int filled = HexTrailAdvance(&trail, &hexGrid, bee.arrivalEdges[i], bee.arrivalVerts[i]);
        if (filled > 0) PlaySound(fxCoin);
    }

    beeAnim.rotation = HexBeeRotationDeg(&bee, &hexGrid);
    UpdateAnimation(&beeAnim);

    if (IsKeyPressed(KEY_ENTER))
    {
        finishScreen = 1;
        PlaySound(fxCoin);
    }
}

void DrawGameplayScreen(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 24, 28, 36, 255 });

    HexGridDraw(&hexGrid);

    int toVertex = HexEdgeOtherVertex(&hexGrid, bee.edge, bee.fromVertex);
    if (toVertex >= 0)
    {
        DrawCircleV(hexGrid.vertices[toVertex].pos, 4.0f, (Color){ 255, 220, 80, 200 });

        int nextEdge = HexBeePeekNextEdge(&bee, &hexGrid);
        if (nextEdge >= 0)
        {
            int nextVert = HexEdgeOtherVertex(&hexGrid, nextEdge, toVertex);
            DrawLineEx(hexGrid.vertices[toVertex].pos, hexGrid.vertices[nextVert].pos, 3.0f, YELLOW);
        }
    }

    Vector2 beePos = HexBeePosition(&bee, &hexGrid);
    DrawAnimation(&beeAnim, beePos);

    DrawText("A/D turn left/right  |  ENTER end", 16, 16, 20, LIGHTGRAY);
}

void UnloadGameplayScreen(void)
{
    UnloadTexture(hexTexture);
    hexTexture = (Texture2D){ 0 };
}

int FinishGameplayScreen(void)
{
    return finishScreen;
}
