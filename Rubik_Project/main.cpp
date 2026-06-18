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
#include <array>   // Para std::array (normal matrices)
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

// SISTEMA DE PARTICULAS PARA EXPLOSIONES de ASTEROIDES
struct Particle {
    vec3 position;
    vec3 velocity;
    float lifetime; // Tiempo de vida restante en segundos
    float maxLifetime; // Tiempo total de vida util
    float size;
	//Asteroid* visual = nullptr; //
	vec3 color;
};

// Lista dinámica de particulas activas en el espacio
std::vector<Particle> listaParticulas;


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

BlackHole blackhole;

// VARIABLES GLOBALES DE CONTROL DE JUEGO por NIVELES
int nivelActual = 1;
float velocidadAsteroideBase = 7.0f; 

// Función auxiliar para reiniciar el estado de la nave al cambiar de nivel o perder
void resetPosicionNave() {
    spaceship.setPosition(vec3(0.0f, 0.0f, -10.0f));
    spaceship.yaw = 0.0f;
    spaceship.pitch = 0.0f;
    listaAsteroides.clear(); // Limpiar rocas viejas para evitar colisiones injustas
    if (listaBalas.size() > 0) listaBalas.clear(); 
}

void emitirExplosion(vec3 origen, float escalaAsteroide) {
    int cantidadParticulas = 20; // Más partículas para que se note la ráfaga
    
    for (int i = 0; i < cantidadParticulas; i++) {
        Particle p;
        p.position = origen;
        
        // Dispersión aleatoria 3D
        float theta = ((float)(rand() % 100) / 100.0f) * 2.0f * 3.141592f;
        float phi = asinf(((float)(rand() % 100) / 100.0f) * 2.0f - 1.0f);
        float velocidad = 4.0f + ((float)(rand() % 100) / 100.0f) * 6.0f; 
        
        p.velocity.x = cosf(phi) * cosf(theta) * velocidad;
        p.velocity.y = sinf(phi) * velocidad;
        p.velocity.z = cosf(phi) * sinf(theta) * velocidad;
        
        p.maxLifetime = 0.6f + ((float)(rand() % 100) / 100.0f) * 0.4f;
        p.lifetime = p.maxLifetime;
        
        // Tamaño en unidades del mundo (visible a simple vista)
        p.size = 0.4f + ((float)(rand() % 100) / 100.0f) * 0.4f;
        
        // Color aleatorio entre Rojo y Amarillo Brillante (Fuego)
        p.color.x = 1.0f;                                     // Mucho Rojo
        p.color.y = 0.3f + ((float)(rand() % 100) / 100.0f) * 0.6f; // Variación de Verde para dar Naranja/Amarillo
        p.color.z = 0.0f;                                     // Nada de Azul
        
        listaParticulas.push_back(p);
    }
}

void processParticles(float deltaTime, unsigned int shaderProgram, GLint normalMatrixLoc) {
    if (listaParticulas.empty()) return;

    // 1. Avisar al shader que pinte un color plano sin textura si tiene la variable
    GLint useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
    if (useTextureLoc != -1) glUniform1i(useTextureLoc, 0);

    for (size_t i = 0; i < listaParticulas.size(); ) {
        listaParticulas[i].lifetime -= deltaTime;
        
        if (listaParticulas[i].lifetime <= 0.0f) {
            listaParticulas.erase(listaParticulas.begin() + i);
            continue;
        }
        
        // Actualizar posición física
        listaParticulas[i].position.x += listaParticulas[i].velocity.x * deltaTime;
        listaParticulas[i].position.y += listaParticulas[i].velocity.y * deltaTime;
        listaParticulas[i].position.z += listaParticulas[i].velocity.z * deltaTime;
        
        // Factor de encogimiento (fade-out de tamaño)
        float factorVida = listaParticulas[i].lifetime / listaParticulas[i].maxLifetime;
        float escalaFinal = listaParticulas[i].size * factorVida;
        
        // Construcción manual de la matriz de modelado (Cubo posicionado y escalado)
        matriz4x4 modelMatrix;
        modelMatrix.mat = {
            escalaFinal, 0.0f,        0.0f,        listaParticulas[i].position.x,
            0.0f,        escalaFinal, 0.0f,        listaParticulas[i].position.y,
            0.0f,        0.0f,        escalaFinal, listaParticulas[i].position.z,
            0.0f,        0.0f,        0.0f,        1.0f
        };
        
        // Inyectamos la matriz de modelado al shader
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_TRUE, modelMatrix.mat.data());
        
        // Hack de Iluminación/Color: Forzamos que el objeto brille ignorando las sombras
        // Pasamos el color directamente a los coeficientes de reflexión difusa/ambiental si el shader los tiene
        GLint materialColorLoc = glGetUniformLocation(shaderProgram, "material.diffuse");
        if (materialColorLoc != -1) {
            glUniform3f(materialColorLoc, listaParticulas[i].color.x, listaParticulas[i].color.y, listaParticulas[i].color.z);
        }

        // DIBUJAR: Reutilizamos el cubo del Rubik que sabemos que tu OpenGL renderiza perfectamente sin romperse
        // Si tienes una variable VAO para el cubo o una clase Cubo, usa su bind aquí:
        // En tu while usas: cuboRubik->draw(shaderProgram);
        // Así que podemos forzar el dibujado usando la geometría que ya existe en el buffer:
        cuboRubik->draw(shaderProgram); 
        
        i++;
    }

    // Restaurar texturas para los siguientes objetos del juego
    if (useTextureLoc != -1) glUniform1i(useTextureLoc, 1);
}

// Función para actualizar las estadísticas según el nivel actual
void actualizarDificultadNivel() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "        ¡BIENVENIDO AL NIVEL " << nivelActual << "! " << std::endl;
    
    switch(nivelActual) {
        case 1:
            velocidadAsteroideBase = 7.0f;
            frecuenciaSpawn = 1.8f;
            std::cout << "   Dificultad: FACIL (Cuidado en el espacio)" << std::endl;
            break;
        case 2:
            velocidadAsteroideBase = 10.0f;
            frecuenciaSpawn = 1.4f;
            std::cout << "   Dificultad: NORMAL (Aumenta la velocidad)" << std::endl;
            break;
        case 3:
            velocidadAsteroideBase = 13.5f;
            frecuenciaSpawn = 1.0f;
            std::cout << "   Dificultad: DIFICIL (Rafagas constantes)" << std::endl;
            break;
        case 4:
            velocidadAsteroideBase = 17.0f;
            frecuenciaSpawn = 0.7f;
            std::cout << "   Dificultad: MUY DIFICIL (Pocos segundos para reaccionar)" << std::endl;
            break;
        case 5:
            velocidadAsteroideBase = 22.0f;
            frecuenciaSpawn = 0.4f;
            std::cout << "   Dificultad: ¡MODO IMPOSIBLE! (Lluvia masiva)" << std::endl;
            break;
    }
    std::cout << "========================================\n" << std::endl;
}

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
void processAsteroids(float deltaTime, unsigned int shaderProgram, GLint normalMatrixLoc);
void drawBullets(unsigned int shaderProgram, GLint normalMatrixLoc);

// =========================================================================
// SHADERS CON ILUMINACION (Phase 1: Ambient + Directional, Phase 2: Point)
// =========================================================================

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"      // NEW: normal attribute
    "layout (location = 2) in vec3 aColor;\n"        // shifted from location 1
    "layout (location = 3) in vec2 aTexCoord;\n"     // shifted from location 2

    "out vec3 FragPos;\n"       // world-space position for lighting
    "out vec3 Normal;\n"        // world-space normal
    "out vec3 ourColor;\n"
    "out vec2 TexCoord;\n"

    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform mat3 normalMatrix;\n"  // inverse-transpose of model rotation

    "void main()\n"
    "{\n"
    "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
    "   Normal = normalMatrix * aNormal;\n"
    "   ourColor = aColor;\n"
    "   TexCoord = aTexCoord;\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "}\0";

// Fragment shader with ambient + directional + point lights (Phase 1 + Phase 2)
const char *fragmentShaderTexSource = "#version 330 core\n"
    "out vec4 FragColor;\n"

    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "in vec3 ourColor;\n"
    "in vec2 TexCoord;\n"

    "uniform sampler2D ourTexture;\n"
    "uniform vec3 viewPos;\n"          // camera position for specular

    // Directional light (the sun)
    "uniform vec3 lightDir;\n"
    "uniform vec3 lightColor;\n"
    "uniform float lightIntensity;\n"

    // Ambient
    "uniform float ambientStrength;\n"
    "uniform vec3 ambientColor;\n"

    // Point lights (Phase 2)
    "#define MAX_POINT_LIGHTS 4\n"
    "struct PointLight {\n"
    "    vec3 position;\n"
    "    vec3 color;\n"
    "    float intensity;\n"
    "    float constant;\n"
    "    float linear;\n"
    "    float quadratic;\n"
    "};\n"
    "uniform PointLight pointLights[MAX_POINT_LIGHTS];\n"
    "uniform int numPointLights;\n"

    // Emissive flag for beams (Phase 3 hook)
    "uniform bool isEmissive;\n"
    "uniform vec3 emissiveColor;\n"

    "vec3 calcDirLight(vec3 normal, vec3 baseColor)\n"
    "{\n"
    "    float diff = max(dot(normal, -lightDir), 0.0);\n"
    "    return diff * lightColor * lightIntensity * baseColor;\n"
    "}\n"

    "vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 baseColor)\n"
    "{\n"
    "    vec3 lightDir = normalize(light.position - fragPos);\n"
    "    float diff = max(dot(normal, lightDir), 0.0);\n"
    "    float dist = length(light.position - fragPos);\n"
    "    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);\n"
    "    return diff * light.color * light.intensity * attenuation * baseColor;\n"
    "}\n"

    "void main()\n"
    "{\n"
    "   // Sample texture and blend with vertex color\n"
    "   vec4 texColor = texture(ourTexture, TexCoord);\n"
    "   vec3 baseColor;\n"
    "   if(texColor.a < 0.1) {\n"
    "       baseColor = ourColor;\n"
    "   } else {\n"
    "       baseColor = mix(ourColor, texColor.rgb, 0.7);\n"
    "   }\n"

    "   // Emissive path (for beams/glowing objects)\n"
    "   if(isEmissive) {\n"
    "       FragColor = vec4(emissiveColor * baseColor, 1.0);\n"
    "       return;\n"
    "   }\n"

    "   vec3 norm = normalize(Normal);\n"

    "   // Ambient term\n"
    "   vec3 result = ambientStrength * ambientColor * baseColor;\n"

    "   // Directional light (the sun)\n"
    "   result += calcDirLight(norm, baseColor);\n"

    "   // Point lights (black hole + optional others)\n"
    "   for(int i = 0; i < numPointLights; i++) {\n"
    "       result += calcPointLight(pointLights[i], norm, FragPos, baseColor);\n"
    "   }\n"

    "   FragColor = vec4(result, 1.0);\n"
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
    //BlackHole blackhole;
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

    // ---- PHASE 1: Setup directional light (the sun) ----
    glUniform3f(glGetUniformLocation(shaderProgram, "lightDir"),
        0.0f, 0.707f, 0.707f);   // 45 degrees from above and front
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"),
        1.0f, 0.95f, 0.8f);       // warm sunlight
    glUniform1f(glGetUniformLocation(shaderProgram, "lightIntensity"),
        1.0f);

    // Ambient light
    glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"),
        0.15f);                      // not too dark in shadows
    glUniform3f(glGetUniformLocation(shaderProgram, "ambientColor"),
        0.1f, 0.1f, 0.15f);         // slightly cool ambient for space

    // Emissive defaults (off by default)
    glUniform1i(glGetUniformLocation(shaderProgram, "isEmissive"), GL_FALSE);
    glUniform3f(glGetUniformLocation(shaderProgram, "emissiveColor"), 0.0f, 0.0f, 0.0f);

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
        GLint normalMatrixLoc = glGetUniformLocation(shaderProgram, "normalMatrix");
        GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
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

        // 2. ACTIVAR SHADER PRINCIPAL CON TEXTURAS E ILUMINACION PARA LOS OBJETOS 3D
        glUseProgram(shaderProgram);

        // Enviar matrices de cámara actualizadas al shader program
        glUniformMatrix4fv(viewLoc, 1, GL_TRUE, viewMatrix.mat.data());
        glUniformMatrix4fv(projLoc, 1, GL_TRUE, projMatrix.mat.data());

        // ---- PHASE 2: Upload black hole as point light + optional distant star ----
        vec3 bhPos(blackhole.position[0], blackhole.position[1], blackhole.position[2]);
        vec3 shipPos = spaceship.getPosition();

        // Black hole as point light 0 (warm orange glow)
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[0].position"),
            bhPos.x, bhPos.y, bhPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[0].color"),
            1.0f, 0.4f, 0.05f);        // warm orange (matches disk colors)

        // Dynamic intensity based on distance to black hole
        float distToBH = helper::length(shipPos - bhPos);
        float proximityBoost = 1.0f + 5.0f * (1.0f / (1.0f + distToBH * 0.1f));
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[0].intensity"),
            2.0f * proximityBoost);

        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[0].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[0].linear"), 0.09f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[0].quadratic"), 0.032f);

        // Point light 1: distant blue star (fills shadows from opposite side)
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[1].position"),
            -50.0f, 30.0f, -20.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[1].color"),
            0.3f, 0.5f, 0.8f);          // cool blue
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[1].intensity"),
            0.5f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[1].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[1].linear"), 0.045f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[1].quadratic"), 0.0075f);

        glUniform1i(glGetUniformLocation(shaderProgram, "numPointLights"), 2);

        // Upload camera position for lighting calculations
        vec3 camPos;
        if (camera.isFollowMode()) {
            // Approximate camera position from follow mode
            camPos = shipPos;
        } else {
            float pitchRad = helper::toRadians(camera.getPitch());
            float yawRad = helper::toRadians(camera.getYaw());
            float dist = camera.getDistance();
            camPos = vec3(
                shipPos.x + dist * std::cos(pitchRad) * std::cos(yawRad),
                shipPos.y + dist * std::sin(pitchRad),
                shipPos.z + dist * std::cos(pitchRad) * std::sin(yawRad)
            );
        }
        glUniform3f(viewPosLoc, camPos.x, camPos.y, camPos.z);

        // ---- Draw Rubik's Cube ----
        matriz4x4 modelMatrixCube; // identity matrix
        std::array<float, 9> normalMatrixCube = helper::extract_normal_matrix(modelMatrixCube);
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, modelMatrixCube.mat.data());
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, normalMatrixCube.data());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ourTextureID);
        cuboRubik->draw(shaderProgram);

        // ---- Update and draw game objects ----
        updateBullets(deltaTime);
        spawnAsteroids(currentFrame);
        processAsteroids(deltaTime, shaderProgram, normalMatrixLoc);
		processParticles(deltaTime, shaderProgram, normalMatrixLoc); //particulas de la explosion
        drawBullets(shaderProgram, normalMatrixLoc);

        // DETECCIÓN DE VICTORIA / AVANCE DE NIVEL
        if (!gameOver) {
            // Comparamos la distancia actual contra el radio fisico de la esfera
            if (distToBH <= blackhole.bhRadius) {
                if (nivelActual < 5) {
                    nivelActual++;
                    std::cout << "\n==================================================" << std::endl;
                    std::cout << " ¡NIVEL COMPLETADO! Has cruzado el Agujero Negro" << std::endl;
                    std::cout << "==================================================" << std::endl;
                    resetPosicionNave();
                    actualizarDificultadNivel(); 
                } else {
                    std::cout << "\n==================================================" << std::endl;
                    std::cout << "  ¡¡FELICIDADES!! HAS COMPLETADO EL JUEGO (NIVEL 5) " << std::endl;
                    std::cout << "  Ganaste el juego " << std::endl;
                    std::cout << "==================================================" << std::endl;
                    gameOver = true; 
                }
            }
        }
		
        // ---- Draw Spaceship ----
        matriz4x4 modelMatrixShip = spaceship.getModelMatrix();
        std::array<float, 9> normalMatrixShip = helper::extract_normal_matrix(modelMatrixShip);
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, modelMatrixShip.mat.data());
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, normalMatrixShip.data());
        glUniform1i(ourTextureLoc, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, spaceshipTexID);
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
			nivelActual = 1; // para el juego con niveles
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
    // 1. CONDICIONAL DE TIEMPO ESTRICTO: Controla que solo pase cada 1.5 segundos
    if (currentFrame - tiempoUltimoAsteroide > frecuenciaSpawn) {
            
        // Origen exacto: El centro de la esfera del Agujero Negro [0]=X, [1]=Y, [2]=Z
        float originX = blackhole.position[0];
        float originY = blackhole.position[1];
        float originZ = blackhole.position[2];
        vec3 puntoOrigen(originX, originY, originZ);

        // Objetivo base: La ubicación de la nave del jugador
        vec3 navePos = spaceship.getPosition();

        // Variación aleatoria (Dispersión) para que salgan como ráfagas en cono
        float dispersionX = ((float)(rand() % 100) / 100.0f * 6.0f) - 3.0f; // [-3.0f, 3.0f]
        float dispersionY = ((float)(rand() % 100) / 100.0f * 6.0f) - 3.0f; // [-3.0f, 3.0f]
        vec3 puntoDestino(navePos.x + dispersionX, navePos.y + dispersionY, navePos.z);

        // 2. SISTEMA DE PROBABILIDAD DE TAMAÑO (De tus compañeros)
        // 30% pequeños (0.5), 50% medianos (1.0), 20% grandes (1.5)
        float sizeRoll = (float)(rand() % 100) / 100.0f;
        float tamanoAleatorio;
        if (sizeRoll < 0.3f)       tamanoAleatorio = 0.5f;
        else if (sizeRoll < 0.8f)  tamanoAleatorio = 1.0f;
        else                       tamanoAleatorio = 1.5f;

        // Instanciar el asteroide con la trayectoria Agujero Negro -> Nave
        Asteroid nuevoAsteroide(puntoOrigen, puntoDestino, tamanoAleatorio);
            
        // Velocidad de eyección (puedes ajustarla si van muy rápido o lento)
        //nuevoAsteroide.speed = 8.5f;
        nuevoAsteroide.speed = velocidadAsteroideBase; //velocidad según el nivel actual
        
        // Agregar a la lista e igualar el reloj
        listaAsteroides.push_back(nuevoAsteroide);
        tiempoUltimoAsteroide = currentFrame; 
    }
}

// Actualizar, dibujar y gestionar colisiones de todos los asteroides
void processAsteroids(float deltaTime, unsigned int shaderProgram, GLint normalMatrixLoc) {
	GLint ourTextureLoc =
    glGetUniformLocation(shaderProgram, "ourTexture");
    glUniform1i(ourTextureLoc, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, asteroideTexID);

    for (size_t i = 0; i < listaAsteroides.size(); ) {
        listaAsteroides[i].update(deltaTime);

        // Upload model and normal matrix for lighting
        matriz4x4 asteroidModel = listaAsteroides[i].getModelMatrix();
        std::array<float, 9> asteroidNormalMat = helper::extract_normal_matrix(asteroidModel);
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, asteroidModel.mat.data());
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, asteroidNormalMat.data());

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
				//
				if (listaAsteroides[i].isDestroyed()) {
					asteroidesDestruidos++;
					emitirExplosion(listaAsteroides[i].position, listaAsteroides[i].scale);

					std::cout << "[DISPARO] Asteroide destruido!..." << std::endl;
					listaAsteroides.erase(listaAsteroides.begin() + i);
					removed = true;
				}
                //listaAsteroides.erase(listaAsteroides.begin() + i);
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
            if (listaAsteroides[i].position.z > 5.0f) {
                asteroidesDestruidos++;
                listaAsteroides.erase(listaAsteroides.begin() + i);
            } else {
                i++;
            }
        }
    }
}

// Dibujar todas las balas activas con textura transparente (vertex color)
void drawBullets(unsigned int shaderProgram, GLint normalMatrixLoc) {
	GLint ourTextureLoc =
    glGetUniformLocation(shaderProgram, "ourTexture");
    glUniform1i(ourTextureLoc, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bulletTexID);
    for (auto& b : listaBalas) {
        matriz4x4 bulletModel = b.getModelMatrix();
        std::array<float, 9> bulletNormalMat = helper::extract_normal_matrix(bulletModel);
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, bulletModel.mat.data());
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, bulletNormalMat.data());
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