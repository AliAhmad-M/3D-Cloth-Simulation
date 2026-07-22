#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

#include "particle.h"
#include "constraint.h"
#include "renderer.h"
#include "camera.h"

#define GRID_WIDTH  16
#define GRID_HEIGHT 16
#define NUM_PARTICLES ((GRID_WIDTH * GRID_HEIGHT) + 1)
#define NUM_CONSTRAINTS ((GRID_WIDTH - 1) * GRID_HEIGHT + GRID_WIDTH * (GRID_HEIGHT - 1))

typedef struct {
    GLFWwindow* window;
    renderer_t renderer;
    camera_t camera;

    particle_t* particles; 
    constraint_t* constraints;

    bool is_running;
} engine_t;

bool engine_init(engine_t* engine, int width, int height, const char* title);
void engine_run(engine_t* engine);
void engine_cleanup(engine_t* engine);

