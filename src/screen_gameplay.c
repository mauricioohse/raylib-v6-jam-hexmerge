/**********************************************************************************************
*
*   raylib - Advance Game template
*
*   Gameplay Screen Functions Definitions (Init, Update, Draw, Unload)
*
*   Copyright (c) 2014-2022 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;



//----------------------------------------------------------------------------------
// My render/draw Functions Definition
//----------------------------------------------------------------------------------

Animation CreateAnimation(const char* filepath, float scale, int frameCnt, int speed)
{
  Animation rtn = {0};
  rtn.filepath =  filepath;
  rtn.scale = scale;
  rtn.frameCnt = frameCnt;
  rtn.speed = speed;
  rtn.text = LoadTexture(filepath);

  // note: assumes vertical spritesheet
  rtn.src = (Rectangle){0,0,rtn.text.width, (float)rtn.text.height/(rtn.frameCnt)};
  rtn.dst = (Rectangle){0, 0, rtn.scale * rtn.text.width,
                        (float)rtn.scale * rtn.text.height / rtn.frameCnt};

  return rtn;
}

// should be called every frame, assumes 60 frames per second
void UpdateAnimation(Animation* anim)
{
    anim->countdown--;
    if(anim->countdown <=0)
    {
      anim->frame = (anim->frame + 1) % (anim->frameCnt+1);
      anim->countdown=anim->speed;
    }

    // currently assumes the spritesheet is a vertical one
    anim->src = (Rectangle){
        0, (float)anim->text.height * anim->frame / anim->frameCnt,
        (float)anim->text.height / anim->frameCnt, anim->text.width};
}


void DrawAnimation(Animation* anim, Vector2 position)
{
  DrawTexturePro(anim->text, anim->src, anim->dst, anim->origin,
                 anim->rotation, WHITE);
}


//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    // TODO: Initialize GAMEPLAY screen variables here!
    framesCounter = 0;
    finishScreen = 0;

    // bee sprite init
    
    UpdateAnimation(&beeAnim);
    
}

// Gameplay Screen Update logic
void UpdateGameplayScreen(void)
{
    // TODO: Update GAMEPLAY screen variables here!

    // Press enter or tap to change to ENDING screen
    if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
    {
        finishScreen = 1;
        PlaySound(fxCoin);
    }

    UpdateAnimation(&beeAnim);
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    // TODO: Draw GAMEPLAY screen here!
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), PURPLE);
    Vector2 pos = { 20, 10 };
    DrawTextEx(font, "GAMEPLAY SCREEN", pos, font.baseSize*3.0f, 4, MAROON);
    DrawText("PRESS ENTER or TAP to JUMP to ENDING SCREEN", 130, 220, 20, MAROON);

    DrawAnimation(&beeAnim, (Vector2){0.0f,0.0f});
}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    // TODO: Unload GAMEPLAY screen variables here!
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}