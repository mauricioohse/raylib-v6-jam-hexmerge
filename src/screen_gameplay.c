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
#include "hex_enemy.h"
#include "hex_flower.h"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
#define ENEMY_COUNT 3
#define PLAYER_MAX_LIVES 3
#define HIT_RADIUS 14.0f
#define HEART_SCALE 2.0f
#define LEVEL_SEED_COUNT 3

static int framesCounter = 0;
static int finishScreen = 0;

static HexGrid hexGrid = { 0 };
static HexBee bee = { 0 };
static HexTrail trail = { 0 };
static HexEnemy enemies[ENEMY_COUNT] = { 0 };
static HexFlowerField flowers = { 0 };
static Texture2D hexTexture = { 0 };
static Texture2D spiderTexture = { 0 };
static Texture2D heartsTexture = { 0 };
static Texture2D flowerTexture = { 0 };
static int lives = PLAYER_MAX_LIVES;

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
// Round / lives helpers
//----------------------------------------------------------------------------------
static void ResetRound(void)
{
    HexGridInit(&hexGrid, (Vector2){ 360.0f, 360.0f }, hexTexture);

    int startVertex = HexFindLeftmostVertex(&hexGrid);
    HexBeeInit(&bee, &hexGrid, startVertex, 120.0f);
    HexTrailInit(&trail, startVertex);

    // Three seeds spread across the board (not on the bee's leftmost start face)
    int seedFaces[LEVEL_SEED_COUNT];
    seedFaces[0] = HexFindFace(&hexGrid, 2, -1);
    seedFaces[1] = HexFindFace(&hexGrid, -1, 2);
    seedFaces[2] = HexFindFace(&hexGrid, 0, -2);
    for (int i = 0; i < LEVEL_SEED_COUNT; i++)
    {
        if (seedFaces[i] < 0) seedFaces[i] = (i + 1)%hexGrid.faceCount;
    }
    HexFlowerFieldInit(&flowers, flowerTexture, seedFaces, LEVEL_SEED_COUNT);

    // Spawn spiders spread out, away from the bee's leftmost start
    int rightmost = 0, topmost = 0, bottommost = 0;
    for (int i = 1; i < hexGrid.vertexCount; i++)
    {
        if (hexGrid.vertices[i].pos.x > hexGrid.vertices[rightmost].pos.x) rightmost = i;
        if (hexGrid.vertices[i].pos.y < hexGrid.vertices[topmost].pos.y) topmost = i;
        if (hexGrid.vertices[i].pos.y > hexGrid.vertices[bottommost].pos.y) bottommost = i;
    }
    HexEnemyInit(&enemies[0], HEX_ENEMY_RED_RANDOM, &hexGrid, rightmost, 120.0f);
    HexEnemyInit(&enemies[1], HEX_ENEMY_PURPLE_CHASER, &hexGrid, topmost, 120.0f);
    HexEnemyInit(&enemies[2], HEX_ENEMY_GREEN_MIXED, &hexGrid, bottommost, 120.0f);

    beeAnim.rotation = HexBeeRotationDeg(&bee, &hexGrid);
    UpdateAnimation(&beeAnim);
}

static void DrawLives(void)
{
    float frameW = (float)heartsTexture.width;
    float frameH = (float)heartsTexture.height/2.0f;
    float drawW = frameW*HEART_SCALE;
    float drawH = frameH*HEART_SCALE;
    float gap = 4.0f;
    float totalW = PLAYER_MAX_LIVES*drawW + (PLAYER_MAX_LIVES - 1)*gap;
    float x0 = (float)GetScreenWidth() - 16.0f - totalW;
    float y = 16.0f;

    for (int i = 0; i < PLAYER_MAX_LIVES; i++)
    {
        int frame = (i < lives)? 0 : 1;     // 0 = full, 1 = empty/grey
        Rectangle src = { 0, frameH*(float)frame, frameW, frameH };
        Rectangle dst = { x0 + (float)i*(drawW + gap), y, drawW, drawH };
        DrawTexturePro(heartsTexture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
    }
}

//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------
void InitGameplayScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;
    lives = PLAYER_MAX_LIVES;

    hexTexture = LoadTexture("resources/hexfield.png");
    spiderTexture = LoadTexture("resources/spider.png");
    heartsTexture = LoadTexture("resources/hearts.png");
    flowerTexture = LoadTexture("resources/flower.png");

    ResetRound();
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
        if (filled > 0)
        {
            PlaySound(fxCoin);
            HexFlowerFieldOnFill(&flowers, &hexGrid);
        }
    }

    Vector2 beePos = HexBeePosition(&bee, &hexGrid);
    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        HexEnemyUpdate(&enemies[i], &hexGrid, beePos, dt);
    }

    HexFlowerFieldUpdate(&flowers, dt);

    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        if (HexEnemyTouches(&enemies[i], &hexGrid, beePos, HIT_RADIUS))
        {
            lives--;
            if (lives <= 0) lives = PLAYER_MAX_LIVES;   // all lives lost: restart from the beginning
            ResetRound();
            PlaySound(fxCoin);
            break;
        }
    }

    beeAnim.rotation = HexBeeRotationDeg(&bee, &hexGrid);
    UpdateAnimation(&beeAnim);

    if (HexFlowerFieldWon(&flowers, &hexGrid))
    {
        finishScreen = 1;
        PlaySound(fxCoin);
    }

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
    HexFlowerFieldDraw(&flowers, &hexGrid);

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

    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        HexEnemyDraw(&enemies[i], &hexGrid, spiderTexture);
    }

    Vector2 beePos = HexBeePosition(&bee, &hexGrid);
    DrawAnimation(&beeAnim, beePos);

    DrawLives();
    DrawText("A/D turn left/right  |  ENTER end", 16, 16, 20, LIGHTGRAY);
}

void UnloadGameplayScreen(void)
{
    UnloadTexture(hexTexture);
    hexTexture = (Texture2D){ 0 };
    UnloadTexture(spiderTexture);
    spiderTexture = (Texture2D){ 0 };
    UnloadTexture(heartsTexture);
    heartsTexture = (Texture2D){ 0 };
    UnloadTexture(flowerTexture);
    flowerTexture = (Texture2D){ 0 };
}

int FinishGameplayScreen(void)
{
    return finishScreen;
}
