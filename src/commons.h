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
extern const int WIDTH;
extern const int HEIGHT;
extern const int RESOLUTION; // only exists to scale up window size
extern const int FPSLIMIT; 

extern const int halfWIDTH;
extern const int halfHEIGHT;

extern int windowWidth, windowHeight;

extern unsigned char framebuffer[];

//structs
typedef struct
{
    float prevTime, currTime;
    float deltaTime;
} Time;
extern Time timer;

typedef struct 
{
    bool w,a,s,d;
    bool space;
    bool shift;
    bool showmap;
    bool typing = false; //current player state rather than an actual input
} Input;
extern Input inputPressed;
//typing input
extern std::string msg;

typedef struct 
{
    Vector3 position;
    float rotation;
    int sector; //current sector the player starts in
} Player;
extern Player player;
extern int fov;

struct sector
{
    float floor, ceil;
    Vector2 *vertex; //vertex array
    unsigned int numPoints; //amount of vertices in sector
    short *neighbors;
}; //*sectors = nullptr;
extern unsigned int numSectors;

struct image {
    int width, height;
    unsigned char *pixelData;
};