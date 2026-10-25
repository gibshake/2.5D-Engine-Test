/*
    loadmap.h - loads any map via loading in a entire text file as a string then using a string parser
*/

#include "commons.h"
#include "stringparser.h"

sector* sectors = nullptr; //map sector data to be loaded in and current map the player is in

void loadDefaultMap()
{
    player.position = Vector3(50,50,20);
    player.rotation = 0;
    player.sector = 0;

    numSectors=2;
    sectors = new sector[] {
        { //1st sector
            -10, //floor
            30, //ceiling
            new Vector2[] {
                Vector2(-70,70), //must be done counterclockwise for normals to not be inverted
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
}

bool sectorParse(stringParser& parser, sector& sect)
{
    parser.stringCheck("sector");

    parser.stringCheck("{");

    //get sector info
    parser.numParse(sect.floor);
    parser.numParse(sect.ceil);
    parser.numParse(sect.numPoints);

    //check for vertices
    parser.stringCheck("p");

    sect.vertex = new Vector2[sect.numPoints+1];
    for (int i = 0; i < sect.numPoints+1; i++)
    {
        //get all vertices in sector
        float x, y;
        parser.numParse(x);
        parser.numParse(y);
        sect.vertex[i] = Vector2(x, y);
    }

    parser.stringCheck("p");

    //check for neighboring sectors
    parser.stringCheck("n");

    sect.neighbors = new short[sect.numPoints];
    for (int i = 0; i < sect.numPoints; i++)
    {
        //get all neighbors in sector
        short n;
        parser.numParse(n);
        sect.neighbors[i] = n;
    }

    parser.stringCheck("n");

    parser.stringCheck("}");

    return !parser.failed();
}

bool loadMap(std::string fileName) {

    std::ifstream file(fileName, std::ios::binary);
    //std::cout << "Opening " << fileName << " in " << std::filesystem::current_path() << std::endl;
    if (!file.is_open())
    {
        std::cerr << "Loadmap Error: Failed to open file at \"" << fileName << "\"." << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    const size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    //put all data from text file into buffer string
    std::string buffer(size, '\0');
    file.read(buffer.data(), size);
    file.close();

    //read file data with parser
    stringParser parser(buffer);

    //check if theres a MAP signature
    parser.stringCheck("MAP");

    //get player data
    parser.stringCheck("player");

    parser.stringCheck("{");

    parser.numParse(player.position.x);
    parser.numParse(player.position.y);
    parser.numParse(player.position.z);

    parser.numParse(player.rotation);

    parser.numParse(player.sector);

    parser.stringCheck("}");


    //get numSect
    parser.stringCheck("numSect");
    parser.numParse(numSectors);

    sector* map = nullptr;
    if (!parser.failed())
    {
        //get all sector data
        map = new sector[numSectors];
        for (int i=0; i<numSectors; i++)
        {
            sectorParse(parser, map[i]);
        }
    }

    if (parser.failed())
    {
        if (map != nullptr)
        {
            //possible fatal error if a parser error is thrown before vertex or neighbors gets allocated
            delete [] map->vertex;
            delete [] map->neighbors;
            
            delete [] map;
            map = nullptr;
        }

        std::cerr << "Loadmap Error: Failed to load \""<< fileName <<"\", error found at line " << parser.getLinePos() << " within file." << std::endl;
        return false;
    }
    
    if (sectors != nullptr)
    {
        //map deallocation
        delete [] sectors->vertex;
        delete [] sectors->neighbors;
        
        delete [] sectors;
        sectors = nullptr;
    }
    
    //transfer data to become the current map
    sectors = map;
    
    return !parser.failed();
}