/*
    render.h - rendering functions to display any element of the game
    includes rendering the levels in first person or as a 2d map, along with text or hud elements
*/

#pragma once

#include "commons.h"

void pixel(int x,int y, Vector3 c);
void drawLine(Vector2 a, Vector2 b, Vector3 color);
void drawImage(int xOffset, int yOffset, image &img);
void drawText(std::string_view text, int x, int y, image *src);
void clearBackground();

void draw2D(sector* sect);
void draw3D(sector* sectors);