#include <string.h>
#include <stdint.h>
#include "atlas.inl"
#include "ui.h"

static const char* UI_VS_SRC =
"#version 330 core\n"
"layout(location = 0) in vec2 aPos;\n"
"layout(location = 1) in vec2 aUV;\n"
"layout(location = 2) in vec4 aColor;\n"
"uniform mat4 uProj;\n"
"out vec2 vUV;\n"
"out vec4 vColor;\n"
"void main() {\n"
"    vUV = aUV;\n"
"    vColor = aColor;\n"
"    gl_Position = uProj * vec4(aPos, 0.0, 1.0);\n"
"}\n";

static const char* UI_FS_SRC =
"#version 330 core\n"
"in vec2 vUV;\n"
"in vec4 vColor;\n"
"out vec4 FragColor;\n"
"uniform sampler2D uTex;\n"
"void main() {\n"
"    float alpha = texture(uTex, vUV).r;\n" // atlas is single-channel; ATLAS_WHITE region is solid white -> alpha=1 for filled rects
"    FragColor = vec4(vColor.rgb, vColor.a * alpha);\n"
"}\n";

static GLuint ui_compile_program(const char* vs_src, const char* fs_src) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_src, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_src, NULL);
    glCompileShader(fs);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

void ui_renderer_init(ui_renderer_t* r, int screen_width, int screen_height) {
    memset(r, 0, sizeof(*r));
    r->screen_width = screen_width;
    r->screen_height = screen_height;

    r->shader_program = ui_compile_program(UI_VS_SRC, UI_FS_SRC);
    r->u_proj_loc = glGetUniformLocation(r->shader_program, "uProj");
    r->u_tex_loc = glGetUniformLocation(r->shader_program, "uTex");

    mu_init_atlas();

    // Atlas is single-channel (alpha-like); upload as GL_RED.
    glGenTextures(1, &r->atlas_texture);
    glBindTexture(GL_TEXTURE_2D, r->atlas_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ATLAS_WIDTH, ATLAS_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, atlas_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenVertexArrays(1, &r->vao);
    glGenBuffers(1, &r->vbo);
    glGenBuffers(1, &r->ebo);

    glBindVertexArray(r->vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    // pos(2 floats) + uv(2 floats) + color(4 bytes) per vertex
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16384 * 4, NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(r->indices), NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 20, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 20, (void*)8);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 20, (void*)16);

    glBindVertexArray(0);
}

void ui_renderer_cleanup(ui_renderer_t* r) {
    glDeleteVertexArrays(1, &r->vao);
    glDeleteBuffers(1, &r->vbo);
    glDeleteBuffers(1, &r->ebo);
    glDeleteTextures(1, &r->atlas_texture);
    glDeleteProgram(r->shader_program);
}

void ui_renderer_resize(ui_renderer_t* r, int screen_width, int screen_height) {
    r->screen_width = screen_width;
    r->screen_height = screen_height;
}

// Combined vertex struct
typedef struct { 
    float   x, y, u, v; 
    uint8_t r, g, b, a; 
} 
ui_vertex_t;

static ui_vertex_t g_vbuf[16384];
static int g_vcount;
static unsigned int g_ibuf[16384 * 6 / 4];
static int g_icount;

static void push_quad(mu_Rect dst, mu_Rect src, mu_Color color) {
    if (g_vcount + 4 > 16384 || g_icount + 6 > (int)(sizeof(g_ibuf) / sizeof(g_ibuf[0]))) return;

    float u0 = (float)src.x / ATLAS_WIDTH, v0 = (float)src.y / ATLAS_HEIGHT;
    float u1 = (float)(src.x + src.w) / ATLAS_WIDTH, v1 = (float)(src.y + src.h) / ATLAS_HEIGHT;

    int base = g_vcount;
    g_vbuf[g_vcount++] = (ui_vertex_t){ (float)dst.x,          (float)dst.y,          u0, v0, color.r, color.g, color.b, color.a };
    g_vbuf[g_vcount++] = (ui_vertex_t){ (float)(dst.x + dst.w),(float)dst.y,          u1, v0, color.r, color.g, color.b, color.a };
    g_vbuf[g_vcount++] = (ui_vertex_t){ (float)(dst.x + dst.w),(float)(dst.y + dst.h),u1, v1, color.r, color.g, color.b, color.a };
    g_vbuf[g_vcount++] = (ui_vertex_t){ (float)dst.x,          (float)(dst.y + dst.h),u0, v1, color.r, color.g, color.b, color.a };

    g_ibuf[g_icount++] = base + 0; g_ibuf[g_icount++] = base + 1; g_ibuf[g_icount++] = base + 2;
    g_ibuf[g_icount++] = base + 2; g_ibuf[g_icount++] = base + 3; g_ibuf[g_icount++] = base + 0;
}

static void push_rect(mu_Rect rect, mu_Color color) {
    push_quad(rect, atlas[ATLAS_WHITE], color);
}

static void push_text(const char* text, mu_Vec2 pos, mu_Color color) {
    mu_Rect dst = { pos.x, pos.y, 0, 0 };
    for (const char* p = text; *p; p++) {
        if ((unsigned char)*p < 127) {
            mu_Rect src = atlas[ATLAS_FONT + (unsigned char)*p];
            dst.w = src.w; dst.h = src.h;
            push_quad(dst, src, color);
            dst.x += src.w;
        }
    }
}

static void push_icon(int id, mu_Rect rect, mu_Color color) {
    mu_Rect src = atlas[id];
    int x = rect.x + (rect.w - src.w) / 2;
    int y = rect.y + (rect.h - src.h) / 2;
    push_quad((mu_Rect) { x, y, src.w, src.h }, src, color);
}

static void flush(ui_renderer_t* r) {
    if (g_vcount == 0) return;

    glUseProgram(r->shader_program);

    float L = 0.0f, R_ = (float)r->screen_width, B = (float)r->screen_height, T = 0.0f;
    float proj[16] = {
        2.0f / (R_ - L), 0, 0, 0,
        0, 2.0f / (T - B), 0, 0,
        0, 0, -1.0f, 0,
        (R_ + L) / (L - R_), (T + B) / (B - T), 0, 1.0f
    };
    glUniformMatrix4fv(r->u_proj_loc, 1, GL_FALSE, proj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, r->atlas_texture);
    glUniform1i(r->u_tex_loc, 0);

    glBindVertexArray(r->vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, g_vcount * sizeof(ui_vertex_t), g_vbuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, g_icount * sizeof(unsigned int), g_ibuf);

    glDrawElements(GL_TRIANGLES, g_icount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    g_vcount = 0;
    g_icount = 0;
}

void ui_renderer_draw(ui_renderer_t* r, mu_Context* ctx) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, r->screen_width, r->screen_height);

    g_vcount = 0;
    g_icount = 0;

    mu_Command* cmd = NULL;
    while (mu_next_command(ctx, &cmd)) {
        if (!cmd) continue;

        switch (cmd->type) {
        case MU_COMMAND_TEXT:
            push_text(cmd->text.str, cmd->text.pos, cmd->text.color);
            break;
        case MU_COMMAND_RECT:
            push_rect(cmd->rect.rect, cmd->rect.color);
            break;
        case MU_COMMAND_ICON:
            push_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color);
            break;
        case MU_COMMAND_CLIP:
            flush(r); // clip rect changes -> must flush what used the old scissor first
            glScissor(cmd->clip.rect.x, r->screen_height - (cmd->clip.rect.y + cmd->clip.rect.h),
                cmd->clip.rect.w, cmd->clip.rect.h);
            break;
        }
    }
    flush(r);

    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}