#ifndef BLACKHOLE_H_
#define BLACKHOLE_H_

#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include "matriz.h"
#include "stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ============================================================
// SHADERS (RESTAURADOS CON MIX NATIVO DE GLSL)
// ============================================================
static const char* BH_VERT_SRC = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;
out vec3 ourColor;
out vec2 TexCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    ourColor = aColor;
    TexCoord = aTexCoord;
}
)";

static const char* BH_SPHERE_FRAG_SRC = R"(
#version 330 core
out vec4 FragColor;
void main() { FragColor = vec4(0.0, 0.0, 0.0, 1.0); }
)";

static const char* BH_DISK_FRAG_SRC = R"(
#version 330 core
in vec3 ourColor;
in vec2 TexCoord;
out vec4 FragColor;
uniform float alpha;
uniform float particleIndex;
uniform float beaming; 
void main() {
    float dist = length(TexCoord - vec2(0.5, 0.5)) * 2.0;
    float fade = smoothstep(1.0, 0.0, dist);

    vec3 coreColor  = vec3(1.0,  1.0,  1.0);
    vec3 innerColor = vec3(1.0,  0.85, 0.4);
    vec3 midColor   = vec3(1.0,  0.35, 0.02);
    vec3 outerColor = vec3(0.55, 0.04, 0.0);

    vec3 col;
    if (particleIndex < 0.2)
        col = mix(coreColor, innerColor, particleIndex / 0.2);
    else if (particleIndex < 0.5)
        col = mix(innerColor, midColor, (particleIndex - 0.2) / 0.3);
    else
        col = mix(midColor, outerColor, (particleIndex - 0.5) / 0.5);

    float emission = pow(max(0.0, 1.0 - particleIndex * 2.5), 2.0) * 1.8;
    col += vec3(emission * 0.6, emission * 0.3, emission * 0.1);
    col *= beaming;

    FragColor = vec4(col, fade * alpha);
}
)";

static const char* BH_JET_FRAG_SRC = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform float alpha;
uniform float jetProgress; 
void main() {
    float dx = abs(TexCoord.x - 0.5) * 2.0;
    float fadeX = smoothstep(1.0, 0.0, dx);
    float fadeY = smoothstep(1.0, 0.3, TexCoord.y);

    vec3 baseColor = vec3(0.9, 0.95, 1.0);
    vec3 tipColor  = vec3(0.3, 0.2,  0.9);
    vec3 col = mix(baseColor, tipColor, jetProgress);

    float glow = pow(1.0 - dx, 3.0) * 1.5;
    col += vec3(glow * 0.2, glow * 0.3, glow * 0.8);

    FragColor = vec4(col, fadeX * fadeY * alpha);
}
)";

static const char* BH_PARTICLE_FRAG_SRC = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform float alpha;
uniform float heat; 
void main() {
    float dist = length(TexCoord - vec2(0.5, 0.5)) * 2.0;
    float fade = smoothstep(1.0, 0.0, dist);
    vec3 col = mix(vec3(0.8, 0.1, 0.0), vec3(1.0, 0.9, 0.6), heat);
    FragColor = vec4(col, fade * alpha);
}
)";

static const char* BH_SKYBOX_VERT_SRC = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 TexCoords;
uniform mat4 view;
uniform mat4 projection;
void main() {
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
)";

static const char* BH_SKYBOX_FRAG_SRC = R"(
#version 330 core
in vec3 TexCoords;
out vec4 FragColor;
uniform samplerCube skybox;
void main() { FragColor = texture(skybox, TexCoords); }
)";

// ============================================================
// HELPERS C++
// ============================================================
static GLuint bh_compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(s,512,nullptr,log); std::cerr<<"[BH] Error compilando Shader: "<<log<<"\n"; }
    return s;
}
static GLuint bh_program(const char* vert, const char* frag) {
    GLuint v=bh_compile(GL_VERTEX_SHADER,vert), f=bh_compile(GL_FRAGMENT_SHADER,frag);
    GLuint p=glCreateProgram();
    glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}
static GLuint bh_makeQuadVAO() {
    float q[] = {
        -0.5f,0,-0.5f, 1,0.5f,0, 0,0,
         0.5f,0,-0.5f, 1,0.5f,0, 1,0,
         0.5f,0, 0.5f, 1,0.5f,0, 1,1,
        -0.5f,0,-0.5f, 1,0.5f,0, 0,0,
         0.5f,0, 0.5f, 1,0.5f,0, 1,1,
        -0.5f,0, 0.5f, 1,0.5f,0, 0,1,
    };
    GLuint vao, vbo;
    glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(q),q,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);                       glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return vao;
}

// ============================================================
// CLASE BLACKHOLE
// ============================================================
struct GasParticle {
    float angle;      
    float radius;     
    float height;     
    float speed;      
    float heat;       
    float size;
    float life;       
};

class BlackHole {
private:
    void m_identidad(matriz4x4& m) {
        for(int i=0; i<16; i++) m.mat[i] = (i%5 == 0) ? 1.0f : 0.0f;
    }

public:
    float position[3]   = {0.f, 0.f, 0.f};
    float bhRadius      = 1.5f;
    float diskInner     = 1.55f;
    float diskOuter     = 5.0f;
    int   diskParticles = 200;
    float diskSpeed     = 0.3f;
    float diskAlpha     = 0.7f;
    float jetLength     = 16.0f;   
    int   gasParticleCount = 1500; 
    float diskAngle     = 0.f;

    bool init() {
        progSphere   = bh_program(BH_VERT_SRC,        BH_SPHERE_FRAG_SRC);
        progDisk     = bh_program(BH_VERT_SRC,        BH_DISK_FRAG_SRC);
        progJet      = bh_program(BH_VERT_SRC,        BH_JET_FRAG_SRC);
        progParticle = bh_program(BH_VERT_SRC,        BH_PARTICLE_FRAG_SRC);
        progSkybox   = bh_program(BH_SKYBOX_VERT_SRC, BH_SKYBOX_FRAG_SRC);

        initSphere(36, 18);
        quadVAO = bh_makeQuadVAO();
        initGasParticles();
        initSkybox(); 
        return true;
    }

    void update(float dt) {
        diskAngle += diskSpeed * dt;
        if (diskAngle > 2.f*M_PI) diskAngle -= 2.f*M_PI;

        for (auto& p : gasParticles) {
            p.life -= dt * p.speed;
            p.angle += (diskSpeed * 1.5f / sqrtf(p.radius)) * dt;
            p.radius -= dt * 0.3f; 
            p.height *= (1.0f - dt * 0.8f); 
            p.heat = std::min(1.0f, p.heat + dt * 0.3f); 
            if (p.life <= 0.f || p.radius < diskInner * 0.8f)
                respawnParticle(p);
        }
    }

    void draw(const matriz4x4& view, const matriz4x4& proj) {
        drawSkybox(view, proj);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE); 
        glDisable(GL_BLEND);   
        
        drawSphere(view, proj); 

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); 
        
        drawDisk(view, proj);         
        drawJets(view, proj);         
        drawGasParticles(view, proj); 

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

private:
    GLuint progSphere=0, progDisk=0, progJet=0, progParticle=0, progSkybox=0;
    GLuint sphereVAO=0, sphereVBO=0, sphereEBO=0;
    int    sphereIndexCount=0;
    GLuint quadVAO=0;
    GLuint skyboxVAO=0, skyboxVBO=0, skyboxTex=0;
    std::vector<GasParticle> gasParticles;

    void initGasParticles() {
        gasParticles.resize(gasParticleCount);
        for (auto& p : gasParticles) respawnParticle(p, true);
    }

    void respawnParticle(GasParticle& p, bool randomLife=false) {
        p.angle  = ((float)rand()/RAND_MAX) * 2.f * M_PI;
        p.radius = diskInner*1.2f + ((float)rand()/RAND_MAX) * (diskOuter*1.5f - diskInner*1.2f);
        p.height = (((float)rand()/RAND_MAX) - 0.5f) * diskOuter * 0.8f;
        p.speed  = 0.05f + ((float)rand()/RAND_MAX) * 0.15f;
        p.heat   = ((float)rand()/RAND_MAX) * 0.3f;
        p.size   = 0.2f + ((float)rand()/RAND_MAX) * 0.4f;
        p.life   = randomLife ? ((float)rand()/RAND_MAX) : 1.0f;
    }

    void initSkybox() {
        static const float verts[108] = {
            -1,1,-1,-1,-1,-1,1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1,
            -1,-1,1,-1,-1,-1,-1,1,-1,-1,1,-1,-1,1,1,-1,-1,1,
            1,-1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,-1,-1,
            -1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,-1,-1,1,
            -1,1,-1,1,1,-1,1,1,1,1,1,1,-1,1,1,-1,1,-1,
            -1,-1,-1,-1,-1,1,1,-1,-1,1,-1,-1,-1,-1,1,1,-1,1
        };
        glGenVertexArrays(1,&skyboxVAO); glGenBuffers(1,&skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER,skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        const std::string faces[6]={
            "assets/bh_nebula/right.png","assets/bh_nebula/left.png",
            "assets/bh_nebula/top.png","assets/bh_nebula/bottom.png",
            "assets/bh_nebula/front.png","assets/bh_nebula/back.png"
        };
        glGenTextures(1,&skyboxTex);
        glBindTexture(GL_TEXTURE_CUBE_MAP,skyboxTex);
        stbi_set_flip_vertically_on_load(false);
        for(int i=0;i<6;i++){
            int w,h,ch;
            unsigned char* data=stbi_load(faces[i].c_str(),&w,&h,&ch,0);
            if(data){
                GLenum fmt=(ch==4)?GL_RGBA:GL_RGB;
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,fmt,w,h,0,fmt,GL_UNSIGNED_BYTE,data);
                stbi_image_free(data);
            } else std::cerr<<"[BH] Error cargando textura: "<<faces[i]<<"\n";
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
        glUseProgram(progSkybox);
        glUniform1i(glGetUniformLocation(progSkybox,"skybox"),0);
    }

    void drawSkybox(const matriz4x4& view, const matriz4x4& proj) {
        glDepthMask(GL_FALSE); glDepthFunc(GL_LEQUAL);
        glUseProgram(progSkybox);
        matriz4x4 v=view;
        v.mat[3]=0; v.mat[7]=0; v.mat[11]=0; v.mat[15]=1;
        glUniformMatrix4fv(glGetUniformLocation(progSkybox,"view"),1,GL_TRUE,v.mat.data());
        glUniformMatrix4fv(glGetUniformLocation(progSkybox,"projection"),1,GL_TRUE,proj.mat.data());
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP,skyboxTex);
        glDrawArrays(GL_TRIANGLES,0,36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE); glDepthFunc(GL_LESS);
    }

    void initSphere(int sectors, int stacks) {
        std::vector<float> verts;
        std::vector<unsigned int> idxs;
        for(int i=0;i<=stacks;i++){
            float phi=M_PI/2.f-i*(M_PI/stacks);
            float xy=cosf(phi),z=sinf(phi);
            for(int j=0;j<=sectors;j++){
                float theta=j*(2.f*M_PI/sectors);
                float x=xy*cosf(theta),y=xy*sinf(theta);
                verts.insert(verts.end(),{x,y,z,0,0,0,(float)j/sectors,(float)i/stacks});
            }
        }
        for(int i=0;i<stacks;i++){
            int k1=i*(sectors+1),k2=k1+sectors+1;
            for(int j=0;j<sectors;j++,k1++,k2++){
                if(i!=0)       idxs.insert(idxs.end(),{(unsigned)k1,(unsigned)k2,(unsigned)(k1+1)});
                if(i!=stacks-1)idxs.insert(idxs.end(),{(unsigned)(k1+1),(unsigned)k2,(unsigned)(k2+1)});
            }
        }
        sphereIndexCount=(int)idxs.size();
        glGenVertexArrays(1,&sphereVAO); glGenBuffers(1,&sphereVBO); glGenBuffers(1,&sphereEBO);
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER,sphereVBO);
        glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(float),verts.data(),GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,sphereEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,idxs.size()*sizeof(unsigned int),idxs.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);                               glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));glEnableVertexAttribArray(2);
        glBindVertexArray(0);
    }

    void drawSphere(const matriz4x4& view, const matriz4x4& proj) {
        glUseProgram(progSphere);
        matriz4x4 model;
        m_identidad(model);
        
        model.mat[0]=bhRadius; model.mat[5]=bhRadius; model.mat[10]=bhRadius;
        model.mat[3]=position[0]; model.mat[7]=position[1]; model.mat[11]=position[2];
        
        glUniformMatrix4fv(glGetUniformLocation(progSphere,"model"),     1,GL_TRUE,model.mat.data());
        glUniformMatrix4fv(glGetUniformLocation(progSphere,"view"),      1,GL_TRUE,view.mat.data());
        glUniformMatrix4fv(glGetUniformLocation(progSphere,"projection"),1,GL_TRUE,proj.mat.data());
        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES,sphereIndexCount,GL_UNSIGNED_INT,0);
        glBindVertexArray(0);
    }

    void drawDisk(const matriz4x4& view, const matriz4x4& proj) {
        glUseProgram(progDisk);
        glUniform1f(glGetUniformLocation(progDisk, "alpha"), diskAlpha);
        glBindVertexArray(quadVAO);

        for(int i = 0; i < diskParticles; i++) {
            float t      = 0.5f + 0.5f * sinf(i * 2.399f);
            float radius = diskInner + t * (diskOuter - diskInner);
            float angVel = diskSpeed / sqrtf(radius / diskInner);
            float angle  = (2.f * M_PI * i) / diskParticles + diskAngle * angVel / diskSpeed;

            float px = position[0] + radius * cosf(angle);
            float py = position[1];
            float pz = position[2] + radius * sinf(angle);
            float size = 1.2f + t * 1.8f;

            float beaming = 1.0f + 0.8f * cosf(angle);
            beaming = std::max(0.3f, std::min(2.5f, beaming));

            // CAPA 1
            matriz4x4 modelHoriz; 
            m_identidad(modelHoriz);

            modelHoriz.mat[0] = size; modelHoriz.mat[5] = size * 0.5f; modelHoriz.mat[10] = size;
            modelHoriz.mat[3] = px;   modelHoriz.mat[7] = py;          modelHoriz.mat[11] = pz;

            glUniform1f(glGetUniformLocation(progDisk, "particleIndex"), t);
            glUniform1f(glGetUniformLocation(progDisk, "beaming"), beaming);
            glUniformMatrix4fv(glGetUniformLocation(progDisk, "model"),      1, GL_TRUE, modelHoriz.mat.data());
            glUniformMatrix4fv(glGetUniformLocation(progDisk, "view"),       1, GL_TRUE, view.mat.data());
            glUniformMatrix4fv(glGetUniformLocation(progDisk, "projection"), 1, GL_TRUE, proj.mat.data());
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // CAPA 2 (Lente Gravitacional)
            if (t < 0.4f) { 
                matriz4x4 modelLens;
                m_identidad(modelLens);

                float lensSize = size * 1.5f;
                modelLens.mat[0] = lensSize; 
                modelLens.mat[5] = lensSize; 
                modelLens.mat[10] = lensSize * 0.2f;

                float deformacionY = (pz - position[2]) * 0.5f; 
                
                modelLens.mat[3] = px;   
                modelLens.mat[7] = py + fabs(deformacionY); 
                modelLens.mat[11] = pz * 0.9f; 

                glUniform1f(glGetUniformLocation(progDisk, "particleIndex"), t * 0.5f); 
                glUniform1f(glGetUniformLocation(progDisk, "beaming"), beaming * 1.3f);
                glUniformMatrix4fv(glGetUniformLocation(progDisk, "model"),      1, GL_TRUE, modelLens.mat.data());
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        glUniform1f(glGetUniformLocation(progDisk, "alpha"), diskAlpha);
        glBindVertexArray(0);

        drawPhotonRing(view, proj);
    }

    void drawPhotonRing(const matriz4x4& view, const matriz4x4& proj) {
        int N = 256;
        float rr = bhRadius * 1.02f;
        std::vector<float> ringVerts;
        for(int i=0;i<N;i++){
            float a = (2.f*M_PI*i)/N;
            float px = position[0]+rr*cosf(a);
            float py = position[1];
            float pz = position[2]+rr*sinf(a);
            ringVerts.push_back(px);
            ringVerts.push_back(py);
            ringVerts.push_back(pz);
        }
        GLuint vao, vbo;
        glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,ringVerts.size()*sizeof(float),ringVerts.data(),GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
        glEnableVertexAttribArray(0);

        glUseProgram(progJet);
        glUniform1f(glGetUniformLocation(progJet,"alpha"), 0.95f);
        glUniform1f(glGetUniformLocation(progJet,"jetProgress"), 0.0f); 

        matriz4x4 model;
        m_identidad(model);
        glUniformMatrix4fv(glGetUniformLocation(progJet,"model"),     1,GL_TRUE,model.mat.data());
        glUniformMatrix4fv(glGetUniformLocation(progJet,"view"),      1,GL_TRUE,view.mat.data());
        glUniformMatrix4fv(glGetUniformLocation(progJet,"projection"),1,GL_TRUE,proj.mat.data());

        glLineWidth(2.5f);
        glDrawArrays(GL_LINE_LOOP,0,N);
        glLineWidth(1.0f);

        glBindVertexArray(0);
        glDeleteBuffers(1,&vbo);
        glDeleteVertexArrays(1,&vao);
    }

   void drawJets(const matriz4x4& view, const matriz4x4& proj) {
    glUseProgram(progJet);
    glBindVertexArray(quadVAO);

    float largoJet = 16.0f; 
    int jetSegs    = 180;   // Subimos a 180 para empaquetar los quads de forma masiva
    int jetLayers  = 4;    
    
    // Incrementamos el factor a 1.8f para forzar un solapamiento (overlap) fuerte.
    // Esto hace que las texturas se fundan entre sí y borren las líneas horizontales.
    float segH = (largoJet / jetSegs) * 1.8f; 

    for(int pole = -1; pole <= 1; pole += 2) {
        for(int s = 0; s < jetSegs; s++) {
            float progress = (float)s / jetSegs;
            
            // Variación caótica sutil basada en el índice para romper el alineamiento perfecto de rejilla
            float jitter = sinf(s * 0.5f + diskAngle * 2.0f) * 0.02f;
            
            // Posición en Y con jitter integrado
            float py = position[1] + pole * (bhRadius + s * (segH * 0.55f) + jitter);

            // Control de ancho cónico invertido (ancho abajo, aguja arriba)
            float factorAncho = 3.8f * (1.0f - progress) + 0.3f * progress; 
            float width = bhRadius * 0.3f * factorAncho;
            
            // Desvanecimiento suave en los extremos
            float fade = glm::smoothstep(1.0f, 0.0f, progress);

            for(int layer = 0; layer < jetLayers; layer++) {
                // Añadimos una rotación espiral progresiva al jet con la altura (progress * 0.5f)
                float layerAngle = (2.f * M_PI * layer) / jetLayers + diskAngle * 0.4f + (progress * 0.5f);
                
                // Mantenemos el núcleo compacto
                float px = position[0] + cosf(layerAngle) * width * 0.03f;
                float pz = position[2] + sinf(layerAngle) * width * 0.03f;

                matriz4x4 model;
                m_identidad(model);
                model.mat[0]  = width; 
                model.mat[5]  = segH * 1.5f; 
                model.mat[10] = width * 0.4f;
                
                model.mat[3]  = px; 
                model.mat[7]  = py; 
                model.mat[11] = pz;

                // Forzamos un alfa ligeramente más denso (0.85f) para compactar el color
                glUniform1f(glGetUniformLocation(progJet, "alpha"),       fade * 0.85f);
                glUniform1f(glGetUniformLocation(progJet, "jetProgress"),  progress);
                glUniformMatrix4fv(glGetUniformLocation(progJet, "model"),      1, GL_TRUE, model.mat.data());
                glUniformMatrix4fv(glGetUniformLocation(progJet, "view"),       1, GL_TRUE, view.mat.data());
                glUniformMatrix4fv(glGetUniformLocation(progJet, "projection"), 1, GL_TRUE, proj.mat.data());
                
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }
    glBindVertexArray(0);
}

    void drawGasParticles(const matriz4x4& view, const matriz4x4& proj) {
        glUseProgram(progParticle);
        glBindVertexArray(quadVAO);

        for(const auto& p : gasParticles){
            float px=position[0]+p.radius*cosf(p.angle);
            float py=position[1]+p.height;
            float pz=position[2]+p.radius*sinf(p.angle);
            float alpha=p.life*0.6f;

            matriz4x4 model;
            m_identidad(model);
            model.mat[0]=p.size; model.mat[5]=p.size; model.mat[10]=p.size;
            model.mat[3]=px; model.mat[7]=py; model.mat[11]=pz;

            glUniform1f(glGetUniformLocation(progParticle,"alpha"),alpha);
            glUniform1f(glGetUniformLocation(progParticle,"heat"), p.heat);
            glUniformMatrix4fv(glGetUniformLocation(progParticle,"model"),     1,GL_TRUE,model.mat.data());
            glUniformMatrix4fv(glGetUniformLocation(progParticle,"view"),      1,GL_TRUE,view.mat.data());
            glUniformMatrix4fv(glGetUniformLocation(progParticle,"projection"),1,GL_TRUE,proj.mat.data());
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glBindVertexArray(0);
    }
};

#endif // BLACKHOLE_H_