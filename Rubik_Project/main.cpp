/*
UCSP
COMPUTACION GRAFICA - 2026-I

- ALEXANDER BAYLON
- CRISTIAN MELLADO
- JOSE VILCA
- WALTER VALDIVIA

Proyecto CUBO RUBIK
main.cpp
*/

#include "rubik.h"
#include "helper.h" 
#include "camera.h"
#include "skybox.h"
#include "spaceship.h"
#include "blackhole.h"
#include "asteroid.h"

#include <vector>
#include <cstdlib> // Para rand() y srand()

// VARIABLES GLOBALES - SISTEMA DE ASTEROIDES
std::vector<Asteroid> listaAsteroides;
float tiempoUltimoAsteroide = 0.0f;
float frecuenciaSpawn = 1.5f; // Generar un asteroide cada 1.5 segundos
unsigned int asteroideTexID;  // ID para la textura de los asteroides

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

// Vertex shader with normals and lighting support
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "layout (location = 2) in vec3 aColor;\n"
    "layout (location = 3) in vec2 aTexCoord;\n"

    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "out vec3 ourColor;\n"
    "out vec2 TexCoord;\n"

    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform mat3 normalMatrix;\n"

    "void main()\n"
    "{\n"
    "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
    "   Normal = normalize(normalMatrix * aNormal);\n"
    "   ourColor = aColor;\n"
    "   TexCoord = aTexCoord;\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "}\0";

// Fragment shader with ambient + directional + point lighting
const char *fragmentShaderTexSource = "#version 330 core\n"
    "out vec4 FragColor;\n"

    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "in vec3 ourColor;\n"
    "in vec2 TexCoord;\n"

    "uniform sampler2D ourTexture;\n"

    // Directional light
    "uniform vec3 lightDir;\n"
    "uniform vec3 lightColor;\n"
    "uniform float lightIntensity;\n"

    // Ambient
    "uniform float ambientStrength;\n"
    "uniform vec3 ambientColor;\n"

    // Point lights (max 2)
    "uniform vec3 pointLightPos[2];\n"
    "uniform vec3 pointLightColor[2];\n"
    "uniform float pointLightIntensity[2];\n"
    "uniform float pointLightConstant[2];\n"
    "uniform float pointLightLinear[2];\n"
    "uniform float pointLightQuadratic[2];\n"
    "uniform int numPointLights;\n"

    // View position for specular
    "uniform vec3 viewPos;\n"

    "vec3 calcDirLight(vec3 baseColor)\n"
    "{\n"
    "   float diff = max(dot(Normal, normalize(-lightDir)), 0.0);\n"
    "   return diff * lightColor * lightIntensity * baseColor;\n"
    "}\n"

    "vec3 calcPointLight(int idx, vec3 baseColor)\n"
    "{\n"
    "   vec3 L = pointLightPos[idx] - FragPos;\n"
    "   float dist = length(L);\n"
    "   L = normalize(L);\n"
    "   float diff = max(dot(Normal, L), 0.0);\n"
    "   float atten = 1.0 / (pointLightConstant[idx] +\n"
    "                        pointLightLinear[idx] * dist +\n"
    "                        pointLightQuadratic[idx] * dist * dist);\n"
    "   return diff * pointLightColor[idx] * pointLightIntensity[idx] * atten * baseColor;\n"
    "}\n"

    "void main()\n"
    "{\n"
    "   // Base color from texture + vertex color\n"
    "   vec4 texColor = texture(ourTexture, TexCoord);\n"
    "   vec3 baseColor;\n"
    "   if(texColor.a < 0.1) {\n"
    "       baseColor = ourColor;\n"
    "   } else {\n"
    "       baseColor = mix(ourColor, texColor.rgb, 0.7);\n"
    "   }\n"

    "   // Ambient\n"
    "   vec3 result = ambientStrength * ambientColor * baseColor;\n"

    "   // Directional light\n"
    "   result += calcDirLight(baseColor);\n"

    "   // Point lights\n"
    "   for(int i = 0; i < numPointLights && i < 2; i++)\n"
    "       result += calcPointLight(i, baseColor);\n"

    "   FragColor = vec4(result, 1.0);\n"
    "}\0";


int main()
{
    // glfw: initialize and configure
    // ------------------------------
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

    // ---------------------------------------
    // texturas
    // ---------------------------------------
    unsigned int ourTextureID;

    //Load each texture and check for errors
    ourTextureID = loadTexture("assets/cubitoBorder.png");

    // Verify all textures loaded successfully
    if (ourTextureID) {
        std::cout << "All textures loaded successfully!" << std::endl;
    } else {
        std::cout << "Failed to load one or more textures!" << std::endl;
        // Handle error - maybe exit program or use default textures
    }

    // Load spaceship texture
    unsigned int spaceshipTexID = loadTexture("assets/spaceshiptexture.bmp");

    // Load asteroid texture
    asteroideTexID = loadTexture("assets/asteroide.jpg");
    if (asteroideTexID == 0) {
        std::cout << "[WARNING] Failed to load asteroid texture. Asteroids will use vertex colors only." << std::endl;
    }

    // Init agujero negro (reemplaza skybox de estrellas)
    BlackHole blackhole;
    blackhole.init();
	blackhole.position[0] = 20.0f;  // mueve el BH lejos del Rubik en X
	blackhole.bhRadius     = 2.0f;
	blackhole.diskInner    = 3.0f;
	blackhole.diskOuter    = 8.0f;
	blackhole.diskParticles = 500;
	blackhole.diskAlpha = 0.5;
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
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
	// check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::0::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // delete used Shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    cuboRubik->init();
    cuboRubik->printMenu();

    // tell opengl for each sampler to which texture it belongs
    glUseProgram(shaderProgram);

    int ourTextureLoc = glGetUniformLocation(shaderProgram, "ourTexture");

    // Set texture units
    glUniform1i(ourTextureLoc, 0); // Texture unit 0

    // --- PHASE 1+2: LIGHTING UNIFORM LOCATIONS ---
    lightDirLoc         = glGetUniformLocation(shaderProgram, "lightDir");
    lightColorLoc       = glGetUniformLocation(shaderProgram, "lightColor");
    lightIntensityLoc   = glGetUniformLocation(shaderProgram, "lightIntensity");
    ambientStrengthLoc  = glGetUniformLocation(shaderProgram, "ambientStrength");
    ambientColorLoc     = glGetUniformLocation(shaderProgram, "ambientColor");
    viewPosLoc          = glGetUniformLocation(shaderProgram, "viewPos");
    normalMatrixLoc     = glGetUniformLocation(shaderProgram, "normalMatrix");
    numPointLightsLoc   = glGetUniformLocation(shaderProgram, "numPointLights");
    for (int i = 0; i < 2; i++) {
        std::string idx = "[" + std::to_string(i) + "]";
        pointLightPosLoc[i]        = glGetUniformLocation(shaderProgram, ("pointLightPos" + idx).c_str());
        pointLightColorLoc[i]      = glGetUniformLocation(shaderProgram, ("pointLightColor" + idx).c_str());
        pointLightIntensityLoc[i]  = glGetUniformLocation(shaderProgram, ("pointLightIntensity" + idx).c_str());
        pointLightConstantLoc[i]   = glGetUniformLocation(shaderProgram, ("pointLightConstant" + idx).c_str());
        pointLightLinearLoc[i]     = glGetUniformLocation(shaderProgram, ("pointLightLinear" + idx).c_str());
        pointLightQuadraticLoc[i]  = glGetUniformLocation(shaderProgram, ("pointLightQuadratic" + idx).c_str());
    }

    // --- PHASE 1: DIRECTIONAL LIGHT SETUP ---
    glUniform3f(lightDirLoc,        0.0f, 0.707f, 0.707f);
    glUniform3f(lightColorLoc,      1.0f, 0.95f, 0.8f);
    glUniform1f(lightIntensityLoc,  1.0f);
    glUniform1f(ambientStrengthLoc, 0.15f);
    glUniform3f(ambientColorLoc,    0.1f, 0.1f, 0.15f);

    // --- PHASE 2: POINT LIGHT DEFAULTS ---
    glUniform1i(numPointLightsLoc, 1);
    // Point light 0 = Black Hole (position updated per frame)
    glUniform3f(pointLightPosLoc[0],       0.0f, 0.0f, 0.0f);
    glUniform3f(pointLightColorLoc[0],     1.0f, 0.35f, 0.05f);
    glUniform1f(pointLightIntensityLoc[0], 2.5f);
    glUniform1f(pointLightConstantLoc[0],  1.0f);
    glUniform1f(pointLightLinearLoc[0],    0.09f);
    glUniform1f(pointLightQuadraticLoc[0], 0.032f);
    // Point light 1 = Distant blue star (optional fill)
    glUniform3f(pointLightPosLoc[1],       -50.0f, 30.0f, -20.0f);
    glUniform3f(pointLightColorLoc[1],     0.3f, 0.5f, 0.8f);
    glUniform1f(pointLightIntensityLoc[1], 0.5f);
    glUniform1f(pointLightConstantLoc[1],  1.0f);
    glUniform1f(pointLightLinearLoc[1],    0.045f);
    glUniform1f(pointLightQuadraticLoc[1], 0.0075f);

    std::cout << "[LIGHTING] Phase 1+2: Directional + Point lights initialized." << std::endl;

    // point and line sizes
    glPointSize(10.f);
    glLineWidth(5.f);

    // cam variables
    float cameraSpeed = 0.05f;
    bool unaPrueba=true;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

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
        if (camera.isFollowMode()) {
            camera.updateFollow(spaceship.getPosition(), spaceship.yaw, spaceship.pitch, deltaTime);
        }

        // ---- ANIMATION UPDATE ----
        cuboRubik->update_animation(deltaTime);
        // --------------------------

        viewLoc = glGetUniformLocation(shaderProgram, "view");
        projLoc = glGetUniformLocation(shaderProgram, "projection");
        modelLoc = glGetUniformLocation(shaderProgram, "model");
        
        // Elegir vista: seguimiento (follow) u orbital, segun el modo activo
        matriz4x4 viewMatrix = camera.isFollowMode() ? camera.getFollowViewMatrix() : camera.getViewMatrix();

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspectRatio = (float)width / (float)height;
        matriz4x4 projMatrix = camera.getPerspectiveMatrix(aspectRatio);
        
        // 1. DIBUJAR ENTORNO: AGUJERO NEGRO (Nebulosa + Esfera + Disco de Acreción)
        blackhole.update(deltaTime);
        blackhole.draw(viewMatrix, projMatrix);

        // 2. ACTIVAR SHADER PRINCIPAL CON ILUMINACIÓN PARA LOS OBJETOS 3D
        glUseProgram(shaderProgram);

        // --- PHASE 1+2: PER-FRAME LIGHTING UNIFORMS ---
        vec3 shipPos = spaceship.getPosition();
        vec3 bhPos(blackhole.position[0], blackhole.position[1], blackhole.position[2]);

        // View position
        vec3 viewPosVal = camera.isFollowMode()
            ? shipPos + vec3(0.0f, 3.0f, 8.0f)
            : vec3(0.0f, 0.0f, 15.0f);
        glUniform3f(viewPosLoc, viewPosVal.x, viewPosVal.y, viewPosVal.z);

        // Phase 2: Black hole as point light 0
        glUniform3f(pointLightPosLoc[0], bhPos.x, bhPos.y, bhPos.z);
        float distToBH = helper::length(shipPos - bhPos);
        float proximityBoost = 1.0f + 4.0f / (1.0f + distToBH * 0.05f);
        glUniform1f(pointLightIntensityLoc[0], 2.5f * proximityBoost);

        // Upload view/projection matrices
        glUniformMatrix4fv(viewLoc, 1, GL_TRUE, viewMatrix.mat.data());
        glUniformMatrix4fv(projLoc, 1, GL_TRUE, projMatrix.mat.data());

        // --- CUBO RUBIK ---
        matriz4x4 modelMatrixCube; // Identity — vertices are CPU-transformed
        std::array<float, 9> normalMatrixCube = {1,0,0, 0,1,0, 0,0,1};
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, modelMatrixCube.mat.data());
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_TRUE, normalMatrixCube.data());
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

        // --- ASTEROIDS RENDER ---
        glUniform1i(ourTextureLoc, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, asteroideTexID);

        for (size_t i = 0; i < listaAsteroides.size(); ) {
            listaAsteroides[i].update(deltaTime);

            // Upload model + normal matrix for lighting
            matriz4x4 astModel = listaAsteroides[i].getModelMatrix();
            std::array<float, 9> astNormalMat = helper::extractNormalMatrix(astModel);
            glUniformMatrix4fv(modelLoc, 1, GL_TRUE, astModel.mat.data());
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_TRUE, astNormalMat.data());

            listaAsteroides[i].drawRaw(shaderProgram);

            if (listaAsteroides[i].position.z > 2.0f) {
                listaAsteroides.erase(listaAsteroides.begin() + i);
            } else {
                i++;
            }
        }

        // --- SPACESHIP RENDER ---
        glUniform1i(ourTextureLoc, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, spaceshipTexID);

        matriz4x4 shipModel = spaceship.getModelMatrix();
        std::array<float, 9> shipNormalMat = helper::extractNormalMatrix(shipModel);
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, shipModel.mat.data());
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_TRUE, shipNormalMat.data());

        spaceship.drawRaw(shaderProgram);
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

// Callback del mouse: controla hacia dónde mira la cabina de tu nave espacial
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
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