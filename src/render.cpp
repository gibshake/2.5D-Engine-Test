#include "render.h"

void draw3D(sector* sectors) {
    //render the level in 3d

    float dx = std::sin(degToRad(player.rotation));
    float dy = std::cos(degToRad(player.rotation));

    //queue for rendering each onscreen neighboring sectors
    enum {MaxQueue = 32};
    struct itemQueue {int sector, leftWIDTH, rightWIDTH;} queue[MaxQueue], *head=queue, *tail=queue;
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
    //int fov = 90;
    float focal_length = halfWIDTH/(std::tan(degToRad(fov)/2));


    float x0 = std::ceil(halfWIDTH - focal_length * r0x / r0z); //left wall side
    float x1 = std::ceil(halfWIDTH - focal_length * r1x / r1z); //right wall side

    //ceiling and floor
    float ceilingHeight = sect->ceil;
    float floorHeight = sect->floor;

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
    int xleft, xright;
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
    for (int x = xleft; x < xright; x++) {
        float t;
        int topy, bottomy;
        
        //use lerp to find the correct x value between x0 and x1 for the column on screen
        t = (x-x0) / (x1-x0);
        topy = top0 + t * (top1 - top0);
        bottomy = bottom0 + t * (bottom1 - bottom0);

        //clip y values to screen space once transformed
        if (bottomy >= topHEIGHT[x] || topy < bottomHEIGHT[x])
        continue;

        topy = min(topy, topHEIGHT[x]);
        bottomy = max(bottomy, bottomHEIGHT[x]);
        
        //finally render the columns
        if (neighbor < 0)
        {
            //render normal wall
            for (int y = bottomy; y < topy; y++)
            {
                pixel(x, y, Vector3(255,255,0));
            }
        }
        else if (neighbor >= 0 && renderedSectors[neighbor] < 0)
        {
            //neighbor wall

            int nTopy = nTop0 + t * (nTop1 - nTop0);
            int nBottomy = nBottom0 + t * (nBottom1 - nBottom0);

            //set screen height for rendering the neighbor sector
            if (bottomy < nBottomy)
                bottomHEIGHT[x] = nBottomy;
            else
                bottomHEIGHT[x] = bottomy;

            if (topy > nTopy)
                topHEIGHT[x] = nTopy;
            else
                topHEIGHT[x] = topy;

            //render top and bottom part of the neighbor wall
            if (bottomy < nBottomy)
            {
                for (int y = bottomy; y < nBottomy; y++)
                {
                    //if theres a bottom wall part render it
                    pixel(x, y, Vector3(255,0,255));
                }
            }
            if (topy > nTopy)
            {
                for (int y = nTopy; y < topy; y++)
                {
                    //if theres a top wall part render it
                    pixel(x, y, Vector3(255,255,0));
                }
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
    //render the level in a 2d map

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

void pixel(int x,int y, Vector3 c)
{ 
    //render a pixel at the (x,y) point of the screen

    //check if within screen
    if (y > HEIGHT-1 || y < 0 || x > WIDTH-1 || x < 0)
    {
        //std::cout << "Warning: Drawing outside of screen at x: " << x << " y: " << y << std::endl;
        return;
    }

    int i = (y*WIDTH + x)*4;
    
    framebuffer[i + 0] = c.x; //red
    framebuffer[i + 1] = c.y; //green
    framebuffer[i + 2] = c.z; //blue
    framebuffer[i + 3] = 255; //alpha
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

void drawImage(int xOffset, int yOffset, image &img)
{
    for (int y=0; y < img.height; y++)
    {
        for (int x=0; x < img.width; x++)
        {
            int i = (y*img.width + x)*3;
            pixel(x+xOffset, y+yOffset, Vector3(
                img.pixelData[i+0],
                img.pixelData[i+1],
                img.pixelData[i+2]
            ));
        }
    }
}

void drawText(std::string_view text, int x, int y, image *src)
{
    //TODO: replace this awful solution when a doom.wad file structure equivalent becomes supported
    for (size_t i=0; i<text.size(); i++)
    {
        if(text[i]=='u')
            drawImage(i*src[0].width + x, y, src[0]);
        else if(text[i]=='v')
            drawImage(i*src[1].width + x, y, src[1]);
        else if(text[i]=='w')
            drawImage(i*src[2].width + x, y, src[2]);
        else if(text[i]=='x')
            drawImage(i*src[3].width + x, y, src[3]);
        else if(text[i]=='y')
            drawImage(i*src[4].width + x, y, src[4]);
        else if(text[i]=='z')
            drawImage(i*src[5].width + x, y, src[5]);
        else if(text[i]=='o')
            drawImage(i*src[6].width + x, y, src[6]);
        else if(text[i]=='p')
            drawImage(i*src[7].width + x, y, src[7]);
        else if(text[i]=='q')
            drawImage(i*src[8].width + x, y, src[8]);
        else if(text[i]=='r')
            drawImage(i*src[9].width + x, y, src[9]);
        else if(text[i]=='s')
            drawImage(i*src[10].width + x, y, src[10]);
        else if(text[i]=='t')
            drawImage(i*src[11].width + x, y, src[11]);
        else if(text[i]=='i')
            drawImage(i*src[12].width + x, y, src[12]);
        else if(text[i]=='j')
            drawImage(i*src[13].width + x, y, src[13]);
        else if(text[i]=='k')
            drawImage(i*src[14].width + x, y, src[14]);
        else if(text[i]=='l')
            drawImage(i*src[15].width + x, y, src[15]);
        else if(text[i]=='m')
            drawImage(i*src[16].width + x, y, src[16]);
        else if(text[i]=='n')
            drawImage(i*src[17].width + x, y, src[17]);
        else if(text[i]=='c')
            drawImage(i*src[18].width + x, y, src[18]);
        else if(text[i]=='d')
            drawImage(i*src[19].width + x, y, src[19]);
        else if(text[i]=='e')
            drawImage(i*src[20].width + x, y, src[20]);
        else if(text[i]=='f')
            drawImage(i*src[21].width + x, y, src[21]);
        else if(text[i]=='g')
            drawImage(i*src[22].width + x, y, src[22]);
        else if(text[i]=='h')
            drawImage(i*src[23].width + x, y, src[23]);
        else if(text[i]=='6')
            drawImage(i*src[24].width + x, y, src[24]);
        else if(text[i]=='7')
            drawImage(i*src[25].width + x, y, src[25]);
        else if(text[i]=='8')
            drawImage(i*src[26].width + x, y, src[26]);
        else if(text[i]=='9')
            drawImage(i*src[27].width + x, y, src[27]);
        else if(text[i]=='a')
            drawImage(i*src[28].width + x, y, src[28]);
        else if(text[i]=='b')
            drawImage(i*src[29].width + x, y, src[29]);
        else if(text[i]=='0')
            drawImage(i*src[30].width + x, y, src[30]);
        else if(text[i]=='1')
            drawImage(i*src[31].width + x, y, src[31]);
        else if(text[i]=='2')
            drawImage(i*src[32].width + x, y, src[32]);
        else if(text[i]=='3')
            drawImage(i*src[33].width + x, y, src[33]);
        else if(text[i]=='4')
            drawImage(i*src[34].width + x, y, src[34]);
        else if(text[i]=='5')
            drawImage(i*src[35].width + x, y, src[35]);
        else if(text[i]=='!')
            drawImage(i*src[36].width + x, y, src[36]);
        else if(text[i]=='?')
            drawImage(i*src[37].width + x, y, src[37]);
        else if(text[i]=='"')
            drawImage(i*src[38].width + x, y, src[38]);
        else if(text[i]==',')
            drawImage(i*src[39].width + x, y, src[39]);
        else if(text[i]=='.')
            drawImage(i*src[40].width + x, y, src[40]);
        else if(text[i]=='/')
            drawImage(i*src[41].width + x, y, src[41]);
        else if(text[i]=='+')
            drawImage(i*src[42].width + x, y, src[42]);
        else if(text[i]=='-')
            drawImage(i*src[43].width + x, y, src[43]);
        else if(text[i]=='\'')
            drawImage(i*src[44].width + x, y, src[44]);
        else if(text[i]=='_')
            drawImage(i*src[45].width + x, y, src[45]);
        else if(text[i]==';')
            drawImage(i*src[46].width + x, y, src[46]);
        else if(text[i]==':')
            drawImage(i*src[47].width + x, y, src[47]);
        //48 is reserved for unknown symbol
        else if(text[i]=='*')
            drawImage(i*src[49].width + x, y, src[49]);
        else if(text[i]=='%')
            drawImage(i*src[50].width + x, y, src[50]);
        else if(text[i]=='(')
            drawImage(i*src[51].width + x, y, src[51]);
        else if(text[i]==')')
            drawImage(i*src[52].width + x, y, src[52]);
        else if(text[i]=='=')
            drawImage(i*src[53].width + x, y, src[53]);
        else if(text[i]!=' ') //unknown char symbol
            drawImage(i*src[48].width + x, y, src[48]);
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