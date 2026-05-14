#include <cmath>

const float PI = 3.1415f;

float degToRad(float degrees) {
    return degrees * (PI / 180.0f);
}

class Vector3
{
public:
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector3(float num) : x(num), y(num), z(num) {}
};

class Vector2
{
public:
    float x, y;

    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
    Vector2(float num) : x(num), y(num) {}
};