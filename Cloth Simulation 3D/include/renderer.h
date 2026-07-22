#pragma once
#include <glad/glad.h>
#include <stdint.h>
#include <vector3.h>

typedef struct {
    // Sphere (lit) shader
    GLuint sphere_shader_program;
    GLint  u_view_proj_loc;
    GLint  u_offset_loc;
    GLint  u_scale_loc;
    GLint  u_color_loc;
    GLint  u_light_dir_loc;

    GLuint sphere_vao, sphere_vbo, sphere_ebo;
    int    sphere_index_count;

    // Line (unlit) shader
    GLuint line_shader_program;
    GLint  u_line_view_proj_loc;
    GLint  u_line_color_loc;
    GLuint line_vao, line_vbo;

    // Quad (lit) shader
    GLuint quad_shader_program;
    GLint  u_quad_view_proj_loc;
    GLint  u_quad_color_loc;
    GLint  u_quad_light_dir_loc;
    GLuint quad_vao, quad_vbo;
} renderer_t;

void renderer_init(renderer_t* r);
void renderer_cleanup(renderer_t* r);

void renderer_draw_sphere(
    renderer_t* r, vector3_t position, float radius,
    uint8_t color_r, uint8_t color_g, uint8_t color_b,
    const float* view_proj
);

void renderer_draw_line(
    renderer_t* r, vector3_t start, vector3_t end,
    const float* view_proj
);

void renderer_draw_quad(
    renderer_t* r,
    vector3_t p0, vector3_t p1, vector3_t p2, vector3_t p3,
    vector3_t n0, vector3_t n1, vector3_t n2, vector3_t n3,
    uint8_t color_r, uint8_t color_g, uint8_t color_b,
    const float* view_proj
);