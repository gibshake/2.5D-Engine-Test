#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "math.h"

const int WIDTH = 320;
const int HEIGHT = 240;
const int RESOLUTION = 3; // scales up internal resolution

const int halfWIDTH = WIDTH/2;
const int halfHEIGHT = HEIGHT/2;

int windowWidth = WIDTH*RESOLUTION, windowHeight = HEIGHT*RESOLUTION;

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
    float verticalAngle;
} Player; Player player;

//TEMPORARY; REMOVE SOON
Vector2 wallpos1(70,30);
Vector2 wallpos2(70,70);

typedef struct
{
    float prevTime, currTime;
    float deltaTime;
} Time; Time timer;

void setDeltaTime();
void processInput(GLFWwindow *window);
void pixel(int x,int y, Vector3 c);
void drawLine(Vector2 a, Vector2 b, Vector3 color);
void clearBackground();
void init();

void drawWall(float x1, float x2, float y1, float y2, float h1, float h2);
void draw2D();
void draw3D();
void display();

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
    glfwSwapInterval(1); // Enable vsync (without it cpu usage goes up to 50% for some reason)

    //TODO check if GL_ARB_framebuffer_object is supported and disable dynamic window scaling if not
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glPointSize(RESOLUTION); //pixel size
    glOrtho(0, WIDTH*RESOLUTION, 0, HEIGHT*RESOLUTION, -1, 1); // set the origin at bottom-left corner

    //glEnable(GL_TEXTURE_2D);

    init();
    
    while(!glfwWindowShouldClose(window))
    {
        processInput(window);
        setDeltaTime();

        //glOrtho(0, WIDTH*RESOLUTION, 0, HEIGHT*RESOLUTION, -1, 1); //transformation for drawing pixels
        display();

        //window scaling tests
        /*glLoadIdentity(); //transformation for drawing quads
        glBegin(GL_QUADS);
            glVertex2f(-0.5f, -0.5f);
            glVertex2f(0.5f, -0.5f);
            glVertex2f(0.5f, 0.5f);
            glVertex2f(-0.5f, 0.5f);
        glEnd();
        windowScaling(windowWidth, windowHeight);*/

        glfwSwapBuffers(window);
        glfwPollEvents();    
    }

    glfwTerminate();
    return 0;
}

void init() {
    //init deltatime
    timer.prevTime = glfwGetTime();

    player.position = Vector3(50,50,20);
    player.rotation = 0;
    player.verticalAngle = 0;
}

void display() {
    //TODO: fps limit to stop cpu usage from rising and save battery life
    clearBackground();

    if (inputPressed.showmap)
    draw2D();
    else
    draw3D();
}

void draw3D() {
    float dx = std::sin(degToRad(player.rotation));
    float dy = std::cos(degToRad(player.rotation));
    
    //player
    //pixel(player.position.x, player.position.y, Vector3(255, 0, 0));
    //pixel(dx*2 + player.position.x, dy*2 + player.position.y, Vector3(255, 0, 0));
    //pixel(halfWIDTH, halfHEIGHT, Vector3(255, 0, 0));
    //pixel(halfWIDTH, halfHEIGHT+2, Vector3(255, 0, 0));

    //transformations
    Vector2 v1(wallpos1.x - player.position.x, wallpos1.y - player.position.y);
    Vector2 v2(wallpos2.x - player.position.x, wallpos2.y - player.position.y);
    //v1.x += halfWIDTH; v2.x += halfWIDTH;
    //v1.y += halfHEIGHT; v2.y += halfHEIGHT;

    //rotate
    float rot = player.rotation-90;
    
    float r0z = v1.x * std::cos(degToRad(rot)) - v1.y * std::sin(degToRad(rot)); //z axis is forward for the player
    float r0x = v1.x * std::sin(degToRad(rot)) + v1.y * std::cos(degToRad(rot)); //x axis is left and right for the player
    float r1z = v2.x * std::cos(degToRad(rot)) - v2.y * std::sin(degToRad(rot));
    float r1x = v2.x * std::sin(degToRad(rot)) + v2.y * std::cos(degToRad(rot));

    //clip walls behind player
    if (r0z <= 0 && r1z <= 0) return;

    //if only one corner isn't visible clip the unvisible part to become visible
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

    //perspective
    int fov = 90;
    float hfov = 0.73*HEIGHT;
    float vfov = 0.2*HEIGHT;
    //float hfov = halfWIDTH/std::tan(degToRad(fov)/2);
    //float vfov = 2*std::atan((WIDTH/HEIGHT)*std::tan(degToRad(fov)/2));

    float xscale0 = hfov / r0z;
    float xscale1 = hfov / r1z;
    //float xscale0 = r0z / hfov;
    //float xscale1 = r1z / hfov;

    float x0 = halfWIDTH - (r0x*xscale0); //left wall side
    float x1 = halfWIDTH - (r1x*xscale1); //right wall side

    int ceilingHeight = 30;
    int floorHeight = -10;

    float yscale0 = vfov / r0z;
    float yscale1 = vfov / r1z;
    //float yscale0 = r0z / vfov;
    //float yscale1 = r1z / vfov;

    float top0 = halfHEIGHT + (ceilingHeight - player.position.z) * yscale0;
    float bottom0 = halfHEIGHT + (floorHeight - player.position.z) * yscale0;
    float top1 = halfHEIGHT + (ceilingHeight - player.position.z) * yscale1;
    float bottom1 = halfHEIGHT + (floorHeight - player.position.z) * yscale1;

    //test wall
    //drawLine(wallpos1, wallpos2);
    //drawLine(v1, v2);
    //drawLine(Vector2(r0z+halfWIDTH, r0x+halfHEIGHT), Vector2(r1z+halfWIDTH, r1x+halfHEIGHT));
    drawLine(Vector2(x0, top0), Vector2(x1, top1), Vector3(0, 255, 0));
    drawLine(Vector2(x0, bottom0), Vector2(x1, bottom1), Vector3(0, 255, 255));

    drawLine(Vector2(x0, top0), Vector2(x0, bottom0), Vector3(255, 0, 0));
    drawLine(Vector2(x1, top1), Vector2(x1, bottom1), Vector3(0, 0, 255));
}

void draw2D() {
    float dx = std::sin(degToRad(player.rotation));
    float dy = std::cos(degToRad(player.rotation));
    
    //player
    //pixel(player.position.x, player.position.y, Vector3(255, 0, 0));
    //pixel(dx*2 + player.position.x, dy*2 + player.position.y, Vector3(255, 0, 0));
    pixel(halfWIDTH, halfHEIGHT, Vector3(255, 0, 0));
    pixel(halfWIDTH, halfHEIGHT+2, Vector3(255, 0, 0));

    //transformations
    Vector2 v1(wallpos1.x - player.position.x, wallpos1.y - player.position.y);
    Vector2 v2(wallpos2.x - player.position.x, wallpos2.y - player.position.y);
    //v1.x += halfWIDTH; v2.x += halfWIDTH;
    //v1.y += halfHEIGHT; v2.y += halfHEIGHT;

    //rotate
    float rot = player.rotation;
    
    float r0z = v1.x * std::cos(degToRad(rot)) - v1.y * std::sin(degToRad(rot)); //z axis is forward for the player
    float r0x = v1.x * std::sin(degToRad(rot)) + v1.y * std::cos(degToRad(rot)); //x axis is left and right for the player
    float r1z = v2.x * std::cos(degToRad(rot)) - v2.y * std::sin(degToRad(rot));
    float r1x = v2.x * std::sin(degToRad(rot)) + v2.y * std::cos(degToRad(rot));

    drawLine(Vector2(r0z+halfWIDTH, r0x+halfHEIGHT), Vector2(r1z+halfWIDTH, r1x+halfHEIGHT), Vector3(0, 255, 0));
    pixel(r0z+halfWIDTH, r0x+halfHEIGHT, Vector3(255, 0, 0));
    pixel(r1z+halfWIDTH, r1x+halfHEIGHT, Vector3(0, 0, 255));
}

void drawWall(float x1, float x2, float y1, float y2, float h1, float h2) {
    float x, y;
    float dy = y2-y1;
    float dh = h2-h1;
    float dx = x2-x1; if (dx == 0) dx=1;
    float xs=x1;

    for (x=x1; x<x2; x++) {
        float b1 = dy * (x-xs)/dx+y1;
        float b2 = dh * (x-xs)/dx+h1;
        pixel(x, b1, Vector3(255, 0, 0));
        pixel(x, b2, Vector3(255, 0, 0));
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
        player.position.x += dx;
        player.position.y += dy;
        //std::cout << "player position: " << player.position.x << " " << player.position.y << " " << player.position.z << std::endl;
    }
    if (inputPressed.s) {
        //move backward
        player.position.x -= dx;
        player.position.y -= dy;
    }

    if (inputPressed.space) {
        //fly up
        player.position.z += 1;
    }
    if (inputPressed.shift) {
        //fly down
        player.position.z -= 1;
    }
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
    timer.currTime = glfwGetTime();
    timer.deltaTime = timer.currTime - timer.prevTime;

    timer.prevTime = timer.currTime;
    //std::cout << "deltatime: " << timer.deltaTime << std::endl;
}