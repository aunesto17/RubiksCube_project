/*
UCSP
COMPUTACION GRAFICA - 2026-I

- ALEXANDER BAYLON
- CRISTIAN MELLADO
- JOSE VILCA
- WALTER VALDIVIA

<<<<<<< HEAD
Proyecto CUBO RUBIK + ESCAPE ESPACIAL DE ASTEROIDES Y AGUJERO NEGRO
main_final_unificado.cpp
=======
Proyecto CUBO RUBIK
main.cpp
>>>>>>> origin/master
*/

#include "rubik.h"
#include "helper.h" 
#include "camera.h"
#include "skybox.h"
#include "spaceship.h"
#include "blackhole.h"
<<<<<<< HEAD
#include "asteroid.h"

#include <vector>
#include <cstdlib> // Para rand() y srand()

// VARIABLES GLOBALES - SISTEMA DE ASTEROIDES
std::vector<Asteroid> listaAsteroides;
float tiempoUltimoAsteroide = 0.0f;
float frecuenciaSpawn = 1.5f; // Generar un asteroide cada 1.5 segundos
unsigned int asteroideTexID;  // ID para la textura de los asteroides
=======

>>>>>>> origin/master

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
<<<<<<< HEAD
=======
    // ------------------------------
>>>>>>> origin/master
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);  // Request 24-bit depth buffer

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
<<<<<<< HEAD

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Rubik + Space Escape (Asteroids & Black Hole)", NULL, NULL);
=======
    // --------------------
    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Rubik + Black Hole", NULL, NULL);
>>>>>>> origin/master
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

<<<<<<< HEAD
    // CARGA DE TEXTURAS Y MALLAS
    unsigned int ourTextureID = loadTexture("assets/cubitoBorder.png");

=======
    // ---------------------------------------
    // texturas
    // ---------------------------------------
    unsigned int ourTextureID;

    //Load each texture and check for errors
    ourTextureID = loadTexture("assets/cubitoBorder.png");

    // Verify all textures loaded successfully
>>>>>>> origin/master
    if (ourTextureID) {
        std::cout << "All textures loaded successfully!" << std::endl;
    } else {
        std::cout << "Failed to load one or more textures!" << std::endl;
<<<<<<< HEAD
    }

    // Cargar textura de la nave
    unsigned int spaceshipTexID = loadTexture("assets/spaceshiptexture.bmp");

    // Cargar textura y malla del asteroide
    asteroideTexID = loadTexture("assets/asteroide.jpg");
    Asteroid::loadMesh("assets/asteroide.3ds");

    // Inicializar Agujero Negro (Reemplaza al Skybox estándar)
    BlackHole blackhole;
    blackhole.init();
	blackhole.position[0] = 20.0f;  // Mueve el BH lejos del Rubik en X
=======
        // Handle error - maybe exit program or use default textures
    }

    // Load spaceship texture
    unsigned int spaceshipTexID = loadTexture("assets/spaceshiptexture.bmp");

    // Init agujero negro (reemplaza skybox de estrellas)
    BlackHole blackhole;
    blackhole.init();
	blackhole.position[0] = 20.0f;  // mueve el BH lejos del Rubik en X
>>>>>>> origin/master
	blackhole.bhRadius     = 2.0f;
	blackhole.diskInner    = 3.0f;
	blackhole.diskOuter    = 8.0f;
	blackhole.diskParticles = 500;
	blackhole.diskAlpha = 0.5;
<<<<<<< HEAD

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

=======
    // Init spaceship
    if (!spaceship.load("assets/spaceship.3DS")) {
        std::cout << "Warning: Failed to load spaceship model. Continuing without spaceship." << std::endl;
    }
    spaceship.setPosition(vec3(8.0f, 0.0f, 0.0f));

    // build and compile our shader program
    // ------------------------------------
    // VERTEX SHADER
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::0::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // FRAGMENT SHADER   
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderTexSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // LINK SHADERS and form a SHADER PROGRAM
>>>>>>> origin/master
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
<<<<<<< HEAD

=======
	// check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::0::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // delete used Shaders
>>>>>>> origin/master
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    cuboRubik->init();
    cuboRubik->printMenu();

<<<<<<< HEAD
    glUseProgram(shaderProgram);
    int ourTextureLoc = glGetUniformLocation(shaderProgram, "ourTexture");
    glUniform1i(ourTextureLoc, 0); // Textura en unidad 0

    glPointSize(10.f);
    glLineWidth(5.f);

    // Sincronizar tiempo de inicio
    lastFrame = glfwGetTime();

    // RENDER LOOP UNIFICADO
=======
    // tell opengl for each sampler to which texture it belongs
    glUseProgram(shaderProgram);

    int ourTextureLoc = glGetUniformLocation(shaderProgram, "ourTexture");

    // Set texture units
    glUniform1i(ourTextureLoc, 0); // Texture unit 0

    // point and line sizes
	glPointSize(10.f);
    glLineWidth(5.f);

    // cam variables
    float cameraSpeed = 0.05f;
    bool unaPrueba=true;

    // render loop
    // -----------
>>>>>>> origin/master
    while (!glfwWindowShouldClose(window))
    {
        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

<<<<<<< HEAD
        // Captura de controles continuos (Cámara Orbital, Nave y Rotación del Cubo)
        processInput(window); 

        // Listener de eventos discretos (Teclas de control del Cubo)
        glfwSetKeyCallback(window, key_callback);

        // Limpieza de pantalla
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, 1.0f);

        // Actualización de Animaciones lógicas y de cámara
        camera.updateCameraAnimation(deltaTime);
        camera.setTarget(spaceship.getPosition());

=======
        processInput(window); // para eventos continuos(nave o camara)

        // input
        glfwSetKeyCallback(window, key_callback);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // render
        // ------
		// color de fondo
		glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, 1.0f);

        camera.updateCameraAnimation(deltaTime);

        camera.setTarget(spaceship.getPosition());

        // Actualizar camara de seguimiento si el modo follow esta activo
>>>>>>> origin/master
        if (camera.isFollowMode()) {
            camera.updateFollow(spaceship.getPosition(), spaceship.yaw, spaceship.pitch, deltaTime);
        }

<<<<<<< HEAD
        cuboRubik->update_animation(deltaTime);

        // Obtener locations para transformaciones de matrices del Shader principal
        viewLoc = glGetUniformLocation(shaderProgram, "view");
        projLoc = glGetUniformLocation(shaderProgram, "projection");
        modelLoc = glGetUniformLocation(shaderProgram, "model");
        ourTextureLoc = glGetUniformLocation(shaderProgram, "ourTexture");
        
        // Seleccionar matriz de vista activa (Seguimiento o Libre)
=======
        // ---- ANIMATION UPDATE ----
        cuboRubik->update_animation(deltaTime);
        // --------------------------

        viewLoc = glGetUniformLocation(shaderProgram, "view");
        projLoc = glGetUniformLocation(shaderProgram, "projection");
        modelLoc = glGetUniformLocation(shaderProgram, "model");
        
        // Elegir vista: seguimiento (follow) u orbital, segun el modo activo
>>>>>>> origin/master
        matriz4x4 viewMatrix = camera.isFollowMode() ? camera.getFollowViewMatrix() : camera.getViewMatrix();

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspectRatio = (float)width / (float)height;
<<<<<<< HEAD
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

        // GENERACIÓN DINÁMICA DE ASTEROIDES
        if (currentFrame - tiempoUltimoAsteroide > frecuenciaSpawn) {
            float spawnX = ((float)(rand() % 40) - 20.0f); 
            float spawnY = ((float)(rand() % 20) - 10.0f); 
            vec3 puntoOrigen(spawnX, spawnY, -50.0f); // Nacen al fondo

            vec3 puntoDestino = spaceship.getPosition(); // Su objetivo es interceptar la nave
            float tamanoAleatorio = 1.0f;

            Asteroid nuevoAsteroide(puntoOrigen, puntoDestino, tamanoAleatorio);
            listaAsteroides.push_back(nuevoAsteroide);

            tiempoUltimoAsteroide = currentFrame; 
        }

        // ACTUALIZACIÓN Y RENDERIZADO DE LOS ASTEROIDES
        glUniform1i(ourTextureLoc, 0); 
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, asteroideTexID); // Textura de roca espacial activa

        for (size_t i = 0; i < listaAsteroides.size(); ) {
            listaAsteroides[i].update(deltaTime);
            listaAsteroides[i].draw(shaderProgram);

            // Si el asteroide ya rebasó la nave, se elimina para optimizar recursos
            if (listaAsteroides[i].position.z > 2.0f) {
                listaAsteroides.erase(listaAsteroides.begin() + i);
            } else {
                i++; 
            }
        }

        // RENDERIZADO DE LA NAVE JUGADORA
        glUniform1i(ourTextureLoc, 0); 
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, spaceshipTexID); // Cambiar a la textura metálica de la nave
        
        spaceship.draw(shaderProgram, viewMatrix, projMatrix);

        // Swap de buffers e IO eventos
=======
    
        matriz4x4 projMatrix = camera.getPerspectiveMatrix(aspectRatio);
        
        glUseProgram(shaderProgram);

        // Send matrices to shader
        glUniformMatrix4fv(viewLoc, 1, GL_TRUE, viewMatrix.mat.data());
        glUniformMatrix4fv(projLoc, 1, GL_TRUE, projMatrix.mat.data());
        
        matriz4x4 modelMatrix;
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, modelMatrix.mat.data()); 

        // Bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ourTextureID);  // Top/Bottom texture
        
        cuboRubik->draw(shaderProgram);

        // Agujero negro: skybox nebulosa + esfera negra + disco de acrecion
        blackhole.update(deltaTime);
        blackhole.draw(viewMatrix, projMatrix);

        // Restaurar estado para el shader del Rubik/nave
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(viewLoc,  1, GL_TRUE, viewMatrix.mat.data());
        glUniformMatrix4fv(projLoc,  1, GL_TRUE, projMatrix.mat.data());

        // Re-bind spaceship texture before drawing spaceship
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, spaceshipTexID);
        spaceship.draw(shaderProgram, viewMatrix, projMatrix);

        //camera.getViewMatrix(viewMatrix.m);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
>>>>>>> origin/master
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

<<<<<<< HEAD
    // Liberación de memoria de shaders al cerrar
    glDeleteProgram(shaderProgram);
=======
    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteProgram(shaderProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
>>>>>>> origin/master
    glfwTerminate();
    return 0;
}

<<<<<<< HEAD
// Eventos discretos de teclado (Clicks instantáneos)
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        isClockwise = !isClockwise;
        std::cout << "Direccion de rotacion: " << (isClockwise ? "Horaria" : "Antihoraria ' ") << std::endl;
    }

    // Control de Capas de Rubik
    if (!cuboRubik->is_animation_running() && !cuboRubik->isSequenceRunning()) {
        if (key == GLFW_KEY_T && action == GLFW_PRESS) cuboRubik->rotateU(isClockwise);
        if (key == GLFW_KEY_R && action == GLFW_PRESS) cuboRubik->rotateL(isClockwise); 
        if (key == GLFW_KEY_F && action == GLFW_PRESS) cuboRubik->rotateF(isClockwise); 
        if (key == GLFW_KEY_G && action == GLFW_PRESS) cuboRubik->rotateR(isClockwise); 
        if (key == GLFW_KEY_Y && action == GLFW_PRESS) cuboRubik->rotateB(isClockwise); 
        if (key == GLFW_KEY_H && action == GLFW_PRESS) cuboRubik->rotateD(isClockwise); 
        
        if (key == GLFW_KEY_V && action == GLFW_PRESS) cuboRubik->rotateSV(isClockwise); 
        if (key == GLFW_KEY_B && action == GLFW_PRESS) cuboRubik->rotateSH(isClockwise); 
        if (key == GLFW_KEY_N && action == GLFW_PRESS) cuboRubik->rotateSS(isClockwise); 
    }
=======
// mejor forma de detectar eventos de teclado y que la reaccion de pollEvents sea eficiente.
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //float currentFrame = glfwGetTime();
    //deltaTime = currentFrame - lastFrame;
    //lastFrame = currentFrame;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    
    // -------------- direccion de rotaciones ------------------
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        isClockwise = !isClockwise;
        std::cout << "Direccion de rotacion: " 
                  << (isClockwise ? "Horaria" : "Antihoraria ' ") 
                  << std::endl;
    }
    // -------------- MOVIMIENTO CAMARA ------------------
    // (WASD/QE camera movement is now polled in the main loop)
    // -------------------------------------------------
    // ---------------- CAMADAS ------------------
    // 
    // Block face/slice rotation inputs while an animation is running.
    // This prevents queuing multiple simultaneous rotations which would
    // corrupt the cube's logical and visual state.
    //
    if (!cuboRubik->is_animation_running() && !cuboRubik->isSequenceRunning()) {
        // rotate cube faces
        if (key == GLFW_KEY_T && action == GLFW_PRESS) {
            cuboRubik->rotateU(isClockwise);
        }
        if (key == GLFW_KEY_R && action == GLFW_PRESS) {
            cuboRubik->rotateL(isClockwise); 
        }
        if (key == GLFW_KEY_F && action == GLFW_PRESS) {
            cuboRubik->rotateF(isClockwise); 
        }
        if (key == GLFW_KEY_G && action == GLFW_PRESS) {
            cuboRubik->rotateR(isClockwise); 
        }
        if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
            cuboRubik->rotateB(isClockwise); 
        }
        if (key == GLFW_KEY_H && action == GLFW_PRESS) {
            cuboRubik->rotateD(isClockwise); 
        }
        // rotate cube slices
        if (key == GLFW_KEY_V && action == GLFW_PRESS)
        {
            cuboRubik->rotateSV(isClockwise); // clockwise
        }
        if (key == GLFW_KEY_B && action == GLFW_PRESS)
        {
            cuboRubik->rotateSH(isClockwise); // clockwise
        }
        if (key == GLFW_KEY_N && action == GLFW_PRESS)
        {
            cuboRubik->rotateSS(isClockwise); // clockwise
        }
    }
    // If animation is running and a rotation key is pressed, print a debug message
>>>>>>> origin/master
    else if (action == GLFW_PRESS && (
        key == GLFW_KEY_T || key == GLFW_KEY_R || key == GLFW_KEY_F || 
        key == GLFW_KEY_G || key == GLFW_KEY_Y || key == GLFW_KEY_H ||
        key == GLFW_KEY_V || key == GLFW_KEY_B || key == GLFW_KEY_N
    )) {
        std::cout << "[INPUT] Rotation key ignored: animation in progress." << std::endl;
    }
<<<<<<< HEAD

=======
    // -------------------------------------------------

    // 2. MOVIMIENTO DE LA NAVE
    // (Arrow key spaceship movement is now polled in the main loop)

    // 3. ROTACION CONTINUA DEL CUBO GLOBAL
    // (Z/X/C cube rotation is now polled in the main loop)

    // reset cube - allowed during animation
>>>>>>> origin/master
    if (key == GLFW_KEY_K && action == GLFW_PRESS) {
        cuboRubik->cancelSequence();
        cuboRubik->resetRubik();
    }  
    
<<<<<<< HEAD
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
    // Si NO estamos siguiendo la nave, las teclas WASD controlan el modo orbital libre
=======
    // change display mode to lines only - allowed during animation
    if(key == GLFW_KEY_I && action == GLFW_PRESS){
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    // change display mode to fill only - allowed during animation
    if(key == GLFW_KEY_O && action == GLFW_PRESS){
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    // change display mode to points only - allowed during animation
    if(key == GLFW_KEY_0 && action == GLFW_PRESS){
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }
    // mostrar menu
    if(key == GLFW_KEY_P && action == GLFW_PRESS){
        cuboRubik->printMenu();
    }
    // -------------- SOLVER & SCRAMBLE ------------------
    if (key == GLFW_KEY_J && action == GLFW_PRESS) {
        cuboRubik->solveRubik();
    }
    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        cuboRubik->scrambleRubik(SCRAMBLE_NUM_MOVES);
    }
    // -------------- SEQUENCE SPEED CONTROL ------------------
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        cuboRubik->setSequenceSpeed(1.0f);
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        cuboRubik->setSequenceSpeed(2.0f);
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
        cuboRubik->setSequenceSpeed(4.0f);
    }
    if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
        cuboRubik->setSequenceSpeed(8.0f);
    }
    if (key == GLFW_KEY_5 && action == GLFW_PRESS) {
        cuboRubik->setSequenceSpeed(16.0f);
    }
    if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS) {
        cuboRubik->setSequenceSpeed(cuboRubik->getSequenceSpeed() + 1.0f);
    }
    if (key == GLFW_KEY_MINUS && action == GLFW_PRESS) {
        cuboRubik->setSequenceSpeed(cuboRubik->getSequenceSpeed() - 1.0f);
    }
    // -------------------------------------------------
    // change background color - allowed during animation
    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        backgroundColor = getRandomColor();}

    // Alternar modo camara: orbital / seguimiento
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        camera.toggleFollowMode();
    }

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    // Camara orbital: WASD orbita + QE zoom (solo en modo orbital)
>>>>>>> origin/master
    if (!camera.isFollowMode()) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.moveForward(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.moveBackward(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.moveLeft(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.moveRight(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.zoomIn(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.zoomOut(deltaTime);
    }
<<<<<<< HEAD
    
    // Movimiento fluido de propulsión de la Nave (Flechas del Teclado)
=======
    // Propulsion de la nave: flechas arriba/abajo mueven, izquierda/derecha rotan yaw
>>>>>>> origin/master
    const float shipSpeed = 5.0f;
    float step = shipSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) spaceship.moveForward(step);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) spaceship.moveBackward(step);
<<<<<<< HEAD
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) spaceship.yaw += 90.0f * deltaTime;  
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) spaceship.yaw -= 90.0f * deltaTime; 

    // Rotación Global del Cubo Rubik mediante matrices (Z, X, C)
=======
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) spaceship.yaw += 90.0f * deltaTime;   // girar a la izquierda
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) spaceship.yaw -= 90.0f * deltaTime;  // girar a la derecha
    // Rotacion global del cubo Rubik: Z / X / C
>>>>>>> origin/master
    const float rotSpeed = 90.0f;
    float ang = rotSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) cuboRubik->rotarCuboGlobalX(ang);
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) cuboRubik->rotarCuboGlobalX(-ang);
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) cuboRubik->rotarCuboGlobalY(ang);
}

<<<<<<< HEAD
// Callback del mouse: controla hacia dónde mira la cabina de tu nave espacial
=======
// Callback del mouse: calcula el delta de movimiento y aplica rotacion yaw/pitch a la nave
>>>>>>> origin/master
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    float xf = (float)xpos;
    float yf = (float)ypos;

<<<<<<< HEAD
=======
    // En el primer evento, solo guardar la posicion sin aplicar delta
>>>>>>> origin/master
    if (firstMouse) {
        lastMouseX = xf;
        lastMouseY = yf;
        firstMouse = false;
    }

<<<<<<< HEAD
    float dx = xf - lastMouseX;  
    float dy = lastMouseY - yf;  // Invertido (arriba es positivo)
    lastMouseX = xf;
    lastMouseY = yf;

    spaceship.yaw -= dx * mouseSensitivity;
    spaceship.pitch += dy * mouseSensitivity;

    // Limitar el cabeceo (Pitch) para evitar romper la matriz de vista (Gimbal Lock)
=======
    // Calcular el desplazamiento del mouse desde el frame anterior
    float dx = xf - lastMouseX;  // delta horizontal
    float dy = lastMouseY - yf;  // delta vertical (invertido: mouse arriba = positivo)
    lastMouseX = xf;
    lastMouseY = yf;

    // Aplicar rotacion: dx negado para que mover el mouse a la derecha gire a la derecha
    spaceship.yaw -= dx * mouseSensitivity;
    spaceship.pitch += dy * mouseSensitivity;

    // Limitar pitch para evitar volcadura completa
>>>>>>> origin/master
    if (spaceship.pitch > 89.0f) spaceship.pitch = 89.0f;
    if (spaceship.pitch < -89.0f) spaceship.pitch = -89.0f;
}