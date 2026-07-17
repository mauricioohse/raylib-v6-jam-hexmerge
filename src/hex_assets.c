/**********************************************************************************************
*
*   Beehold - Global asset bag (textures + audio)
*
**********************************************************************************************/

#include "hex_assets.h"

#include <string.h>

HexAssets assets = { 0 };

void HexAssetsLoad(void)
{
    memset(&assets, 0, sizeof(assets));

    //---- Textures ----
    assets.hexfield = LoadTexture("resources/hexfield.png");
    assets.hexpond = LoadTexture("resources/hexpond.png");
    assets.wasp = LoadTexture("resources/wasp.png");
    assets.hearts = LoadTexture("resources/hearts.png");
    assets.flower = LoadTexture("resources/flower.png");
    assets.bubbles = LoadTexture("resources/bubbles.png");
    assets.star = LoadTexture("resources/star.png");
    assets.fire = LoadTexture("resources/fire.png");
    assets.speaker = LoadTexture("resources/speaker.png");
    assets.ratingStar = LoadTexture("resources/rating_star.png");
    assets.ratingStarEmpty = LoadTexture("resources/rating_star_empty.png");
    assets.bee = LoadTexture("resources/bee.png");
    assets.iconDiscord = LoadTexture("resources/icon_discord.png");
    assets.iconX = LoadTexture("resources/icon_x.png");
    assets.iconTwitch = LoadTexture("resources/icon_twitch.png");

    SetTextureFilter(assets.bubbles, TEXTURE_FILTER_POINT);
    SetTextureFilter(assets.star, TEXTURE_FILTER_POINT);
    SetTextureFilter(assets.fire, TEXTURE_FILTER_POINT);
    SetTextureFilter(assets.speaker, TEXTURE_FILTER_POINT);
    SetTextureFilter(assets.ratingStar, TEXTURE_FILTER_POINT);
    SetTextureFilter(assets.ratingStarEmpty, TEXTURE_FILTER_POINT);
    SetTextureFilter(assets.wasp, TEXTURE_FILTER_POINT);
    SetTextureFilter(assets.flower, TEXTURE_FILTER_POINT);
    SetTextureFilter(assets.bee, TEXTURE_FILTER_POINT);
    SetTextureFilter(assets.iconDiscord, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(assets.iconX, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(assets.iconTwitch, TEXTURE_FILTER_BILINEAR);

    //---- Audio ----
    assets.music = LoadMusicStream("resources/beehold_theme.wav");
    assets.music.looping = true;
    assets.musicStarPower = LoadMusicStream("resources/star_power.wav");

    assets.fxCoin = LoadSound("resources/coin.wav");
    assets.fxFail = LoadSound("resources/fail.wav");
    assets.fxPaint = LoadSound("resources/paint.wav");
    assets.fxWin = LoadSound("resources/win.wav");
    assets.fxLife = LoadSound("resources/life.wav");
    assets.fxCheckpoint = LoadSound("resources/checkpoint.wav");
    assets.fxZoom = LoadSound("resources/zoom.wav");
    SetSoundVolume(assets.fxZoom, 0.08f);
    assets.fxBeeZoom = LoadSound("resources/bee_zoom.wav");
    SetSoundVolume(assets.fxBeeZoom, 0.18f);

    SetMusicVolume(assets.music, 0.20f);
    SetMusicVolume(assets.musicStarPower, 0.55f);
}
