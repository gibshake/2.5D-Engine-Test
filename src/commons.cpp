#include "commons.h"

//constants
const int WIDTH = 320;
const int HEIGHT = 240;
const int RESOLUTION = 3; // only exists to scale up window size
const int FPSLIMIT = 35; 

const int halfWIDTH = WIDTH/2;
const int halfHEIGHT = HEIGHT/2;

int windowWidth = WIDTH*RESOLUTION, windowHeight = HEIGHT*RESOLUTION;

unsigned char framebuffer[WIDTH*HEIGHT*4];

Time timer;
Input inputPressed;
//typing input
std::string msg = "";

Player player;
int fov = 90; //player fov

unsigned int numSectors = 0;
