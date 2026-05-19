#ifndef FIGURA_H_
#define FIGURA_H_

#include <vector>
#include <iostream>
#include <math.h>
#include <array>
#include "helper.h"

class Figura
{  
protected:
    std::string name;
public:
    std::vector<vec3> vertices;
    std::vector<vec3> verticesOrig; // to reset fig
    std::vector<vec3> vertexColors;
    std::vector<vec2> vertexTexCoords;

    std::vector<unsigned int> VAOs; // Vertex Array Object
    std::vector<unsigned int> VBOs; // Vertex Buffer Object
    std::vector<unsigned int> EBOs; // Element Buffer Object         

    std::vector<unsigned int> indices;

    std::vector<vec3> defaultColors = {
        vec3(1.0f, 1.0f, 1.0f), // UP (W).
        vec3(0.0f, 155.0f/255.0f, 72.0f/255.0f), // LEFT (G).
        vec3(183.0f/255.0f, 18.0f/255.0f, 52.0f/255.0f), // FRONT (R).
        vec3(0.0f, 70.0f/255.0f, 173.0f/255.0f), // RIGHT (B).
        vec3(1.0f, 88.0f/255.0f, 0.0f), // BACK (O).
        vec3(1.0f, 213.0f/255.0f, 0.0f)  // DOWN (Y).
    };

    Figura(const std::string & name) : name(name) {}

    void updateFig(std::vector<vec3> v){
        vertices = v;
    }

    virtual void updateBuffers() {}
    virtual void setupBuffers() {}
    virtual void draw(unsigned int shaderProgram) {}

    virtual void resetFig(){
        vertices = verticesOrig;
        updateBuffers();
    }

    virtual void applyTransform(matriz4x4 m) {
        vertices = m.multFig(vertices);

        // Fix Cumulative Floating-Point Drift by snapping the vertices
        for(vec3& v : vertices) {
            v.setX(std::round(v.getX() * 1000.0f) / 1000.0f);
            v.setY(std::round(v.getY() * 1000.0f) / 1000.0f);
            v.setZ(std::round(v.getZ() * 1000.0f) / 1000.0f);
        }

        updateBuffers();
    }

    unsigned getVAO(unsigned int index) { return this->VAOs[index]; }

    std::string getName() { return this->name; }
    virtual std::vector<vec3> getColors() const { return this->defaultColors; } 

    ~Figura(){
        for (int i = 0; i < VAOs.size(); i++) {
            glDeleteVertexArrays(1, &VAOs[i]);
        }
        for (int i = 0; i < VBOs.size(); i++) {
            glDeleteBuffers(1, &VBOs[i]);
        }
        for (int i = 0; i < EBOs.size(); i++) {
            glDeleteBuffers(1, &EBOs[i]);
        }
    }
};

class Cubo : public Figura
{
private:
    float size;
    std::array<bool, 6> activeFaces = {true, true, true, true, true, true};

    void buildRect(const vec3 & topLeft,
                   const vec3 & topRight,
                   const vec3 & bottomRight,
                   const vec3 & bottomLeft,
                   std::vector<vec3> & vertBuffer){	
        vertBuffer.push_back(bottomLeft);
        vertBuffer.push_back(bottomRight);
        vertBuffer.push_back(topRight);

        vertBuffer.push_back(topRight);
        vertBuffer.push_back(topLeft);
        vertBuffer.push_back(bottomLeft);
    }

public:
    Cubo(const std::string & name, float size,
        vec3 position, std::array<bool, 6> faces) : Figura(name){
            const unsigned numVertsPerFace = 6;
            const unsigned numFaces = 6;
            float dist = size / 2.0f;
            std::vector<vec3> normals;

            this->activeFaces = faces;

            this->size = size;
            
            // creamos las 6 caras
            // top
            // TL, TR, BR, BL 
            this->buildRect(vec3(-dist, dist, -dist),
                            vec3(dist, dist, -dist),
                            vec3(dist, dist, dist),
                            vec3(-dist, dist, dist),
                            this->vertices);
            // Left.
            this->buildRect(vec3(-dist,  dist, -dist),
                            vec3(-dist,  dist, dist),
                            vec3(-dist, -dist, dist),
                            vec3(-dist, -dist, -dist),
                            this->vertices);

            // Front.
            // TL, TR, BR, BL
            this->buildRect(vec3(-dist,  dist, dist),
                            vec3( dist,  dist, dist),
                            vec3( dist, -dist, dist),
                            vec3(-dist, -dist, dist),
                            this->vertices);

            // Right.
            this->buildRect(vec3(dist,  dist, dist),
                            vec3(dist,  dist, -dist),
                            vec3(dist, -dist, -dist),
                            vec3(dist, -dist, dist),
                            this->vertices);

            // Back.
            // TL, TR, BR, BL
            this->buildRect(vec3( dist,  dist, -dist),
                            vec3(-dist,  dist, -dist), 
                            vec3(-dist, -dist, -dist),
                            vec3( dist, -dist, -dist),
                            this->vertices);

            // Down.
            this->buildRect(vec3(-dist, -dist,  dist),
                            vec3( dist, -dist,  dist),
                            vec3( dist, -dist, -dist),
                            vec3(-dist, -dist, -dist),
                            this->vertices);

            // translate the vertices to position
            for(std::vector<vec3>::iterator vertex = this->vertices.begin();
                vertex != this->vertices.end(); ++vertex){
                    vertex->setX(vertex->getX() + position.getX());
                    vertex->setY(vertex->getY() + position.getY());
                    vertex->setZ(vertex->getZ() + position.getZ());
            }

            //std::cout << "Cubo::Cubo() - vertices.size() = " << this->vertices.size() << std::endl;

            this->verticesOrig = this->vertices;
            setupBuffers();

            /*
             TODO: crear las normales de los vertices
            */
            
        }

    void setupBuffers() override {
        const std::vector<vec2> texCoords = {
            vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(1.0f, 1.0f), 
            vec2(1.0f, 1.0f), vec2(0.0f, 1.0f), vec2(0.0f, 0.0f)
        };

        std::vector<float> vertexData;
        vertexColors.clear();
        vertexTexCoords.clear();

        for (size_t face = 0; face < 6; face++) {
            for (size_t v = 0; v < 6; v++) {
                size_t vertexIndex = face * 6 + v;
                
                // Position
                vertexData.push_back(vertices[vertexIndex].getX());
                vertexData.push_back(vertices[vertexIndex].getY());
                vertexData.push_back(vertices[vertexIndex].getZ());
                
                // Color and texture based on active faces
                if(!activeFaces[face]) {
                    vertexData.push_back(0.2f); vertexData.push_back(0.2f); vertexData.push_back(0.2f);
                    vertexData.push_back(0.0f); vertexData.push_back(0.0f);
                    vertexColors.push_back(vec3(0.2f, 0.2f, 0.2f));
                    vertexTexCoords.push_back(vec2(0.0f, 0.0f));
                } else {
                    const vec3& color = defaultColors[face];
                    vertexData.push_back(color.getX()); vertexData.push_back(color.getY()); vertexData.push_back(color.getZ());
                    vertexData.push_back(texCoords[v].getX()); vertexData.push_back(texCoords[v].getY());
                    vertexColors.push_back(color);
                    vertexTexCoords.push_back(texCoords[v]);
                }
            }
        }
        
        VAOs.resize(1);
        VBOs.resize(1);
        glGenVertexArrays(1, &VAOs[0]);
        glGenBuffers(1, &VBOs[0]);
        
        glBindVertexArray(VAOs[0]);
        glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), &vertexData[0], GL_DYNAMIC_DRAW); // Changed to DYNAMIC_DRAW
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }

    void updateBuffers() override {
        glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
        std::vector<float> vertexData;
        
        for (size_t i = 0; i < vertices.size(); i++) {
            vertexData.push_back(vertices[i].getX());
            vertexData.push_back(vertices[i].getY());
            vertexData.push_back(vertices[i].getZ());
            vertexData.push_back(vertexColors[i].getX());
            vertexData.push_back(vertexColors[i].getY());
            vertexData.push_back(vertexColors[i].getZ());
            vertexData.push_back(vertexTexCoords[i].getX());
            vertexData.push_back(vertexTexCoords[i].getY());
        }
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertexData.size() * sizeof(float), &vertexData[0]);
    }

    void transform(matriz4x4 & m) {
        /*
        // Iterate over all vertices in this cube
        for (vec3& v : vertices) {
            // Get the original vertex coordinates
            float x = v.getX();
            float y = v.getY();
            float z = v.getZ();

            // Apply the matrix multiplication (m * v)
            // We assume a 4th component (w) of 1.0 for a 3D point
            float newX = m.m[0] * x + m.m[4] * y + m.m[8]  * z + m.m[12];
            float newY = m.m[1] * x + m.m[5] * y + m.m[9]  * z + m.m[13];
            float newZ = m.m[2] * x + m.m[6] * y + m.m[10] * z + m.m[14];
            
            // (Optional: 4th component 'w' calculation, useful for perspective)
            // float w = m.m[3] * x + m.m[7] * y + m.m[11] * z + m.m[15];
            // if (w != 1.0f && w != 0.0f) {
            //     newX /= w;
            //     newY /= w;
            //     newZ /= w;
            // }

            // Set the new vertex coordinates
            v.setX(newX);
            v.setY(newY);
            v.setZ(newZ);
        }
            */
    }

    void draw(unsigned int shaderProgram) override {
        glBindVertexArray(VAOs[0]);
        for (int face = 0; face < 6; face++) {
            glDrawArrays(GL_TRIANGLES, face * 6, 6);
        }
    }

    // return active faces 
    virtual std::array<bool, 6> getActiveFaces() const { return this->activeFaces; }
    // destructor
    ~Cubo(){};
};


#endif // FIGURA_H_