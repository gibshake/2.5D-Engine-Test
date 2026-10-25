#include "math.h"

const float PI = 3.1415f;

float degToRad(float degrees) {
    return degrees * (PI / 180.0f);
}

float min(float a, float b)
{
    //return the smallest value of a or b
    return (a < b ? a : b);
}

float max(float a, float b)
{
    //return the biggest value of a or b
    return (a > b ? a : b);
}

float clamp(float a, float minimum, float maxium)
{
    return (min(max(a,minimum),maxium));
}

void swap(float &a, float &b)
{
    //swap values of a and b
    float temp = b;
    b = a;
    a = temp;
}

float crossProduct (Vector2 a, Vector2 b)
{
    return (a.x * b.y - a.y * b.x);
}

float dotProduct (Vector2 a, Vector2 b)
{
    return (a.x * b.x + a.y * b.y);
}