#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

//sleep and time functions
#include <chrono>
#include <thread>

#include "math.h"

const int WIDTH = 320;
const int HEIGHT = 240;
const int RESOLUTION = 3; // scales up internal resolution
const int FPSLIMIT = 35; 

const int halfWIDTH = WIDTH/2;
const int halfHEIGHT = HEIGHT/2;

int windowWidth = WIDTH*RESOLUTION, windowHeight = HEIGHT*RESOLUTION;

//fps limit clock
using steadyClock = std::chrono::steady_clock;
auto nextFrame = steadyClock::now();

float fpsCap = 1.0 / (float)FPSLIMIT;
auto frameTime = std::chrono::duration_cast<steadyClock::duration>(std::chrono::duration<float>(fpsCap));
auto frameMargin = std::chrono::microseconds(500);

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

typedef struct
{
    float prevTime, currTime;
    float deltaTime;
} Time; Time timer;

struct sector
{
    float floor, ceil;
    Vector2 *vertex; //vertex array
    unsigned int numPoints; //amount of vertices in sector
    short *neighbors;
}; //*sectors = nullptr;
unsigned int numSectors = 2;

//TODO: LOAD SECTORS FROM FILE
sector* sectors = new sector[] {
    { //1st sector
        -10, //floor
        30, //ceiling
        new Vector2[] {
            Vector2(-70,70), //must be done clockwise for normals to not be inverted
            Vector2(-70, -70),
            Vector2(70,-70),
            Vector2(70,70),
            Vector2(-70, 70)
        },
        4, //amount of walls in the sector
        new short[] {
            -1, //-1 is a normal wall
            -1,
            -1,
            1 //1 means this wall connects to that index of a sector
        }
    }, //2nd sector
        {
        -30, //floor
        50, //ceiling
        new Vector2[] {
            Vector2(-70,70),
            Vector2(70, 70),
            Vector2(140, 210),
            Vector2(70,350),
            Vector2(-70,350),
            Vector2(-140, 210),
            Vector2(-70, 70)
        },
        6, //amount of walls in the sector
        new short[] {
            0, //0 means this wall connects to that index of a sector
            -1, //-1 is a normal wall
            -1,
            -1,
            -1,
            -1
        }
    }
};

float getCurrentTime();
void setDeltaTime();
void playerWallCollision(Vector2 &wishDir, short prevSect = -1);
void playerMovement();
void processInput(GLFWwindow *window);
void fpsLimit();
void pixel(int x,int y, Vector3 c);
void drawLine(Vector2 a, Vector2 b, Vector3 color);
void clearBackground();
void init();

void draw2D(sector* sect);
void draw3D(sector* sectors);
void display(GLFWwindow *window);

void windowScaling(int width, int height);

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    windowWidth = width;
    windowHeight = height;
    //windowScaling(width, height);
    //glViewport(0, 0, width, height);
}

int main()
{
    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    //set to opengl 2.1
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow* window;
    window = glfwCreateWindow(WIDTH*RESOLUTION, HEIGHT*RESOLUTION, "DOOM", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to open GLFW window" << std::endl;
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    
    glViewport(0, 0, WIDTH*RESOLUTION, HEIGHT*RESOLUTION);
    //glfwSwapInterval(1); // Enable vsync (limits fps to monitor refresh rate, dont enable for fps limit)

    //TODO check if GL_ARB_framebuffer_object is supported and disable dynamic window scaling if not
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glPointSize(RESOLUTION); //pixel size
    glOrtho(0, WIDTH*RESOLUTION, 0, HEIGHT*RESOLUTION, -1, 1); // set the origin at bottom-left corner

    //glEnable(GL_TEXTURE_2D);

    init();
    
    while(!glfwWindowShouldClose(window))
    {
        //main loop

        setDeltaTime();
        processInput(window);

        //glOrtho(0, WIDTH*RESOLUTION, 0, HEIGHT*RESOLUTION, -1, 1); //transformation for drawing pixels
        display(window);

        //window scaling tests
        /*glLoadIdentity(); //transformation for drawing quads
        glBegin(GL_QUADS);
            glVertex2f(-0.5f, -0.5f);
            glVertex2f(0.5f, -0.5f);
            glVertex2f(0.5f, 0.5f);
            glVertex2f(-0.5f, 0.5f);
        glEnd();
        windowScaling(windowWidth, windowHeight);*/

        glfwPollEvents();

        fpsLimit(); //the entire main loop will run at 35 fps or FPSLIMIT
    }

    glfwTerminate();
    return 0;
}

void init() {
    //init deltatime
    timer.prevTime = getCurrentTime();

    player.position = Vector3(50,50,20);
    player.rotation = 0;
    player.sector = 0;
}

void display(GLFWwindow *window) {

    clearBackground();

    
    if (inputPressed.showmap)
    draw2D(&sectors[player.sector]);
    else
    draw3D(sectors);


    glfwSwapBuffers(window);

}

void draw3D(sector* sectors) {
    float dx = std::sin(degToRad(player.rotation));
    float dy = std::cos(degToRad(player.rotation));

    //queue for rendering each onscreen neighboring sectors
    enum {MaxQueue = 32};
    struct itemQueue {int sector; float leftWIDTH, rightWIDTH;} queue[MaxQueue], *head=queue, *tail=queue;
    *head = (struct itemQueue) {player.sector, 0, WIDTH}; //start with first sector to render and the entire screen width
    if (++head == queue+MaxQueue) head = queue;

    /*keep track of remaining amount of window size to render for on screen neighboring sectors (starts at full window size and get smaller and smaller for each neighboring sector until completed)*/
    int topHEIGHT[WIDTH], bottomHEIGHT[WIDTH]={0};
    for (int x=0; x<WIDTH; x++) topHEIGHT[x] = HEIGHT;

    //keep track of already rendered sectors
    short renderedSectors[numSectors];
    for (int x=0; x<numSectors; x++) renderedSectors[x] = -1;

    do {
    //queue tail
    const struct itemQueue currentItem = *tail;
    if (++tail == queue+MaxQueue) tail = queue;

    //left to right width amount for rendering and avoiding overdraw
    int leftWIDTH = currentItem.leftWIDTH;
    int rightWIDTH = currentItem.rightWIDTH;
        
    sector* sect = &sectors[currentItem.sector]; //current sector being rendered
    for (int p=0; p < sect->numPoints; p++) //for each wall in the sector
    {

    Vector2 wallpos1 = sect->vertex[p+0];
    Vector2 wallpos2 = sect->vertex[p+1];

    //change wall points from world position to being relative to the player's position
    Vector2 v1(wallpos1.x - player.position.x, wallpos1.y - player.position.y);
    Vector2 v2(wallpos2.x - player.position.x, wallpos2.y - player.position.y);

    //rotation transformation
    float rot = player.rotation-90;
    
    float r0z = v1.x * std::cos(degToRad(rot)) - v1.y * std::sin(degToRad(rot)); //z axis is forward for the player
    float r0x = v1.x * std::sin(degToRad(rot)) + v1.y * std::cos(degToRad(rot)); //x axis is left and right for the player
    float r1z = v2.x * std::cos(degToRad(rot)) - v2.y * std::sin(degToRad(rot));
    float r1x = v2.x * std::sin(degToRad(rot)) + v2.y * std::cos(degToRad(rot));

    //clip walls behind player
    if (r0z <= 0 && r1z <= 0) continue;

    //if only one side of the wall isn't visible on screen clip the nonvisible part to avoid issues
    float nearz = 0.1;
    if (r0z <= 0) {
        float t = (nearz - r0z) / (r1z - r0z);
        float newx = r0x + t * (r1x - r0x);
        r0z = nearz;
        r0x = newx;
    }
    if (r1z <= 0) {
        float t = (nearz - r1z) / (r0z - r1z);
        float newx = r1x + t * (r0x - r1x);
        r1z = nearz;
        r1x = newx;
    }

    //perspective transformation
    int fov = 90;
    float focal_length = halfWIDTH/(std::tan(degToRad(fov)/2));


    float x0 = halfWIDTH - focal_length * r0x / r0z; //left wall side
    float x1 = halfWIDTH - focal_length * r1x / r1z; //right wall side

    //ceiling and floor
    int ceilingHeight = sect->ceil;
    int floorHeight = sect->floor;

    float top0 = halfHEIGHT + focal_length * (ceilingHeight - player.position.z) / r0z;
    float bottom0 = halfHEIGHT + focal_length * (floorHeight - player.position.z) / r0z;
    float top1 = halfHEIGHT + focal_length * (ceilingHeight - player.position.z) / r1z;
    float bottom1 = halfHEIGHT + focal_length * (floorHeight - player.position.z) / r1z;

    //top and bottom part of neighbor wall (ignored if normal wall)
    float nCeilingHeight = 0, nFloorHeight = 0;
    float nTop0, nTop1, nBottom0, nBottom1;

    short neighbor = sect->neighbors[p];
    if (neighbor >= 0 && renderedSectors[neighbor] < 0)
    {
        nCeilingHeight = sectors[neighbor].ceil;
        nFloorHeight = sectors[neighbor].floor;

        nTop0 = halfHEIGHT + focal_length * (nCeilingHeight - player.position.z) / r0z;
        nBottom0 = halfHEIGHT + focal_length * (nFloorHeight - player.position.z) / r0z;
        nTop1 = halfHEIGHT + focal_length * (nCeilingHeight - player.position.z) / r1z;
        nBottom1 = halfHEIGHT + focal_length * (nFloorHeight - player.position.z) / r1z;
    }

    //clip values into screen space or return if not on screen
    float xleft, xright;
    if (x0 > x1)
    {
        swap(x0, x1);

        if (x0 >= rightWIDTH || x1 < leftWIDTH)
        continue;
    
        xleft = max(x0, leftWIDTH);
        xright = min(x1, rightWIDTH);

        swap(x0, x1);
    }
    else
    {
        if (x0 >= rightWIDTH || x1 < leftWIDTH)
        continue;
    
        xleft = max(x0, leftWIDTH);
        xright = min(x1, rightWIDTH);
    }

    if (bottom0 >= HEIGHT && bottom1 >= HEIGHT)
    continue;

    if (top0 < 0 && top1 < 0)
    continue;
    
    //debug wall drawing
    //drawLine(Vector2(x0, top0), Vector2(x1, top1), Vector3(0, 255, 0));
    //drawLine(Vector2(x0, bottom0), Vector2(x1, bottom1), Vector3(0, 255, 255));

    //drawLine(Vector2(x0, top0), Vector2(x0, bottom0), Vector3(255, 0, 0));
    //drawLine(Vector2(x1, top1), Vector2(x1, bottom1), Vector3(0, 0, 255));

    //draw wall by rendering columns
    for (float x = xleft; x <= xright; x++) {
        float t, topy, bottomy;
        
        //use lerp to find the correct x value between x0 and x1 for the column on screen
        t = (x-x0) / (x1-x0);
        topy = top0 + t * (top1 - top0);
        bottomy = bottom0 + t * (bottom1 - bottom0);

        //clip y values to screen space once transformed
        if (bottomy >= topHEIGHT[(int)x] || topy < bottomHEIGHT[(int)x])
        continue;

        topy = min(topy, topHEIGHT[(int)x]);
        bottomy = max(bottomy, bottomHEIGHT[(int)x]);
        
        //finally render the columns
        if (neighbor >= 0 && renderedSectors[neighbor] < 0)
        {

            float nTopy = nTop0 + t * (nTop1 - nTop0);
            float nBottomy = nBottom0 + t * (nBottom1 - nBottom0);

            //set screen height for rendering the neighbor sector
            if (bottomy < nBottomy)
                bottomHEIGHT[(int)x] = nBottomy;
            else
                bottomHEIGHT[(int)x] = bottomy;

            if (topy > nTopy)
                topHEIGHT[(int)x] = nTopy;
            else
                topHEIGHT[(int)x] = topy;

            //render top and bottom part of the neighbor wall
            for (float y = bottomy; y <= topy; y++)
            {
                //TODO: inefficient way of rendering make separate rendering loops for top and bottom walls
                if (bottomy < nBottomy && y <= nBottomy)
                {
                    //if theres a bottom wall part render it
                    pixel(x, y, Vector3(255,0,255));
                }
                else if (topy > nTopy && y >= nTopy)
                {
                    //if theres a top wall part render it
                    pixel(x, y, Vector3(255,255,0));
                }
            }
        }
        else if (neighbor < 0)
        {
            //render normal wall
            for (float y = bottomy; y <= topy; y++)
            {
                pixel(x, y, Vector3(255,255,0));
            }
        }
    }

    if (neighbor >= 0 && xleft <= xright && renderedSectors[neighbor] < 0)
    {
        renderedSectors[currentItem.sector] = currentItem.sector;
        //queue increases if theres a neighbor on screen that wasn't visited before
        *head = (struct itemQueue) {neighbor, xleft, xright};
        if (++head == queue+MaxQueue) head = queue;
    }

    } // end of loop for each wall in currently rendered sector

    } while(head != tail); //render other neighboring sectors
}

void draw2D(sector* sect) {
    float dx = std::sin(degToRad(player.rotation));
    float dy = std::cos(degToRad(player.rotation));
    
    //player
    pixel(halfWIDTH, halfHEIGHT, Vector3(255, 0, 0));
    pixel(halfWIDTH, halfHEIGHT+2, Vector3(255, 0, 0));

    for (int p=0; p<sect->numPoints; p++) //for each wall in the sector
    {
    Vector2 wallpos1 = sect->vertex[p+0];
    Vector2 wallpos2 = sect->vertex[p+1];

    //transformations
    Vector2 v1(wallpos1.x - player.position.x, wallpos1.y - player.position.y);
    Vector2 v2(wallpos2.x - player.position.x, wallpos2.y - player.position.y);

    //rotate
    float rot = player.rotation;
    
    float r0z = v1.x * std::cos(degToRad(rot)) - v1.y * std::sin(degToRad(rot)); //z axis is forward for the player
    float r0x = v1.x * std::sin(degToRad(rot)) + v1.y * std::cos(degToRad(rot)); //x axis is left and right for the player
    float r1z = v2.x * std::cos(degToRad(rot)) - v2.y * std::sin(degToRad(rot));
    float r1x = v2.x * std::sin(degToRad(rot)) + v2.y * std::cos(degToRad(rot));

    //apply normal wall or neighboring sector color
    Vector3 color;
    short neighbor = sect->neighbors[p];
    if (neighbor >= 0)
    {
        //neighboring sector wall
        color = Vector3(255, 0, 0);
    }
    else
    {
        //normal wall
        color = Vector3(0, 255, 0);
    }

    //draw wall
    drawLine(Vector2(r0z+halfWIDTH, r0x+halfHEIGHT), Vector2(r1z+halfWIDTH, r1x+halfHEIGHT), color);
    pixel(r0z+halfWIDTH, r0x+halfHEIGHT, Vector3(255, 0, 0));
    pixel(r1z+halfWIDTH, r1x+halfHEIGHT, Vector3(0, 0, 255));
    }
}

void playerMovement()
{
    if (inputPressed.a) {
        //rotate left
        player.rotation -= 1 * 200 * timer.deltaTime;
        if (player.rotation < 0) player.rotation += 360;
        //std::cout << "player angle: " << player.rotation << std::endl;
    }
    if (inputPressed.d) {
        //rotate right
        player.rotation += 1 * 200 * timer.deltaTime;
        if (player.rotation > 359) player.rotation -= 360;
        //std::cout << "player angle: " << player.rotation << std::endl;
    }

    float dx = std::sin(degToRad(player.rotation)) * timer.deltaTime * 50;
    float dy = std::cos(degToRad(player.rotation)) * timer.deltaTime * 50;
    if (inputPressed.w) {
        //move forward

        Vector2 wishDir(dx,dy);

        //modify wishDir position if it hits a wall to never go past a wall
        playerWallCollision(wishDir);

        //move the player
        player.position.x += wishDir.x;
        player.position.y += wishDir.y;
    }
    if (inputPressed.s) {
        //move backward

        Vector2 wishDir(-dx,-dy);

        //modify wishDir position if it hits a wall to never go past a wall
        playerWallCollision(wishDir);

        //move the player
        player.position.x += wishDir.x;
        player.position.y += wishDir.y;
    }

    sector* sect = &sectors[player.sector];
    if (inputPressed.space) {
        //fly up
        float ceiling = sect->ceil;
        float wishHeight = 1;

        //player cant go higher than ceiling
        if (player.position.z + wishHeight > ceiling)
        player.position.z = ceiling;
        else
        player.position.z += wishHeight;
    }
    if (inputPressed.shift) {
        //fly down
        float floor = sect->floor;
        float wishHeight = 1;

        //player cant go lower than floor
        if (player.position.z - wishHeight < floor)
        player.position.z = floor;
        else
        player.position.z -= wishHeight;
    }
}

void playerWallCollision(Vector2 &wishDir, short prevSect)
{
    //wall collision detection
    sector* sect = &sectors[player.sector];
    for (int p = 0; p < sect->numPoints; p++)
    {
        Vector2 a = sect->vertex[p+0];
        Vector2 b = sect->vertex[p+1];

        Vector2 wall(b.x - a.x, b.y - a.y);
        Vector2 plyPos(player.position.x - a.x, player.position.y - a.y);
        Vector2 wishPos((player.position.x + wishDir.x) - a.x, (player.position.y + wishDir.y) - a.y);

        float currentSide = crossProduct(wall, plyPos);
        float wishSide = crossProduct(wall, wishPos);

        if (currentSide > 0 && wishSide < 0 || currentSide < 0 && wishSide > 0)
        {
            //if player passed the wall

            short neighbor = sect->neighbors[p];
            if (neighbor >= 0 && neighbor != prevSect && 
                player.position.z >= sectors[neighbor].floor && player.position.z <= sectors[neighbor].ceil)
            {
                //if player crossed a neighboring sector and didn't hit any lower or upper wall part then change the rendering sector to the neighbor
                prevSect = player.sector;
                player.sector = neighbor;

                //check for collisions again in the new sector
                playerWallCollision(wishDir, prevSect);
            }
            else
            {
                //if player crossed a wall


                //calculate wall's normal
                Vector2 n(b.x - a.x, b.y - a.y);

                //rotate 90 degrees
                float temp = n.x;
                n.x = -n.y;
                n.y = temp;

                //scale values only to 0-1
                float len = std::sqrt(n.x*n.x+n.y*n.y);
                n.x /= len;
                n.y /= len;

                //how similar or opposite wishDir and wall normal's direction are
                float vn = dotProduct(wishDir, n);

                if (vn < 0)
                {
                    //if wishdir is on opposite side of wall normal

                    //limit movement only within the wall
                    wishDir.x -= n.x * vn;
                    wishDir.y -= n.y * vn;
                }
                
                //wishDir.x = 0;
                //wishDir.y = 0;
            }
        }
    }
}

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    inputPressed.w = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    inputPressed.a = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    inputPressed.s = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    inputPressed.d = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    inputPressed.space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    inputPressed.shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    inputPressed.showmap = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;

    playerMovement();
}

void pixel(int x,int y, Vector3 c)
{ 
    glColor3ub(c.x, c.y, c.z); 
    glBegin(GL_POINTS);
    glVertex2f(float(x*RESOLUTION)+RESOLUTION*0.5, float(y*RESOLUTION)+RESOLUTION*0.5); //offset by half a pixel because opengl doesnt position pixels correctly
    glEnd();
}

void drawLine(Vector2 p0, Vector2 p1, Vector3 color)
{
    //bresenham line algorithm implementation

    //pixel(p0.x, p0.y, Vector3(0, 255, 0));
    //pixel(p1.x, p1.y, Vector3(0, 255, 0));

    int dx = std::abs(p1.x - p0.x);
    int dy = std::abs(p1.y - p0.y);

    int sx = (p0.x < p1.x) ? 1 : -1;
    int sy = (p0.y < p1.y) ? 1 : -1;

    int err = dx - dy;

    int steps = (dx > dy) ? dx : dy;

    for (int i = 0; i <= steps; i++) {
        pixel(p0.x, p0.y, color);
        //std::cout << "p: " << x << ", " << y << std::endl;

        if (p0.x == p1.x && p0.y == p1.y)
            break;
        
        int e2 = 2 * err;

        if (e2 > -dy)
        {
            err -= dy;
            p0.x += sx;
        }
        
        if (e2 < dx)
        {
            err += dx;
            p0.y += sy;
        }
    }
}

void clearBackground()
{
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            pixel(x, y, Vector3(10,10,100));
        }
    }
}

void windowScaling(int width, int height) { //over 3 hours have been wasted on trying to make this work just dont bother
    glViewport(0, 0, WIDTH*RESOLUTION, HEIGHT*RESOLUTION);

    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH*RESOLUTION, HEIGHT*RESOLUTION, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    glViewport(0, 0, width, height);

    glDisable(GL_DEPTH_TEST); // disable depth testing for 2D rendering

    glLoadIdentity(); //transformation for drawing quads
    /*glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
    glEnd();*/
    
    glColor3ub(255, 0, 0); 
    glBegin(GL_QUADS);
        glVertex2f(-1.0f, -1.0f);
        glVertex2f(1.0f, -1.0f);
        glVertex2f(1.0f, 1.0f);
        glVertex2f(-1.0f, 1.0f);
    glEnd();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void setDeltaTime() {
    timer.currTime = getCurrentTime();
    timer.deltaTime = timer.currTime - timer.prevTime;

    timer.prevTime = timer.currTime;
    //std::cout << "deltatime: " << timer.deltaTime << std::endl;
}

float getCurrentTime() {
    //function made to easily change this with a different solution
    return glfwGetTime();
}

void fpsLimit() {
    //fps limit
    nextFrame += frameTime;

    auto sleepUntil = nextFrame - frameMargin;
    if (steadyClock::now() < sleepUntil)
    {
        std::this_thread::sleep_until(sleepUntil);
    }

    //busy wait for last 500ms since sleep_until wakes up slightly delayed instead of on time
    while (steadyClock::now() < nextFrame)
    {
        std::this_thread::yield();
    }
}