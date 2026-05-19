#ifndef TRANSFORM_H_
#define TRANSFORM_H_

#include <vector>
#include <math.h>
#include <stack>
#include <iostream>

#include "matriz.h"
#include "figura.h"
#include "vertex.h"

#define degToRad(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)
#define radToDeg(angleInRadians) ((angleInRadians) * 180.0 / M_PI)

class Transform
{
private:
    

public:
    matriz4x4 m;

    std::stack<float> rotXpila;
    std::stack<float> rotYpila;
    std::stack<float> rotZpila;

    std::stack<vec3> traslacionPila;
    std::stack<vec3> escalaPila;

    const float * dataPtr( void ) const { return m.mat.data(); }

    Transform(){}

    Transform & mult( const matriz4x4 &mat ){
        m.multMat(mat);
    }

    // --- TRASLACION ---
    matriz4x4 traslacion(const vec3 & pos) {
        traslacionPila.push(pos);
        matriz4x4 m;
        m.mat = {
            1, 0, 0, pos.getX(),
            0, 1, 0, pos.getY(),
            0, 0, 1, pos.getZ(),
            0, 0, 0, 1
        };
        return m;
    }

    matriz4x4 traslacion_i() {
        matriz4x4 m; 
        if(traslacionPila.empty()) {
            std::cout << "No hay traslaciones en la pila" << std::endl;
            // Return identity matrix if stack is empty
            m.mat = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
            return m;
        }
        vec3 pos = traslacionPila.top();
        traslacionPila.pop();
        m.mat = {
            1, 0, 0, -pos.getX(),
            0, 1, 0, -pos.getY(),
            0, 0, 1, -pos.getZ(),
            0, 0, 0, 1
        };
        return m;
    }

    // --- ESCALA ---
    matriz4x4 escala(const vec3 & esc) {
        escalaPila.push(esc);
        matriz4x4 m;
        m.mat = {
            esc.getX(), 0,          0,          0,
            0,          esc.getY(), 0,          0,
            0,          0,          esc.getZ(), 0,
            0,          0,          0,          1
        };
        return m;
    }

    matriz4x4 escala_i() {
        matriz4x4 m;
        if(escalaPila.empty()) {
            std::cout << "No hay escalas en la pila" << std::endl;
            m.mat = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
            return m;
        }
        vec3 esc = escalaPila.top();
        escalaPila.pop();
        m.mat = {
            1/esc.getX(), 0,            0,            0,
            0,            1/esc.getY(), 0,            0,
            0,            0,            1/esc.getZ(), 0,
            0,            0,            0,            1
        };
        return m;
    }

    // --- ROTACION X ---
    matriz4x4 rotacionX(float angle) {
        float angleRad = degToRad(angle);
        float c = cos(angleRad);
        float s = sin(angleRad);
        rotXpila.push(angleRad);
        matriz4x4 m;
        m.mat = {
            1, 0,  0, 0,
            0, c, -s, 0,
            0, s,  c, 0,
            0, 0,  0, 1
        };
        return m;
    }

    matriz4x4 rotacionX_i() {
        matriz4x4 m;
        if(rotXpila.empty()) {
            std::cout << "No hay rotaciones(X) en la pila" << std::endl;
            m.mat = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
            return m;
        }
        float angleRad = rotXpila.top();
        rotXpila.pop();
        float c = cos(angleRad);
        float s = sin(angleRad);
        m.mat = {
            1, 0,  0, 0,
            0, c,  s, 0,
            0,-s,  c, 0,
            0, 0,  0, 1
        };
        return m;
    }

    
    // --- ROTACION Y ---
    matriz4x4 rotacionY(float angle) {
        float angleRad = degToRad(angle);
        float c = cos(angleRad);
        float s = sin(angleRad);
        rotYpila.push(angleRad);
        matriz4x4 m;
        m.mat = {
            c, 0, s, 0,
            0, 1, 0, 0,
           -s, 0, c, 0,
            0, 0, 0, 1
        };
        return m;
    }

    matriz4x4 rotacionY_i() {
        matriz4x4 m;
        if(rotYpila.empty()) {
            std::cout << "No hay rotaciones(Y) en la pila" << std::endl;
            m.mat = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
            return m;
        }
        float angleRad = rotYpila.top();
        rotYpila.pop();
        float c = cos(angleRad);
        float s = sin(angleRad);
        m.mat = {
            c, 0,-s, 0,
            0, 1, 0, 0,
            s, 0, c, 0,
            0, 0, 0, 1
        };
        return m;
    }

    // --- ROTACION Z ---
    matriz4x4 rotacionZ(float angle) {
        float angleRad = degToRad(angle);
        float c = cos(angleRad);
        float s = sin(angleRad);
        rotZpila.push(angleRad);
        matriz4x4 m;
        m.mat = {
            c,-s, 0, 0,
            s, c, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
        return m;
    }

    matriz4x4 rotacionZ_i() {
        matriz4x4 m;
        if(rotZpila.empty()) {
            std::cout << "No hay rotaciones(Z) en la pila" << std::endl;
            m.mat = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
            return m;
        }
        float angleRad = rotZpila.top();
        rotZpila.pop();
        float c = cos(angleRad);
        float s = sin(angleRad);
        m.mat = {
            c, s, 0, 0,
           -s, c, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
        return m;
    }

    // reuse matrix
    void reset() {
        m.mat = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    }

    ~Transform() {}
};

#endif // TRANSFORM_H_