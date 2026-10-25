/*
    commands.h - ingame commands that can be accessed using the built-in command prompt by pressing ~
    commands can vary in use, from changing the current map to simply changing the player's fov
*/

#pragma once

#include "commons.h"
#include "stringparser.h"
#include "render.h"
#include "loadmap.h"
#include <unordered_map>
#include <functional>

struct args {
    int count;
    std::string *item;
};

bool map(const args& arg)
{
    //changes the current map to any map file
    //COMMAND: map (filepath to map file)
    //COMMAND EXAMPLE: map test.map

    if (arg.count != 2)
    return false;

    if (arg.item[0] == "map")
    {
        std::string mapfile = arg.item[1];
        bool success = loadMap(mapfile);

        return success;
    }

    return false;
}

bool setfov(const args& arg)
{
    //changes player fov to the value set
    //COMMAND: fov (0 < any int number)
    //COMMAND EXAMPLE: fov 90

    if (arg.count != 2)
    return false;

    if (arg.item[0] == "fov")
    {
        std::string numStr = arg.item[1];
        int num;

        stringParser parser(numStr);
        parser.numParse(num);

        if (!parser.failed() && 0 < num)
        {
            fov = num;
            return true;
        }
    }

    return false;
}

bool command(std::string cmd)
{
    //converts an inputed string containing command info into the correct command or executed function

    bool success = false;

    if (cmd.empty())
    return false;

    args arg;
    arg.count = 0;

    //get argument values
    stringParser parser(cmd);
    while (!parser.endOfStr())
    {
        parser.next();
        arg.count++;
    }
    arg.item = new std::string[arg.count];

    parser.setPos(0);
    for (int i=0; i<arg.count; i++)
    {
        parser.strParse(arg.item[i]);
    }

    std::unordered_map<std::string, std::function<bool(const args&)>> commands = {
        {"map", map},
        {"fov", setfov}
    };

    std::string cmdName = arg.item[0];

    //execute command if it exists
    auto it = commands.find(cmdName);
    if (it != commands.end())
        success = it->second(arg);
    else
    {
        std::cout << "Unknown command \"" << cmd << "\"" << std::endl;
        delete [] arg.item;
        return false;
    }

    //deallocate
    delete [] arg.item;
    arg.item = nullptr;

    return success;
}