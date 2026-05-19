#ifndef VERTEX_H_
#define VERTEX_H_

#include <vector>
#include <cmath>   // For sin, cos, tan, sqrt
#include <cstring> // For memcpy

class vec3
{
public:
    float x, y, z;
    vec3(float x = 0.0f, 
            float y = 0.0f, 
            float z = 0.0f) : x(x), y(y), z(z) {}

    std::vector<float> operator+(std::vector<float> & v)
    {
        std::vector<float> res;
        for(int i = 0; i < v.size(); i+=3)
        {
            res.push_back(v[i] + x);
            res.push_back(v[i+1] + y);
            res.push_back(v[i+2] + z);
        }
        return res;
    }

    vec3 operator+(const vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    vec3 operator-(const vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    // sum operator
    vec3 operator+(vec3 & v)
    {
        return vec3(x + v.x, y + v.y, z + v.z);
    }

    // subtraction operator
    vec3 operator-(vec3 & v)
    {
        return vec3(x - v.x, y - v.y, z - v.z);
    }

    // multiplication operator
    vec3 operator*(float num)
    {
        return vec3(x*num, y*num, z*num);
    }
    const vec3 operator*(float num) const
    {
        return vec3(x*num, y*num, z*num);
    }

    // plus equal operator
    vec3 operator+=(vec3 & v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    // division operator
    vec3 operator/(float num)
    {
        return vec3(x/num, y/num, z/num);
    }

    // equal operator
    // vec3 operator=(vec3 & v)
    // {
    //     x = v.x;
    //     y = v.y;
    //     z = v.z;
    //     return *this;
    // }

    float getX() const { return x; }
    float getY() const { return y; }
    float getZ() const { return z; }

    void setX( float num ) { this->x = num; }
    void setY( float num ) { this->y = num; }
    void setZ( float num ) { this->z = num; }

    ~vec3() {}
};


class vec2
{
public:
    float x, y;
    vec2(float x = 0.0f, 
            float y = 0.0f) : x(x), y(y) {}

    std::vector<float> operator+(std::vector<float> & v)
    {

    }

    float getX() const { return x; }
    float getY() const { return y; }

    void setX( float num ) { this->x = num; }
    void setY( float num ) { this->y = num; }

    ~vec2() {}
};


#endif // VERTEX_H_