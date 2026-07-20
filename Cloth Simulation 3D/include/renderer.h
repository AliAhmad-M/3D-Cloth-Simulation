#pragma once
#include <glad/glad.h>
#include <vector3.h>

typedef struct {
    GLuint shader_program;
    GLuint u_view_proj_loc;
    GLuint u_offset_loc;
    GLuint u_scale_loc;

    // Mesh buffers
    GLuint sphere_vao, sphere_vbo, sphere_ebo;
    int sphere_index_count;

    GLuint line_vao, line_vbo;
} renderer_t;

void renderer_init(renderer_t* r);
void renderer_cleanup(renderer_t* r);

void renderer_draw_sphere(renderer_t* r, vector3_t position, float radius, const float* view_proj);
void renderer_draw_line(renderer_t* r, vector3_t start, vector3_t end, const float* view_proj);

