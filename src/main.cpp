#include "commons.h"
#include "commands.h"
#include "render.h"
//#include "loadmap.h"
#include "loadimage.h"
#include "font.h"

//fps limit clock
using steadyClock = std::chrono::steady_clock;
auto nextFrame = steadyClock::now();

float fpsCap = 1.0 / (float)FPSLIMIT;
auto frameTime = std::chrono::duration_cast<steadyClock::duration>(std::chrono::duration<float>(fpsCap));
auto frameMargin = std::chrono::microseconds(500);

GLuint screenTexture; //screenbuffer texture
image fontimg;
image fontmap[54];

float getCurrentTime();
void setDeltaTime();
void playerWallCollision(Vector2 &wishDir, short prevSect = -1);
void playerMovement();
void processInput(GLFWwindow *window);
void typingCharInput(GLFWwindow* window, unsigned int codepoint);
void typingKeyInput(GLFWwindow* window, int key, int scancode, int action, int mods);
void fpsLimit();
void initScreenbuffer();
void updateScreenBuffer();
void init();

void display(GLFWwindow *window);

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    windowWidth = width;
    windowHeight = height;
    
    glViewport(0, 0, width, height);
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
    window = glfwCreateWindow(windowWidth, windowHeight, "DOOM", NULL, NULL);
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

    
    glViewport(0, 0, windowWidth, windowHeight);
    //glfwSwapInterval(1); // Enable vsync (limits fps to monitor refresh rate, dont enable with fps limit)

    //TODO remove GL_ARB_framebuffer_object since its unneeded
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    //glOrtho(0, WIDTH*RESOLUTION, 0, HEIGHT*RESOLUTION, -1, 1); // set the origin at bottom-left corner
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    
    glDisable(GL_DEPTH_TEST);

    //setup input for typing mode
    glfwSetCharCallback(window, typingCharInput);
    glfwSetKeyCallback(window, typingKeyInput);

    initScreenbuffer();
    init();
    
    while(!glfwWindowShouldClose(window))
    {
        //main loop

        setDeltaTime();

        if (!inputPressed.typing)
        processInput(window);

        display(window);

        glfwPollEvents();

        fpsLimit(); //the entire main loop will run at 35 fps or FPSLIMIT
    }

    glfwTerminate();
    return 0;
}

void init() {
    //init deltatime
    timer.prevTime = getCurrentTime();

    //load fontmap
    if (!loadBMP("doomfont.bmp", fontimg))
    {
        //use default font if failed to load image
        std::cerr << "Failed to load doomfont.bmp, reverting to default font." << std::endl;
        fontimg = defaultfont();
    }

    //convert each letter within the font image into their own individial images
    for (int i=0; i<54; i++)
    {
        fontmap[i] = loadTile(fontimg, 8, 8, i);
    }
    delete[] fontimg.pixelData;

    //TODO: make a main menu to prevent being required to load a default map at startup
    loadDefaultMap();
}

void initScreenbuffer()
{
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        WIDTH,
        HEIGHT,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    glBindTexture(GL_TEXTURE_2D, 0);
}

void updateScreenBuffer()
{
    //displays framebuffer into the screen via drawing a fullscreen quad with framebuffer as a texture

    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0, 0,
        WIDTH, HEIGHT,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        framebuffer
    );

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, screenTexture);

    //glColor4f(1,1,1,1); 
    glBegin(GL_QUADS);
        glVertex2f(-1.0f, -1.0f); glTexCoord2f(1.0f, 0.0f);
        glVertex2f(1.0f, -1.0f); glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, 1.0f); glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-1.0f, 1.0f); glTexCoord2f(0.0f, 0.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    
}

void display(GLFWwindow *window) {

    clearBackground();
    
    //render the level
    if (inputPressed.showmap)
    draw2D(&sectors[player.sector]);
    else
    draw3D(sectors);

    //render text
    drawText(msg, 0, 0, fontmap);

    updateScreenBuffer();
    glfwSwapBuffers(window);

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
    //if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    //    glfwSetWindowShouldClose(window, true);
    inputPressed.w = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    inputPressed.a = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    inputPressed.s = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    inputPressed.d = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    inputPressed.space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    inputPressed.shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    inputPressed.showmap = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
    
    if(glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS) //~ key
    inputPressed.typing = true;

    playerMovement();
}

void typingCharInput(GLFWwindow* window, unsigned int codepoint)
{
    if (!inputPressed.typing)
    return;

    char character = static_cast<char>(codepoint);
    msg.push_back(character);
}

void typingKeyInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //handles unique inputs when user is typing like enter, esc, backspace
    if (!inputPressed.typing)
    return;

    if (key == GLFW_KEY_BACKSPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        //backspace
        if (!msg.empty())
            msg.pop_back();
        return;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        //esc
        msg = "";
        inputPressed.typing = false;
        return;
    }
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
    {
        //enter
        command(msg);
        inputPressed.typing = false;
        msg = "";
        return;
    }
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