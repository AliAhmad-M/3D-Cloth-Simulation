#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "engine.h"

static void spawn_objects(engine_t* engine) {
    float spacing = 0.12f;
    float start_x = -(GRID_WIDTH * spacing) * 0.5f;
    float start_z = -(GRID_HEIGHT * spacing) * 0.5f;
    float start_y = 0.55f;

    // Initialize cloth particles
    for (int z = 0; z < GRID_HEIGHT; ++z) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            int idx = z * GRID_WIDTH + x;
            particle_create(&engine->particles[idx]);

            engine->particles[idx].curr_position = (vector3_t){
                start_x + x * spacing, start_y, start_z + z * spacing
            };
            engine->particles[idx].prev_position = engine->particles[idx].curr_position;
            engine->particles[idx].radius = 0.006f;
            engine->particles[idx].is_fixed = false;
        }
    }

    // Add a fixed sphere
    int sphere_idx = GRID_WIDTH * GRID_HEIGHT;
    particle_create(&engine->particles[sphere_idx]);
    engine->particles[sphere_idx].curr_position = (vector3_t){ 0.0f, -0.3f, 0.0f };
    engine->particles[sphere_idx].prev_position = engine->particles[sphere_idx].curr_position;
    engine->particles[sphere_idx].radius = 0.25f;
    engine->particles[sphere_idx].is_fixed = true;
    engine->particles[sphere_idx].color_r = 0;
    engine->particles[sphere_idx].color_g = 189;
    engine->particles[sphere_idx].color_b = 255;

    // Initialize cloth constraints
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
}

bool engine_init(engine_t* engine, int width, int height, const char* title) {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    engine->window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!engine->window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(engine->window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return false;
    }

    // Initialize renderer
    glEnable(GL_DEPTH_TEST);
    renderer_init(&engine->renderer);

    vector3_t eye = { 1.5f, 1.2f, 2.0f };
    vector3_t target = { 0.0f, -0.3f, 0.0f };
    vector3_t up = { 0.0f, 1.0f, 0.0f };
    camera_init(&engine->camera, eye, target, up, (float)width / (float)height);

    engine->is_running = true;

    // Allocate memory for particle and constraints
    engine->particles   = (particle_t*)malloc(sizeof(particle_t) * NUM_PARTICLES);
    engine->constraints = (constraint_t*)malloc(sizeof(constraint_t) * NUM_CONSTRAINTS);

    spawn_objects(engine);

    return true;
}

void engine_run(engine_t* engine) {
    const float dt = 1.0f / 60.0f;
    const int substeps = 8;
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
            // Apply gravity to all dynamic particles
            for (int i = 0; i < NUM_PARTICLES; ++i) {
                if (!engine->particles[i].is_fixed) {
                    particle_update(&engine->particles[i], dt);
                }
            }

            // Interleave constraints and sphere collisions
            particle_t* sphere = &engine->particles[GRID_WIDTH * GRID_HEIGHT];

            for (int iter = 0; iter < substeps; ++iter) {
                // Resolve cloth structural constraints
                for (int i = 0; i < NUM_CONSTRAINTS; ++i) {
                    cloth_constraint_resolve(&engine->constraints[i]);
                }

                // Collide cloth particles against the static sphere
                for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; ++i) {
                    particle_t* p = &engine->particles[i];

                    vector3_t delta = vector3_sub(p->curr_position, sphere->curr_position);
                    float dist_sq = vector3_dot(delta, delta);
                    float min_dist = p->radius + sphere->radius;

                    if (dist_sq < min_dist * min_dist && dist_sq > 0.000001f) {
                        float dist = sqrtf(dist_sq);
                        float overlap = min_dist - dist;
                        vector3_t normal = vector3_mul(delta, 1.0f / dist);

                        // Push particles
                        p->curr_position = vector3_add(p->curr_position, vector3_mul(normal, overlap));

                        // Add a friction
                        vector3_t vel = vector3_sub(p->curr_position, p->prev_position);
                        p->prev_position = vector3_add(p->prev_position, vector3_mul(vel, 0.1f));
                    }
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
                engine->particles[i].color_r,
                engine->particles[i].color_g,
                engine->particles[i].color_b,
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