/*
UCSP
COMPUTACION GRAFICA - 2026-I

- ALEXANDER BAYLON
- CRISTIAN MELLADO
- JOSE VILCA
- WALTER VALDIVIA

Proyecto CUBO RUBIK + ESCAPE ESPACIAL DE ASTEROIDES Y AGUJERO NEGRO
main_final_unificado.cpp
*/

#include "rubik.h"
#include "helper.h" 
#include "camera.h"
#include "skybox.h"
#include "spaceship.h"
#include "blackhole.h"
#include "asteroid.h"
#include "bullet.h"

#include <vector>
#include <cstdlib> // Para rand() y srand()

// VARIABLES GLOBALES - SISTEMA DE ASTEROIDES
std::vector<Asteroid> listaAsteroides;
float tiempoUltimoAsteroide = 0.0f;
float frecuenciaSpawn = 1.5f; // Generar un asteroide cada 1.5 segundos
unsigned int asteroideTexID;  // ID para la textura de los asteroides

// VARIABLES GLOBALES - ESTADO DEL JUEGO
int vidas = 3;
bool gameOver = bool(false);
float invincibleTimer = 0.0f;   // tiempo restante de invencibilidad tras un golpe (segundos)
const float INVINCIBLE_DURATION = 2.0f;  // duracion de invencibilidad post-golpe
int asteroidesDestruidos = 0;   // contador de asteroides que pasaron la nave (score basico)

// VARIABLES GLOBALES - SISTEMA DE BALAS (SHOOTING)
std::vector<Bullet> listaBalas;
float tiempoUltimoDisparo = 0.0f;
const float FIRE_RATE = 0.15f; // segundos entre disparos
unsigned int bulletTexID;      // textura 1x1 transparente para que el shader use vertex color

void framebuffer_size_callback(GLFWwindow* window, int width, int height); //dimensionar la pantalla

// Compile-time animation speed defines
#define DEFAULT_SCRAMBLE_SPEED  4.0f
#define DEFAULT_SOLVE_SPEED     1.5f
#define SCRAMBLE_NUM_MOVES      50

// resolucion de la ventana
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

static void key_callback(GLFWwindow*, int, int, int, int);
static void cursor_position_callback(GLFWwindow*, double, double); // callback del mouse: rota la nave

class colorVec {
	public:
    float x, y, z;
    colorVec(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}
};

colorVec getRandomColor() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    return colorVec(dis(gen), dis(gen), dis(gen));
}

// definimos las figuras
Camera camera;
CuboRubik * cuboRubik = new CuboRubik(glfwGetTime(), camera);
Spaceship spaceship;
bool isClockwise = true; // direccion de rotacion camadas

Transform trans; // temporal para mover el cubo

float lastFrame = 0.0f;
float deltaTime = 0.0f;
float currentFrame = 0.0f;

float lastMouseX = 400.0f, lastMouseY = 300.0f; // ultima posicion del cursor (para calcular delta)
bool firstMouse = true;                         // true hasta el primer evento de mouse
const float mouseSensitivity = 0.1f;            // sensibilidad del mouse para rotar la nave

// initial colors
colorVec backgroundColor(0.0f, 0.0f, 0.0f); // white background

// variable for current drawing mode
GLenum currentDrawMode = GL_TRIANGLES;

// Continuous input polling — runs every frame
void processInput(GLFWwindow* window);

// Funciones del juego (definidas despues de main)
void handleShooting(GLFWwindow* window, float currentFrame);
void updateBullets(float deltaTime);
void spawnAsteroids(float currentFrame);
void processAsteroids(float deltaTime, unsigned int shaderProgram);
void drawBullets(unsigned int shaderProgram);

// vertex shader basico con textura
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"
    "layout (location = 2) in vec2 aTexCoord;\n"

    "out vec3 ourColor;\n"
    "out vec2 TexCoord;\n"

    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"

    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "   ourColor = aColor;\n"
    "   TexCoord = vec2(aTexCoord.x,aTexCoord.y);\n"
    "}\0";

// fragment shader con textura y blending
const char *fragmentShaderTexSource = "#version 330 core\n"
    "out vec4 FragColor;\n"

    "in vec3 ourColor;\n"
    "in vec2 TexCoord;\n"

    "uniform sampler2D ourTexture;\n" 

    "void main()\n"
    "{\n"
    "   // Sample from the single texture\n"
    "   vec4 texColor = texture(ourTexture, TexCoord);\n"

    "   // Handle transparency and color blending (same logic as before)\n"
    "   if(texColor.a < 0.1) {\n"
    "       // If mostly transparent, use the face color\n"
    "       FragColor = vec4(ourColor, 1.0);\n"
    "   } else {\n"
    "       // Otherwise blend the texture with the face color\n"
    "       vec3 blendedColor = mix(ourColor, texColor.rgb, 0.7);\n"
    "       FragColor = vec4(blendedColor, 1.0);\n"
    "   }\n"
    "}\0";


int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);  // Request 24-bit depth buffer

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Rubik + Space Escape (Asteroids & Black Hole)", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);         // registrar callback del mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);       // capturar y ocultar el cursor

    // glad: load all OpenGL function pointers
	gladLoadGL(glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST); // corrige el rendering 3D
    glDepthFunc(GL_LESS); 

    glEnable(GL_BLEND); // para transparencia en texturas
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // mezcla de textura con triangulos

    // CARGA DE TEXTURAS Y MALLAS
    unsigned int ourTextureID = loadTexture("assets/cubitoBorder.png");

    if (ourTextureID) {
        std::cout << "All textures loaded successfully!" << std::endl;
    } else {
        std::cout << "Failed to load one or more textures!" << std::endl;
    }

    // Cargar textura de la nave
    unsigned int spaceshipTexID = loadTexture("assets/spaceshiptexture.bmp");

    // Cargar textura y malla del asteroide
    asteroideTexID = loadTexture("assets/asteroide.jpg");
    Asteroid::loadMesh("assets/asteroide.3ds");

    // Crear textura 1x1 transparente para las balas (el shader usa vertex color cuando alpha < 0.1)
    unsigned char transparentPixel[] = {0, 0, 0, 0};
    glGenTextures(1, &bulletTexID);
    glBindTexture(GL_TEXTURE_2D, bulletTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, transparentPixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Inicializar malla compartida de las balas (esfera UV)
    Bullet::initMesh(1.0f);

    // Inicializar Agujero Negro (Reemplaza al Skybox estándar)
    BlackHole blackhole;
    blackhole.init();
	blackhole.position[0] = 20.0f;  // Mueve el BH lejos del Rubik en X
	blackhole.bhRadius     = 2.0f;
	blackhole.diskInner    = 3.0f;
	blackhole.diskOuter    = 8.0f;
	blackhole.diskParticles = 500;
	blackhole.diskAlpha = 0.5;

    // Inicializar Nave Espacial
    if (!spaceship.load("assets/spaceship.3DS")) {
        std::cout << "Warning: Failed to load spaceship model. Continuing without spaceship." << std::endl;
    }
    
    // Posición inicial de juego de la nave (Tomada de tu versión de asteroides)
    spaceship.setPosition(vec3(0.0f, 0.0f, -10.0f));

    // Compilación de Shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderTexSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    cuboRubik->init();
    cuboRubik->printMenu();

    glUseProgram(shaderProgram);
    int ourTextureLoc = glGetUniformLocation(shaderProgram, "ourTexture");
    glUniform1i(ourTextureLoc, 0); // Textura en unidad 0

    glPointSize(10.f);
    glLineWidth(5.f);

    // Sincronizar tiempo de inicio
    lastFrame = glfwGetTime();

    // RENDER LOOP UNIFICADO
    while (!glfwWindowShouldClose(window))
    {
        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Actualizar temporizador de invencibilidad
        if (invincibleTimer > 0.0f) {
            invincibleTimer -= deltaTime;
        }

        // Captura de controles continuos (Cámara Orbital, Nave y Rotación del Cubo)
        processInput(window); 

        // DISPARO DE BALAS (Click Izquierdo del Mouse)
        handleShooting(window, currentFrame);

        // Listener de eventos discretos (Teclas de control del Cubo)
        glfwSetKeyCallback(window, key_callback);

        // Limpieza de pantalla
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, 1.0f);

        // Actualización de Animaciones lógicas y de cámara
        camera.updateCameraAnimation(deltaTime);
        camera.setTarget(spaceship.getPosition());

        if (camera.isFollowMode()) {
            camera.updateFollow(spaceship.getPosition(), spaceship.yaw, spaceship.pitch, deltaTime);
        }

        cuboRubik->update_animation(deltaTime);

        // Obtener locations para transformaciones de matrices del Shader principal
        viewLoc = glGetUniformLocation(shaderProgram, "view");
        projLoc = glGetUniformLocation(shaderProgram, "projection");
        modelLoc = glGetUniformLocation(shaderProgram, "model");
        ourTextureLoc = glGetUniformLocation(shaderProgram, "ourTexture");
        
        // Seleccionar matriz de vista activa (Seguimiento o Libre)
        matriz4x4 viewMatrix = camera.isFollowMode() ? camera.getFollowViewMatrix() : camera.getViewMatrix();

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspectRatio = (float)width / (float)height;
        matriz4x4 projMatrix = camera.getPerspectiveMatrix(aspectRatio);
        
        // 1. DIBUJAR ENTORNO: AGUJERO NEGRO (Nebulosa + Esfera + Disco de Acreción)
        blackhole.update(deltaTime);
        blackhole.draw(viewMatrix, projMatrix);

        // 2. ACTIVAR SHADER PRINCIPAL CON TEXTURAS PARA LOS OBJETOS 3D
        glUseProgram(shaderProgram);

        // Enviar matrices de cámara actualizadas al shader program
        glUniformMatrix4fv(viewLoc, 1, GL_TRUE, viewMatrix.mat.data());
        glUniformMatrix4fv(projLoc, 1, GL_TRUE, projMatrix.mat.data());

        // CUBO RUBIK
        matriz4x4 modelMatrixCube; // Matriz identidad base
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, modelMatrixCube.mat.data());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ourTextureID);  
        cuboRubik->draw(shaderProgram);

        // ACTUALIZACION Y RENDERIZADO DE OBJETOS DEL JUEGO
        updateBullets(deltaTime);
        spawnAsteroids(currentFrame);
        processAsteroids(deltaTime, shaderProgram);
        drawBullets(shaderProgram);

        // RENDERIZADO DE LA NAVE JUGADORA
        glUniform1i(ourTextureLoc, 0); 
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, spaceshipTexID); // Cambiar a la textura metálica de la nave
        
        spaceship.draw(shaderProgram, viewMatrix, projMatrix);

        // Swap de buffers e IO eventos
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Liberación de memoria de shaders al cerrar
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

// Eventos discretos de teclado (Clicks instantáneos)
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    // Reiniciar juego
    bool gameReset = false;
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        if (gameOver || vidas < 3) {
            vidas = 3;
            gameOver = false;
            invincibleTimer = 0.0f;
            asteroidesDestruidos = 0;
            listaAsteroides.clear();
            listaBalas.clear();
            tiempoUltimoDisparo = 0.0f;
            spaceship.setPosition(vec3(0.0f, 0.0f, -10.0f));
            spaceship.yaw = 0.0f;
            spaceship.pitch = 0.0f;
            gameReset = true;
            std::cout << "[JUEGO] Reiniciado. Vidas: 3" << std::endl;
        }
    }
    
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        isClockwise = !isClockwise;
        std::cout << "Direccion de rotacion: " << (isClockwise ? "Horaria" : "Antihoraria ' ") << std::endl;
    }

    // Control de Capas de Rubik
    if (!cuboRubik->is_animation_running() && !cuboRubik->isSequenceRunning()) {
        if (key == GLFW_KEY_T && action == GLFW_PRESS) cuboRubik->rotateU(isClockwise);
        if (key == GLFW_KEY_R && action == GLFW_PRESS && !gameReset) cuboRubik->rotateL(isClockwise); 
        if (key == GLFW_KEY_F && action == GLFW_PRESS) cuboRubik->rotateF(isClockwise); 
        if (key == GLFW_KEY_G && action == GLFW_PRESS) cuboRubik->rotateR(isClockwise); 
        if (key == GLFW_KEY_Y && action == GLFW_PRESS) cuboRubik->rotateB(isClockwise); 
        if (key == GLFW_KEY_H && action == GLFW_PRESS) cuboRubik->rotateD(isClockwise); 
        
        if (key == GLFW_KEY_V && action == GLFW_PRESS) cuboRubik->rotateSV(isClockwise); 
        if (key == GLFW_KEY_B && action == GLFW_PRESS) cuboRubik->rotateSH(isClockwise); 
        if (key == GLFW_KEY_N && action == GLFW_PRESS) cuboRubik->rotateSS(isClockwise); 
    }
    else if (action == GLFW_PRESS && (
        key == GLFW_KEY_T || key == GLFW_KEY_R || key == GLFW_KEY_F || 
        key == GLFW_KEY_G || key == GLFW_KEY_Y || key == GLFW_KEY_H ||
        key == GLFW_KEY_V || key == GLFW_KEY_B || key == GLFW_KEY_N
    )) {
        std::cout << "[INPUT] Rotation key ignored: animation in progress." << std::endl;
    }

    if (key == GLFW_KEY_K && action == GLFW_PRESS) {
        cuboRubik->cancelSequence();
        cuboRubik->resetRubik();
    }  
    
    // Modos de Dibujo del Polígono
    if(key == GLFW_KEY_I && action == GLFW_PRESS) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if(key == GLFW_KEY_O && action == GLFW_PRESS) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if(key == GLFW_KEY_0 && action == GLFW_PRESS) glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    
    if(key == GLFW_KEY_P && action == GLFW_PRESS) cuboRubik->printMenu();
    
    if (key == GLFW_KEY_J && action == GLFW_PRESS) cuboRubik->solveRubik();
    if (key == GLFW_KEY_M && action == GLFW_PRESS) cuboRubik->scrambleRubik(SCRAMBLE_NUM_MOVES);
    
    // Velocidad de secuencias
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) cuboRubik->setSequenceSpeed(1.0f);
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) cuboRubik->setSequenceSpeed(2.0f);
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) cuboRubik->setSequenceSpeed(4.0f);
    if (key == GLFW_KEY_4 && action == GLFW_PRESS) cuboRubik->setSequenceSpeed(8.0f);
    if (key == GLFW_KEY_5 && action == GLFW_PRESS) cuboRubik->setSequenceSpeed(16.0f);
    if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS) cuboRubik->setSequenceSpeed(cuboRubik->getSequenceSpeed() + 1.0f);
    if (key == GLFW_KEY_MINUS && action == GLFW_PRESS) cuboRubik->setSequenceSpeed(cuboRubik->getSequenceSpeed() - 1.0f);
    
    if (key == GLFW_KEY_L && action == GLFW_PRESS) backgroundColor = getRandomColor();

    // ALTERNAR MODO DE CÁMARA (Orbital vs. Primera Persona en Nave) con la tecla F
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        camera.toggleFollowMode();
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Entrada continua (Acciones fluidas por frame)
void processInput(GLFWwindow* window) {
    if (gameOver) return; // No permitir movimiento si el juego termino
    // Si NO estamos siguiendo la nave, las teclas WASD controlan el modo orbital libre
    if (!camera.isFollowMode()) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.moveForward(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.moveBackward(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.moveLeft(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.moveRight(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.zoomIn(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.zoomOut(deltaTime);
    }
    
    // Movimiento fluido de propulsión de la Nave (Flechas del Teclado)
    const float shipSpeed = 5.0f;
    float step = shipSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) spaceship.moveForward(step);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) spaceship.moveBackward(step);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) spaceship.yaw += 90.0f * deltaTime;  
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) spaceship.yaw -= 90.0f * deltaTime; 

    // Rotación Global del Cubo Rubik mediante matrices (Z, X, C)
    const float rotSpeed = 90.0f;
    float ang = rotSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) cuboRubik->rotarCuboGlobalX(ang);
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) cuboRubik->rotarCuboGlobalX(-ang);
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) cuboRubik->rotarCuboGlobalY(ang);
}

// =========================================================================
// FUNCIONES DEL SISTEMA DE JUEGO (Shooting, Balas, Asteroides, Colisiones)
// =========================================================================

// Manejo de disparo dual desde la cabina de la nave
void handleShooting(GLFWwindow* window, float currentFrame) {
    if (gameOver) return;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) return;
    if (currentFrame - tiempoUltimoDisparo <= FIRE_RATE) return;

    vec3 fwd = spaceship.getForward();
    vec3 right = spaceship.getRight();
    vec3 pos = spaceship.getPosition();

    vec3 leftBarrel  = pos + fwd * 0.5f + right * 0.2f;
    vec3 rightBarrel = pos + fwd * 0.5f - right * 0.2f;

    listaBalas.push_back(Bullet(leftBarrel, fwd));
    listaBalas.push_back(Bullet(rightBarrel, fwd));
    tiempoUltimoDisparo = currentFrame;
}

// Actualizar posiciones de balas y eliminar las que expiran
void updateBullets(float deltaTime) {
    for (size_t i = 0; i < listaBalas.size(); ) {
        listaBalas[i].update(deltaTime);
        if (listaBalas[i].isExpired()) {
            listaBalas.erase(listaBalas.begin() + i);
        } else {
            i++;
        }
    }
}

// Generar asteroides aleatorios con tamaños variables y velocidad escalada
void spawnAsteroids(float currentFrame) {
    if (currentFrame - tiempoUltimoAsteroide <= frecuenciaSpawn) return;

    float spawnX = ((float)(rand() % 40) - 20.0f);
    float spawnY = ((float)(rand() % 20) - 10.0f);
    vec3 puntoOrigen(spawnX, spawnY, -50.0f);
    vec3 puntoDestino = spaceship.getPosition();

    // Tamano aleatorio: 30% small (0.5), 50% med (1.0), 20% large (1.5)
    float sizeRoll = (float)(rand() % 100) / 100.0f;
    float tamanoAleatorio;
    if (sizeRoll < 0.3f)      tamanoAleatorio = 0.5f;
    else if (sizeRoll < 0.8f) tamanoAleatorio = 1.0f;
    else                      tamanoAleatorio = 1.5f;

    Asteroid nuevoAsteroide(puntoOrigen, puntoDestino, tamanoAleatorio);
    nuevoAsteroide.speed = 4.5f / tamanoAleatorio;
    listaAsteroides.push_back(nuevoAsteroide);

    tiempoUltimoAsteroide = currentFrame;
}

// Actualizar, dibujar y gestionar colisiones de todos los asteroides
void processAsteroids(float deltaTime, unsigned int shaderProgram) {
	GLint ourTextureLoc =
    glGetUniformLocation(shaderProgram, "ourTexture");
    glUniform1i(ourTextureLoc, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, asteroideTexID);

    for (size_t i = 0; i < listaAsteroides.size(); ) {
        listaAsteroides[i].update(deltaTime);
        listaAsteroides[i].draw(shaderProgram);

        bool removed = false;

        // Colision nave-asteroide
        if (!gameOver && invincibleTimer <= 0.0f) {
            if (helper::checkSphereCollision(
                    spaceship.getPosition(), spaceship.getCollisionRadius(),
                    listaAsteroides[i].position, listaAsteroides[i].getCollisionRadius())) {
                vidas--;
                std::cout << "[COLISION] Impacto! Vidas restantes: " << vidas << std::endl;
                invincibleTimer = INVINCIBLE_DURATION;
                listaAsteroides.erase(listaAsteroides.begin() + i);
                removed = true;
                if (vidas <= 0) {
                    gameOver = true;
                    std::cout << "========================================" << std::endl;
                    std::cout << "  GAME OVER - Has perdido todas las vidas" << std::endl;
                    std::cout << "  Presiona R para reiniciar" << std::endl;
                    std::cout << "========================================" << std::endl;
                }
            }
        }

        // Colision bala-asteroide
        if (!removed) {
            for (size_t j = 0; j < listaBalas.size(); ) {
                if (helper::checkSphereCollision(
                        listaBalas[j].position, listaBalas[j].getCollisionRadius(),
                        listaAsteroides[i].position, listaAsteroides[i].getCollisionRadius())) {
                    listaAsteroides[i].takeDamage(1);
                    listaBalas.erase(listaBalas.begin() + j);
                    if (listaAsteroides[i].isDestroyed()) {
                        asteroidesDestruidos++;
                        std::cout << "[DISPARO] Asteroide destruido! HP: " << listaAsteroides[i].maxHitPoints << std::endl;
                        listaAsteroides.erase(listaAsteroides.begin() + i);
                        removed = true;
                    }
                    break;
                } else {
                    j++;
                }
            }
        }

        // Eliminar asteroides fuera de rango
        if (!removed) {
            if (listaAsteroides[i].position.z > 2.0f) {
                asteroidesDestruidos++;
                listaAsteroides.erase(listaAsteroides.begin() + i);
            } else {
                i++;
            }
        }
    }
}

// Dibujar todas las balas activas con textura transparente (vertex color)
void drawBullets(unsigned int shaderProgram) {
	GLint ourTextureLoc =
    glGetUniformLocation(shaderProgram, "ourTexture");
    glUniform1i(ourTextureLoc, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bulletTexID);
    for (auto& b : listaBalas) {
        b.draw(shaderProgram);
    }
}

// Callback del mouse: controla hacia dónde mira la cabina de tu nave espacial
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (gameOver) return; // No permitir rotacion si el juego termino
    float xf = (float)xpos;
    float yf = (float)ypos;

    if (firstMouse) {
        lastMouseX = xf;
        lastMouseY = yf;
        firstMouse = false;
    }

    float dx = xf - lastMouseX;  
    float dy = lastMouseY - yf;  // Invertido (arriba es positivo)
    lastMouseX = xf;
    lastMouseY = yf;

    spaceship.yaw -= dx * mouseSensitivity;
    spaceship.pitch += dy * mouseSensitivity;

    // Limitar el cabeceo (Pitch) para evitar romper la matriz de vista (Gimbal Lock)
    if (spaceship.pitch > 89.0f) spaceship.pitch = 89.0f;
    if (spaceship.pitch < -89.0f) spaceship.pitch = -89.0f;
}