#pragma once

#include <iostream>
#include <string>

class stringParser
{
public:
    std::string_view token;

    stringParser(std::string_view data_) : data(data_), pos(0)
    {

    }

    bool failed()
    {
        return error;
    }

    bool endOfStr()
    {
        if (pos >= data.size())
        return true;

        return false;
    }

    void setPos(size_t p)
    {
        pos = p;
    }

    int getLinePos()
    {
        //get what line count the current position is on
        unsigned int linepos=1;
        for (size_t i=0; i < pos; i++)
        {
            if (data[i] == '\n')
            linepos++;
        }

        return linepos;
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
            return false;
        }
        return true;
    }

    bool strParse(std::string& str)
    {
        //pass next token value to string
        if (error)
        return false;

        next();
        str = token;

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
            return false;
        }
        
        return true;
        
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
};