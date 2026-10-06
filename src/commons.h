/*
    commons.h - all global or commonly used includes, variables, functions are found here
*/

#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>

//sleep and time functions
#include <chrono>
#include <thread>

#include <string>
#include <string_view>
#include <charconv>

//#include <filesystem> //only used to show file path location

#include "math.h"

//constants
const int WIDTH = 320;
const int HEIGHT = 240;
const int RESOLUTION = 3; // only exists to scale up window size
const int FPSLIMIT = 35; 

const int halfWIDTH = WIDTH/2;
const int halfHEIGHT = HEIGHT/2;

int windowWidth = WIDTH*RESOLUTION, windowHeight = HEIGHT*RESOLUTION;

unsigned char framebuffer[WIDTH*HEIGHT*4];

//structs
typedef struct
{
    float prevTime, currTime;
    float deltaTime;
} Time; Time timer;

typedef struct 
{
    bool w,a,s,d;
    bool space;
    bool shift;
    bool showmap;
} Input; Input inputPressed;

typedef struct 
{
    Vector3 position;
    float rotation;
    int sector; //current sector the player starts in
} Player; Player player;

struct sector
{
    float floor, ceil;
    Vector2 *vertex; //vertex array
    unsigned int numPoints; //amount of vertices in sector
    short *neighbors;
}; //*sectors = nullptr;
unsigned int numSectors = 0;