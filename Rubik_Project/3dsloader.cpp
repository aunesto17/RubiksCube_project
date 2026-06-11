#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "3dsloader.h"

bool Load3DS(Mesh3DS& mesh, const char* p_filename)
{
    int i;

    FILE* l_file;

    unsigned short l_chunk_id;
    unsigned int l_chunk_lenght;

    unsigned char l_char;
    unsigned short l_qty;

    unsigned short l_face_flags;

    if ((l_file = fopen(p_filename, "rb")) == NULL) {
        std::cout << "[Load3DS] Cannot open file: " << p_filename << std::endl;
        return false;
    }

    fseek(l_file, 0, SEEK_END);
    long fileSize = ftell(l_file);
    fseek(l_file, 0, SEEK_SET);

    while (ftell(l_file) < fileSize)
    {
        fread(&l_chunk_id, 2, 1, l_file);
        fread(&l_chunk_lenght, 4, 1, l_file);

        switch (l_chunk_id)
        {
            // MAIN3DS
            case 0x4d4d:
                break;

            // EDIT3DS
            case 0x3d3d:
                break;

            // EDIT_OBJECT
            case 0x4000:
                i = 0;
                do
                {
                    fread(&l_char, 1, 1, l_file);
                    if (i < 19) mesh.name += l_char;
                    i++;
                } while (l_char != '\0' && i < 20);
                break;

            // OBJ_TRIMESH
            case 0x4100:
                break;

            // TRI_VERTEXL
            case 0x4110:
                fread(&l_qty, sizeof(unsigned short), 1, l_file);
                std::cout << "[Load3DS] Vertices: " << l_qty << std::endl;
                mesh.vertices.reserve(l_qty);
                for (i = 0; i < l_qty; i++)
                {
                    float x, y, z;
                    fread(&x, sizeof(float), 1, l_file);
                    fread(&y, sizeof(float), 1, l_file);
                    fread(&z, sizeof(float), 1, l_file);
                    mesh.vertices.push_back(vec3(x, y, z));
                }
                break;

            // TRI_FACEL1
            case 0x4120:
                fread(&l_qty, sizeof(unsigned short), 1, l_file);
                std::cout << "[Load3DS] Polygons: " << l_qty << std::endl;
                mesh.indices.reserve(l_qty * 3);
                for (i = 0; i < l_qty; i++)
                {
                    unsigned short a, b, c;
                    fread(&a, sizeof(unsigned short), 1, l_file);
                    fread(&b, sizeof(unsigned short), 1, l_file);
                    fread(&c, sizeof(unsigned short), 1, l_file);
                    fread(&l_face_flags, sizeof(unsigned short), 1, l_file);
                    mesh.indices.push_back(a);
                    mesh.indices.push_back(b);
                    mesh.indices.push_back(c);
                }
                break;

            // TRI_MAPPINGCOORS
            case 0x4140:
                fread(&l_qty, sizeof(unsigned short), 1, l_file);
                mesh.texCoords.reserve(l_qty);
                for (i = 0; i < l_qty; i++)
                {
                    float u, v;
                    fread(&u, sizeof(float), 1, l_file);
                    fread(&v, sizeof(float), 1, l_file);
                    mesh.texCoords.push_back(vec2(u, v));
                }
                break;

            // Skip unknown chunks
            default:
                fseek(l_file, l_chunk_lenght - 6, SEEK_CUR);
        }
    }
    fclose(l_file);
    std::cout << "[Load3DS] Loaded: " << mesh.name << " (" << mesh.vertices.size() << " vertices, " << mesh.indices.size() / 3 << " triangles)" << std::endl;
    return true;
}
