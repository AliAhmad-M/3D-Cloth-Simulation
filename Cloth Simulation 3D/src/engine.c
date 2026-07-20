#include "engine.h"
#include <stdio.h>

bool engine_init(engine_t* engine, int width, int height, const char* title) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    engine->window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!engine->window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(engine->window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    renderer_init(&engine->renderer);

    vector3_t eye = { 1.5f, 1.2f, 2.0f };
    vector3_t target = { 0.0f, -0.3f, 0.0f };
    vector3_t up = { 0.0f, 1.0f, 0.0f };
    camera_init(&engine->camera, eye, target, up, (float)width / (float)height);

    engine->is_running = true;
    
    float spacing = 0.08f;
    float start_x = -(GRID_WIDTH * spacing) * 0.5f;
    float start_z = -(GRID_HEIGHT * spacing) * 0.5f;

    // Initialize particles
    for (int z = 0; z < GRID_HEIGHT; ++z) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            int idx = z * GRID_WIDTH + x;
            particle_create(&engine->particles[idx]);

            engine->particles[idx].curr_position = (vector3_t){
                start_x + x * spacing, 0.0f, start_z + z * spacing
            };
            engine->particles[idx].prev_position = engine->particles[idx].curr_position;
            engine->particles[idx].radius = 0.012f;

            bool is_corner = (x == 0 && z == 0) ||
                (x == GRID_WIDTH - 1 && z == 0) ||
                (x == 0 && z == GRID_HEIGHT - 1) ||
                (x == GRID_WIDTH - 1 && z == GRID_HEIGHT - 1);

            if (is_corner) {
                engine->particles[idx].is_fixed = true;
            }
        }
    }

    // Initialize constraints
    int constraint_count = 0;
    for (int z = 0; z < GRID_HEIGHT; ++z) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            int idx = z * GRID_WIDTH + x;

            if (x < GRID_WIDTH - 1) {
                engine->constraints[constraint_count++] = cloth_constraint_add(
                    &engine->particles[idx], &engine->particles[idx + 1]
                );
            }
            if (z < GRID_HEIGHT - 1) {
                engine->constraints[constraint_count++] = cloth_constraint_add(
                    &engine->particles[idx], &engine->particles[idx + GRID_WIDTH]
                );
            }
        }
    }

    return true;
}

void engine_run(engine_t* engine) {
    const float dt = 1.0f / 60.0f;
    const int constraint_iterations = 8;
    double previous_time = glfwGetTime();
    double accumulator = 0.0;

    while (!glfwWindowShouldClose(engine->window) && engine->is_running) {
        double current_time = glfwGetTime();
        double frame_time = current_time - previous_time;
        previous_time = current_time;

        if (frame_time > 0.25) frame_time = 0.25;
        accumulator += frame_time;

        // Physics update
        while (accumulator >= dt) {
            for (int i = 0; i < NUM_PARTICLES; ++i) {
                particle_update(&engine->particles[i], dt);
            }
            for (int iter = 0; iter < constraint_iterations; ++iter) {
                for (int i = 0; i < NUM_CONSTRAINTS; ++i) {
                    cloth_constraint_resolve(&engine->constraints[i]);
                }
            }
            accumulator -= dt;
        }

        // Render
        camera_update_matrix(&engine->camera);

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (int i = 0; i < NUM_CONSTRAINTS; ++i) {
            if (engine->constraints[i].is_active) {
                renderer_draw_line(
                    &engine->renderer,
                    engine->constraints[i].a->curr_position,
                    engine->constraints[i].b->curr_position,
                    engine->camera.view_proj_matrix
                );
            }
        }

        for (int i = 0; i < NUM_PARTICLES; ++i) {
            renderer_draw_sphere(
                &engine->renderer,
                engine->particles[i].curr_position,
                engine->particles[i].radius,
                engine->camera.view_proj_matrix
            );
        }

        glfwSwapBuffers(engine->window);
        glfwPollEvents();
    }
}

void engine_cleanup(engine_t* engine) {
    renderer_cleanup(&engine->renderer);
    if (engine->window) {
        glfwDestroyWindow(engine->window);
    }
    glfwTerminate();
}