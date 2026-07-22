#pragma once
#include <glad/glad.h>
#include "microui.h"

typedef struct {
    GLuint shader_program;
    GLint  u_proj_loc;
    GLint  u_tex_loc;

    GLuint vao, vbo, ebo;
    GLuint atlas_texture;

    // Fixed-size scratch buffers
    float         verts[16384 * 4];
    unsigned char colors[16384 * 4];
    unsigned int  indices[16384 * 6 / 4];
    int vert_count;
    int index_count;

    int screen_width, screen_height;
} ui_renderer_t;

void ui_renderer_init(ui_renderer_t* r, int screen_width, int screen_height);
void ui_renderer_cleanup(ui_renderer_t* r);
void ui_renderer_resize(ui_renderer_t* r, int screen_width, int screen_height);
void ui_renderer_draw(ui_renderer_t* r, mu_Context* ctx);