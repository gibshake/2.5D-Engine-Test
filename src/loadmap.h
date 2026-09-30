/*
    loadmap.h - loads any map via loading in a entire text file as a string then using a string parser
*/

#include "commons.h"

sector* sectors; //map sector data to be loaded in

struct ParseError
{
    size_t pos;
    std::string message;
};

class MapParser
{
public:
    std::string_view token;

    MapParser(std::string_view data_) : data(data_), pos(0)
    {

    }

    bool failed()
    {
        return error;
    }

    bool stringCheck(std::string_view str)
    {
        //check if string is equal to next token otherwise throw an error
        if (error)
        return false;

        next();
        if (token != str)
        {   
            error = true;

            unsigned int linepos=1;
            for (size_t i=0; i < pos; i++)
            {
                if (data[i] == '\n')
                linepos++;
            }
            std::cerr << "Map File Error: Expected \"" << str << "\", instead found \"" << token << "\" at line " << linepos << " within map file." << std::endl;
            return false;
        }
        return true;
    }

    template <typename T>
    bool numParse(T& value)
    {
        //convert next token string to any number type if unable to throw an error
        if (error)
        return false;

        next();
        auto result = std::from_chars(token.data(), token.data() + token.size(), value);

        if (result.ec != std::errc{})
        {
            error = true;

            unsigned int linepos=1;
            for (size_t i=0; i < pos; i++)
            {
                if (data[i] == '\n')
                linepos++;
            }
            std::cerr << "Map File Error: Failed to find expected number at line " << linepos << " within map file." << std::endl;
            return false;
        }
        
        return true;
        
    }

    bool sectorParse(sector& sect)
    {
        bool success = true;

        stringCheck("sector");

        stringCheck("{");

        //get sector info
        numParse(sect.floor);
        numParse(sect.ceil);
        numParse(sect.numPoints);

        //check for vertices
        stringCheck("p");

        sect.vertex = new Vector2[sect.numPoints+1];
        for (int i = 0; i < sect.numPoints+1; i++)
        {
            //get all vertices in sector
            float x, y;
            numParse(x);
            numParse(y);
            sect.vertex[i] = Vector2(x, y);
        }

        stringCheck("p");

        //check for neighboring sectors
        stringCheck("n");

        sect.neighbors = new short[sect.numPoints];
        for (int i = 0; i < sect.numPoints; i++)
        {
            //get all neighbors in sector
            short n;
            numParse(n);
            sect.neighbors[i] = n;
        }

        stringCheck("n");

        stringCheck("}");

        return !error;
    }
private:
    std::string_view data;
    size_t pos;
    bool error = false;

    bool isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    void skipWhitespace()
    {
        while ( pos < data.size() && isWhitespace(data[pos]) )
        {
            pos++;
        }
    }

    void stringParse()
    {
        //parse string to token
        const size_t nameStart = pos;

        while (pos < data.size() && !isWhitespace(data[pos]))
        {
            pos++;
        }

        token = data.substr(nameStart, pos - nameStart);
    }

    void skipComment()
    {
        if (pos+1 < data.size() && data[pos] == '/' && data[pos+1] == '/')
        {
            while (pos < data.size() && data[pos] != '\r' && data[pos] != '\n')
            {
                pos++;
            }
        }
    }

    bool next()
    {
        //get the next word in buffer as a string without any whitespace or comments
        
        if (pos > data.size() || error)
        return false;
        
        skipWhitespace();
        while (pos+1 < data.size() && data[pos] == '/' && data[pos+1] == '/')
        {
            //skip any comments and whitespace until the next token
            skipComment();
            skipWhitespace();
        }

        stringParse(); //only parsed to string token

        return true;
    }
};

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
}

bool loadMap(std::string fileName) {

    std::ifstream file(fileName, std::ios::binary);
    //std::cout << "Opening " << fileName << " in " << std::filesystem::current_path() << std::endl;
    if (!file.is_open())
    {
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
    MapParser parser(buffer);

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

    //get all sector data
    sectors = new sector[numSectors];
    for (int i=0; i<numSectors; i++)
    {
        parser.sectorParse(sectors[i]);
    }
    
    return !parser.failed();
}