/**********************************************************************************************
*
*   raylib - Advance Game template
*
*   Screens Functions Declarations (Init, Update, Draw, Unload)
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

#ifndef SCREENS_H
#define SCREENS_H

#include "raylib.h"
#include "hex_scores.h"

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum GameScreen {
  UNKNOWN = -1,
  TITLE = 0,
  OPTIONS,
  GAMEPLAY,
  ENDING,
#if defined(_DEBUG)
  COVER,
#endif
} GameScreen;

typedef struct
{
    Texture2D text;
    Rectangle src;
    Rectangle dst;
    float scale;
    float rotation;
    Vector2 origin;
    int frame;     // goes from 0 to frameCnt - 1
    int frameCnt;  
    int countdown; // measured in remaining frames before next animation frames
    int speed;     // Number of spritesheet frames shown by second
} Animation;

Animation CreateAnimation(Texture2D texture, float scale, int frameCnt, int speed);
void UpdateAnimation(Animation* anim);
void DrawAnimation(Animation* animation, Vector2 position);
void DrawAnimationTint(Animation* animation, Vector2 position, Color tint);

//----------------------------------------------------------------------------------
// Global Variables Declaration (shared by several modules)
//----------------------------------------------------------------------------------
extern GameScreen currentScreen;
extern Font font;
extern Animation beeAnim;
void PlayWaspZoom(void);    // quiet overlapping buzz when a wasp hops
void PlayBeeZoom(void);     // subtle hop buzz when the bee launches
extern int volumeLevel;     // 0..10, default 5; drives SetMasterVolume
extern float lastRunTime;   // total seconds for the run that just finished
extern HexRunResult lastRun; // per-level times for ending screen
extern bool controllerMode; // title → gameplay: true = A/D relative, false = WASD absolute
extern bool endingFromMenu; // title → ending: show best run instead of last run

#ifdef __cplusplus
extern "C" {            // Prevents name mangling of functions
#endif

// Request a fade transition to another screen (call from screen Update).
void TransitionToScreen(GameScreen screen);

//----------------------------------------------------------------------------------
// Logo Screen Functions Declaration
//----------------------------------------------------------------------------------
void InitLogoScreen(void);
void UpdateLogoScreen(void);
void DrawLogoScreen(void);
void UnloadLogoScreen(void);

//----------------------------------------------------------------------------------
// Title Screen Functions Declaration
//----------------------------------------------------------------------------------
void InitTitleScreen(void);
void UpdateTitleScreen(void);
void DrawTitleScreen(void);
void UnloadTitleScreen(void);

//----------------------------------------------------------------------------------
// Options Screen Functions Declaration
//----------------------------------------------------------------------------------
void InitOptionsScreen(void);
void UpdateOptionsScreen(void);
void DrawOptionsScreen(void);
void UnloadOptionsScreen(void);

//----------------------------------------------------------------------------------
// Gameplay Screen Functions Declaration
//----------------------------------------------------------------------------------
void InitGameplayScreen(void);
void UpdateGameplayScreen(void);
void DrawGameplayScreen(void);
void UnloadGameplayScreen(void);

//----------------------------------------------------------------------------------
// Ending Screen Functions Declaration
//----------------------------------------------------------------------------------
void InitEndingScreen(void);
void UpdateEndingScreen(void);
void DrawEndingScreen(void);
void UnloadEndingScreen(void);

#if defined(_DEBUG)
//----------------------------------------------------------------------------------
// Cover studio (DEBUG only) — pose + export itch cover
//----------------------------------------------------------------------------------
void InitCoverScreen(void);
void UpdateCoverScreen(void);
void DrawCoverScreen(void);
void UnloadCoverScreen(void);
#endif

#ifdef __cplusplus
}
#endif

#endif // SCREENS_H