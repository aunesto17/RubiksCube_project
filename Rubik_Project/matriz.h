#ifndef MATRIZ_H_
#define MATRIZ_H_

#include <vector>
#include <array>
#include "vertex.h"

class matriz4x4
{
public:
    std::array<float, 16> mat{
        1, 0, 0, 0, 
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    // multiplicar dos matrices
    matriz4x4 & multMat( const matriz4x4 &mat2 ){
        std::array<float, 16> res;
        for(int i = 0; i < 4; i++){
            for(int j = 0; j < 4; j++){
                res[i * 4 + j] = 0.0f;
                for(int k = 0; k < 4; k++){
                    res[i * 4 + j] += mat[i * 4 + k] * mat2.mat[k * 4 + j];
                }
            }
        }
        mat = res;
        return *this;
    }

    std::vector<vec3> multFig(const std::vector<vec3> & v){
        std::vector<vec3> res;
        for(size_t i = 0; i < v.size(); i++){
            float x = v[i].getX();
            float y = v[i].getY();
            float z = v[i].getZ();
            float w = 1.0f; // Homogeneous coordinate
            
            float x_res = x*mat[0 * 4 + 0] + y*mat[0 * 4 + 1] + z*mat[0 * 4 + 2] + w*mat[0 * 4 + 3];
            float y_res = x*mat[1 * 4 + 0] + y*mat[1 * 4 + 1] + z*mat[1 * 4 + 2] + w*mat[1 * 4 + 3];
            float z_res = x*mat[2 * 4 + 0] + y*mat[2 * 4 + 1] + z*mat[2 * 4 + 2] + w*mat[2 * 4 + 3];
            
            res.push_back(vec3(x_res, y_res, z_res));
        }
        return res;
    }

    // --- ORTONORMALIZAR LA MATRIZ (Evita deformaciones) ---
    void ortonormalizar() {
        // 1. Extraer los componentes de los ejes X, Y, Z desde la matriz (filas 0, 1 y 2)
        float x_x = mat[0], x_y = mat[1], x_z = mat[2];
        float y_x = mat[4], y_y = mat[5], y_z = mat[6];
        float z_x = mat[8], z_y = mat[9], z_z = mat[10];

        // 2. Normalizar el eje X (forzar longitud de 1.0)
        float magnitudX = sqrt(x_x * x_x + x_y * x_y + x_z * x_z);
        if (magnitudX > 0.0f) {
            x_x /= magnitudX; x_y /= magnitudX; x_z /= magnitudX;
        }

        // 3. Hacer que el eje Y sea perpendicular al eje X: Y = Y - (Y . X) * X
        float productoPuntoYX = (y_x * x_x) + (y_y * x_y) + (y_z * x_z);
        y_x -= productoPuntoYX * x_x;
        y_y -= productoPuntoYX * x_y;
        y_z -= productoPuntoYX * x_z;

        // Normalizar el nuevo eje Y
        float magnitudY = sqrt(y_x * y_x + y_y * y_y + y_z * y_z);
        if (magnitudY > 0.0f) {
            y_x /= magnitudY; y_y /= magnitudY; y_z /= magnitudY;
        }

        // 4. Hacer que el eje Z sea perpendicular a X y a Y: Z = Z - (Z . X)*X - (Z . Y)*Y
        float productoPuntoZX = (z_x * x_x) + (z_y * x_y) + (z_z * x_z);
        float productoPuntoZY = (z_x * y_x) + (z_y * y_y) + (z_z * y_z);
        z_x -= (productoPuntoZX * x_x) + (productoPuntoZY * y_x);
        z_y -= (productoPuntoZX * x_y) + (productoPuntoZY * y_y);
        z_z -= (productoPuntoZX * x_z) + (productoPuntoZY * y_z);

        // Normalizar el nuevo eje Z
        float magnitudZ = sqrt(z_x * z_x + z_y * z_y + z_z * z_z);
        if (magnitudZ > 0.0f) {
            z_x /= magnitudZ; z_y /= magnitudZ; z_z /= magnitudZ;
        }

        // 5. Guardar los ejes limpios y corregidos de vuelta en la matriz
        mat[0] = x_x; mat[1] = x_y; mat[2] = x_z;
        mat[4] = y_x; mat[5] = y_y; mat[6] = y_z;
        mat[8] = z_x; mat[9] = z_y; mat[10] = z_z;
    }


    ~matriz4x4() {};
};



#endif // MATRIZ_H_