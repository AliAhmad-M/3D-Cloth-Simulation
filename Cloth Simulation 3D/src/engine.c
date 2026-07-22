#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "atlas.inl"

static int mu_text_width(mu_Font font, const char* text, int len) {
    int res = 0;
    for (const char* p = text; *p && len--; p++) {
        res += atlas[ATLAS_FONT + (unsigned char)*p].w;
    }
    return res;
}

static int mu_text_height(mu_Font font) { return 18; }

static int mu_button_map(int glfw_button) {
    switch (glfw_button) {
    case GLFW_MOUSE_BUTTON_LEFT:   return MU_MOUSE_LEFT;
    case GLFW_MOUSE_BUTTON_RIGHT:  return MU_MOUSE_RIGHT;
    case GLFW_MOUSE_BUTTON_MIDDLE: return MU_MOUSE_MIDDLE;
    default: return 0;
    }
}

static void glfw_cursor_pos_callback(GLFWwindow* window, double x, double y) {
    engine_t* engine = (engine_t*)glfwGetWindowUserPointer(window);
    mu_input_mousemove(engine->mu_ctx, (int)x, (int)y);
}

static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    engine_t* engine = (engine_t*)glfwGetWindowUserPointer(window);
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    int mu_button = mu_button_map(button);
    if (!mu_button) return;
    if (action == GLFW_PRESS)   mu_input_mousedown(engine->mu_ctx, (int)x, (int)y, mu_button);
    if (action == GLFW_RELEASE) mu_input_mouseup(engine->mu_ctx, (int)x, (int)y, mu_button);
}

static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    engine_t* engine = (engine_t*)glfwGetWindowUserPointer(window);
    mu_input_scroll(engine->mu_ctx, 0, (int)(-yoffset * 30));
}


static void spawn_objects(engine_t* engine) {
    float spacing = 0.07f;
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
            engine->particles[idx].radius = 0.012f;
            engine->particles[idx].is_fixed = false;
        }
    }

    // Add a fixed sphere
    int sphere_idx = GRID_WIDTH * GRID_HEIGHT;
    particle_create(&engine->particles[sphere_idx]);
    engine->particles[sphere_idx].curr_position = (vector3_t){ 0.0f, -0.3f, 0.0f };
    engine->particles[sphere_idx].prev_position = engine->particles[sphere_idx].curr_position;
    engine->particles[sphere_idx].radius = 0.22f;
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

            // Shear constraints (diagonals)
            if (x < GRID_WIDTH - 1 && z < GRID_HEIGHT - 1) {
                engine->constraints[constraint_count++] = cloth_constraint_add(
                    &engine->particles[idx], &engine->particles[idx + GRID_WIDTH + 1]
                );
                engine->constraints[constraint_count++] = cloth_constraint_add(
                    &engine->particles[idx + 1], &engine->particles[idx + GRID_WIDTH]
                );
            }
        }
    }
}

static vector3_t compute_particle_normal(engine_t* engine, int x, int z) {
    int xm = x > 0 ? x - 1 : x;
    int xp = x < GRID_WIDTH - 1 ? x + 1 : x;
    int zm = z > 0 ? z - 1 : z;
    int zp = z < GRID_HEIGHT - 1 ? z + 1 : z;

    vector3_t left = engine->particles[z * GRID_WIDTH + xm].curr_position;
    vector3_t right = engine->particles[z * GRID_WIDTH + xp].curr_position;
    vector3_t up = engine->particles[zm * GRID_WIDTH + x].curr_position;
    vector3_t down = engine->particles[zp * GRID_WIDTH + x].curr_position;

    vector3_t dx = vector3_sub(right, left);
    vector3_t dz = vector3_sub(down, up);
    vector3_t n = vector3_cross(dz, dx);

    float len_sq = vector3_dot(n, n);
    if (len_sq > 0.00001f) return vector3_mul(n, 1.0f / sqrtf(len_sq));
    return (vector3_t) { 0.0f, 1.0f, 0.0f };
}

static void reset_cloth_position(engine_t* engine) {
    float spacing = 0.07f;
    float start_x = -(GRID_WIDTH * spacing) * 0.5f;
    float start_z = -(GRID_HEIGHT * spacing) * 0.5f;
    float start_y = 0.55f;

    for (int z = 0; z < GRID_HEIGHT; ++z) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            int idx = z * GRID_WIDTH + x;
            engine->particles[idx].curr_position = (vector3_t){
                start_x + x * spacing, start_y, start_z + z * spacing
            };
            engine->particles[idx].prev_position = engine->particles[idx].curr_position;
        }
    }
}

static void build_debug_ui(engine_t* engine) {
    mu_Context* ctx = engine->mu_ctx;

    if (mu_begin_window_ex(ctx, "Simulation Controls", mu_rect(20, 20, 220, 340), MU_OPT_NOCLOSE)) {

        // Performance panel
        if (mu_header_ex(ctx, "Performance", MU_OPT_EXPANDED)) {
            static double last_time = 0.0;
            static double time_accumulator = 0.0;
            static int frame_count = 0;
            static char stats_buf[64] = "FPS: --";

            double now = glfwGetTime();
            if (last_time == 0.0) last_time = now;

            double delta = now - last_time;
            last_time = now;

            time_accumulator += delta;
            frame_count++;

            // Update FPS text
            if (time_accumulator >= 1.0) {
                float fps = (float)frame_count / (float)time_accumulator;
                snprintf(stats_buf, sizeof(stats_buf), "FPS: %.1f", fps);

                time_accumulator = 0.0;
                frame_count = 0;
            }

            mu_layout_row(ctx, 1, (int[]) { -1 }, 0);
            mu_label(ctx, stats_buf);
        }

        // Display mode panel
        if (mu_header_ex(ctx, "Display Mode", MU_OPT_EXPANDED)) {
            mu_layout_row(ctx, 2, (int[]) { -110, -1 }, 24);

            if (mu_button_ex(ctx, "Cloth", 0, MU_OPT_ALIGNCENTER)) engine->render_mode = RENDER_MODE_CLOTH;
            if (mu_button_ex(ctx, "Grid", 0, MU_OPT_ALIGNCENTER)) engine->render_mode = RENDER_MODE_GRID;

            mu_layout_row(ctx, 1, (int[]) { -1 }, 0);
            if (engine->render_mode == RENDER_MODE_CLOTH) {
                mu_label(ctx, "Active: Quad Mesh");
            }
            else {
                mu_label(ctx, "Active: Particles & Constraints");
            }
        }

        // Obstacle panel
        if (mu_header_ex(ctx, "Sphere Properties", MU_OPT_EXPANDED)) {
            particle_t* sphere = &engine->particles[GRID_WIDTH * GRID_HEIGHT];

            mu_layout_row(ctx, 1, (int[]) { -1 }, 0);
            mu_label(ctx, "Sphere Radius:");
            mu_slider(ctx, &sphere->radius, 0.05f, 0.50f);

            mu_label(ctx, "Sphere Y Height:");
            mu_slider(ctx, &sphere->curr_position.y, -1.0f, 0.5f);
            sphere->prev_position.y = sphere->curr_position.y;
        }

        // Actions panel
        mu_layout_row(ctx, 1, (int[]) { -1 }, 28);
        if (mu_button_ex(ctx, "Reset Cloth", 0, MU_OPT_ALIGNCENTER)) {
            reset_cloth_position(engine);
        }

        mu_end_window(ctx);
    }
}

static int text_width_cb(mu_Font font, const char* str, int len) {
    if (len < 0) len = (int)strlen(str);
    return len * 6;
}

static int text_height_cb(mu_Font font) {
    return 16;
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

    // Initialize callbacks
    glfwSetWindowUserPointer(engine->window, engine);
    glfwSetCursorPosCallback(engine->window, glfw_cursor_pos_callback);
    glfwSetMouseButtonCallback(engine->window, glfw_mouse_button_callback);
    glfwSetScrollCallback(engine->window, glfw_scroll_callback);

    // Allocate internal modules
    engine->camera = malloc(sizeof(camera_t));
    engine->renderer = malloc(sizeof(renderer_t));
    engine->mu_ctx = malloc(sizeof(mu_Context));
    engine->ui_renderer = malloc(sizeof(ui_renderer_t));

    if (!engine->camera
        || !engine->renderer
        || !engine->mu_ctx
        || !engine->ui_renderer) {
        engine_cleanup(engine);
        return false;
    }

    // Initialize renderer
    glEnable(GL_DEPTH_TEST);
    renderer_init(engine->renderer);

    // Initialize UI library
    mu_init(engine->mu_ctx);

    // Initialize UI renderer
    ui_renderer_init(engine->ui_renderer, width, height);
    engine->mu_ctx->text_width  = text_width_cb;
    engine->mu_ctx->text_height = text_height_cb;
    engine->render_mode = RENDER_MODE_CLOTH;

    // Initialize camera
    camera_init(engine->camera, 
        (vector3_t){ 1.5f,  1.2f, 2.0f }, 
        (vector3_t){ 0.0f, -0.3f, 0.0f }, 
        (vector3_t){ 0.0f,  1.0f, 0.0f }, 
        (float)width / (float)height
    );

    // Allocate memory for particle and constraints
    engine->particles   = (particle_t*)malloc(sizeof(particle_t) * NUM_PARTICLES);
    engine->constraints = (constraint_t*)malloc(sizeof(constraint_t) * NUM_CONSTRAINTS);

    // Create particles and constraints
    spawn_objects(engine);

    engine->is_running = true;
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

        mu_begin(engine->mu_ctx);
        build_debug_ui(engine);
        mu_end(engine->mu_ctx);

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
        camera_update_matrix(engine->camera);

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (engine->render_mode == RENDER_MODE_CLOTH) {
            // Draw cloth quads
            for (int z = 0; z < GRID_HEIGHT - 1; ++z) {
                for (int x = 0; x < GRID_WIDTH - 1; ++x) {
                    vector3_t p0 = engine->particles[z * GRID_WIDTH + x].curr_position;
                    vector3_t p1 = engine->particles[z * GRID_WIDTH + x + 1].curr_position;
                    vector3_t p2 = engine->particles[(z + 1) * GRID_WIDTH + x + 1].curr_position;
                    vector3_t p3 = engine->particles[(z + 1) * GRID_WIDTH + x].curr_position;

                    vector3_t n0 = compute_particle_normal(engine, x, z);
                    vector3_t n1 = compute_particle_normal(engine, x + 1, z);
                    vector3_t n2 = compute_particle_normal(engine, x + 1, z + 1);
                    vector3_t n3 = compute_particle_normal(engine, x, z + 1);

                    renderer_draw_quad(engine->renderer, p0, p1, p2, p3, n0, n1, n2, n3,
                        230, 230, 235, engine->camera->view_proj_matrix);
                }
            }
        }
        else {
            for (int i = 0; i < NUM_CONSTRAINTS; ++i) {
                if (engine->constraints[i].is_active) {
                    // Draw cloth constraints
                    renderer_draw_line(engine->renderer, engine->constraints[i].a->curr_position,
                        engine->constraints[i].b->curr_position, engine->camera->view_proj_matrix);
                }
            }
            for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; ++i) {
                // Draw cloth particles
                renderer_draw_sphere(engine->renderer, engine->particles[i].curr_position, engine->particles[i].radius,
                    engine->particles[i].color_r, engine->particles[i].color_g, engine->particles[i].color_b,
                    engine->camera->view_proj_matrix);
            }
        }

        particle_t* sphere = &engine->particles[GRID_WIDTH * GRID_HEIGHT];
        
        // Draw sphere obstacle
        renderer_draw_sphere(
            engine->renderer, sphere->curr_position, sphere->radius,
            sphere->color_r, sphere->color_g, sphere->color_b, 
            engine->camera->view_proj_matrix
        );

        // Handle UI rendering
        ui_renderer_draw(engine->ui_renderer, engine->mu_ctx);

        glfwSwapBuffers(engine->window);
        glfwPollEvents();
    }
}

void engine_cleanup(engine_t* engine) {
    renderer_cleanup(engine->renderer);
    ui_renderer_cleanup(engine->ui_renderer);
    if (engine->window) {
        glfwDestroyWindow(engine->window);
    }
    glfwTerminate();
}