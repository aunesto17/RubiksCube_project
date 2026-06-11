#ifndef THREEDSLOADER_H_
#define THREEDSLOADER_H_

#include <string>
#include <vector>
#include "vertex.h"

struct Mesh3DS {
    std::string name;
    std::vector<vec3> vertices;
    std::vector<unsigned short> indices;
    std::vector<vec2> texCoords;
};

bool Load3DS(Mesh3DS& mesh, const char* filename);

#endif // THREEDSLOADER_H_
