# Proyecto Final: Simulador de Nave Espacial + Cubo Rubik

**UCSP — Computación Gráfica 2026-I**  
**Equipo:** Alexander Baylon, Cristian Mellado, José Vilca, Walter Valdivia

> Documentación detallada: https://docs.google.com/document/d/1u35rTn6cfw_e-J-3zW3vBR1g2N1zxUwVSK9F6LBvxbA/edit?usp=drivesdk

## Descripción

Aplicación OpenGL 3.3 Core que integra un simulador de Cubo Rubik con animaciones de rotación por capas y un juego de escape espacial. El jugador pilotea una nave a través de un corredor de asteroides para alcanzar un agujero negro, con 5 niveles de dificultad creciente.

## Controles rápidos

| Tecla | Acción |
|---|---|
| ENTER | Iniciar misión (desde menú) |
| ↑↓←→ | Mover / rotar nave |
| Mouse | Apuntar (mover) + disparar (click izq.) |
| WASD / QE | Órbita de cámara / zoom |
| T/R/F/G/Y/H | Rotar caras del cubo (U/L/F/R/B/D) |
| J / M | Resolver / desordenar cubo |
| K / R | Reiniciar cubo / reiniciar juego |
| 1–5 | Velocidad de animación (x1–x16) |
| F | Alternar cámara follow / orbital |
| ESC | Salir |

## Características

- Cubo Rubik con 26 cubies, animación de rotación por capas y slices, solver Kociemba (algoritmo de dos fases con IDA*)
- Dos modos: menú cinemático (auto-órbita) y juego (supervivencia)
- Iluminación: ambiental + direccional + hasta 8 luces puntuales (agujero negro, faro de nave, balas de energía)
- Cámara orbital + follow (tercera persona con suavizado exponencial)
- Agujero negro con disco de acreción, jets polares, partículas de gas y skybox propio
- Sistema de partículas para propulsor y explosiones
- 5 cubos Rubik decorativos con ciclos autónomos scramble→solve fuera del corredor
- Asteroides decorativos emitidos desde el agujero negro en direcciones aleatorias
- 5 niveles de dificultad, detección de colisiones, sistema de vidas

## Archivos clave

| Archivo | Contenido |
|---|---|
| `Rubik_Project/main.cpp` | Entry point, shaders, loop principal, sistema de juego |
| `Rubik_Project/rubik.h` | Clase CuboRubik: animación, solver, secuencias |
| `Rubik_Project/figura.h` | Clases Figura + Cubo: VAO/VBO, 36 vértices por cubie |
| `Rubik_Project/camera.h` | Cámara orbital + follow |
| `Rubik_Project/spaceship.h` | Nave: carga 3DS, renderizado, movimiento direccional |
| `Rubik_Project/asteroid.h` | Asteroide: malla compartida, colisión, HP |
| `Rubik_Project/blackhole.h` | Agujero negro: disco, jets, partículas, skybox |
| `Rubik_Project/solver/` | Solver Kociemba dos fases |

## Referencias

- [OpenGL 3.3 Core Specification](https://www.khronos.org/opengl/)
- [GLFW 3.3](https://www.glfw.org/)
- [Kociemba Two-Phase Algorithm](http://kociemba.org/cube.htm)
