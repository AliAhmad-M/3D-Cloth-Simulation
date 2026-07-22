#include <stdlib.h>
#include <math.h>

#include "renderer.h"

static const float PI = 3.14159265359f;

static const char* SPHERE_VS_SRC =
"#version 330 core\n"
"layout(location = 0) in vec3 aPos;\n"
"uniform mat4 uViewProj;\n"
"uniform vec3 uOffset;\n"
"uniform float uScale;\n"
"out vec3 vNormal;\n"
"void main() {\n"
"    vNormal = aPos;\n"
"    vec3 worldPos = (aPos * uScale) + uOffset;\n"
"    gl_Position = uViewProj * vec4(worldPos, 1.0);\n"
"}\n";

static const char* SPHERE_FS_SRC =
"#version 330 core\n"
"in vec3 vNormal;\n"
"out vec4 FragColor;\n"
"uniform vec3 uColor;\n"
"uniform vec3 uLightDir;\n"
"void main() {\n"
"    vec3 N = normalize(vNormal);\n"
"    vec3 L = normalize(uLightDir);\n"
"    float diff = max(dot(N, L), 0.0);\n"
"    float ambient = 0.25;\n"
"    vec3 shaded = uColor * (ambient + diff * 0.75);\n"
"    FragColor = vec4(shaded, 1.0);\n"
"}\n";

static const char* LINE_VS_SRC =
"#version 330 core\n"
"layout(location = 0) in vec3 aPos;\n"
"uniform mat4 uViewProj;\n"
"void main() {\n"
"    gl_Position = uViewProj * vec4(aPos, 1.0);\n"
"}\n";

static const char* LINE_FS_SRC =
"#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec3 uColor;\n"
"void main() {\n"
"    FragColor = vec4(uColor, 1.0);\n"
"}\n";

static const char* QUAD_VS_SRC =
"#version 330 core\n"
"layout(location = 0) in vec3 aPos;\n"
"layout(location = 1) in vec3 aNormal;\n"
"uniform mat4 uViewProj;\n"
"out vec3 vNormal;\n"
"void main() {\n"
"    vNormal = aNormal;\n"
"    gl_Position = uViewProj * vec4(aPos, 1.0);\n"
"}\n";

static const char* QUAD_FS_SRC =
"#version 330 core\n"
"in vec3 vNormal;\n"
"out vec4 FragColor;\n"
"uniform vec3 uColor;\n"
"uniform vec3 uLightDir;\n"
"void main() {\n"
"    vec3 N = normalize(vNormal);\n"
"    vec3 L = normalize(uLightDir);\n"
"    float diff = abs(dot(N, L));\n"
"    float ambient = 0.25;\n"
"    vec3 shaded = uColor * (ambient + diff * 0.75);\n"
"    FragColor = vec4(shaded, 1.0);\n"
"}\n";

static GLuint compile_program(const char* vs_src, const char* fs_src) {
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

void renderer_init(renderer_t* r) {
    // Sphere (lit) shader
    r->sphere_shader_program = compile_program(SPHERE_VS_SRC, SPHERE_FS_SRC);
    r->u_view_proj_loc = glGetUniformLocation(r->sphere_shader_program, "uViewProj");
    r->u_offset_loc = glGetUniformLocation(r->sphere_shader_program, "uOffset");
    r->u_scale_loc = glGetUniformLocation(r->sphere_shader_program, "uScale");
    r->u_color_loc = glGetUniformLocation(r->sphere_shader_program, "uColor");
    r->u_light_dir_loc = glGetUniformLocation(r->sphere_shader_program, "uLightDir");

    // Line (unlit) shader
    r->line_shader_program = compile_program(LINE_VS_SRC, LINE_FS_SRC);
    r->u_line_view_proj_loc = glGetUniformLocation(r->line_shader_program, "uViewProj");
    r->u_line_color_loc = glGetUniformLocation(r->line_shader_program, "uColor");

    // Quad (lit) shader
    r->quad_shader_program = compile_program(QUAD_VS_SRC, QUAD_FS_SRC);
    r->u_quad_view_proj_loc = glGetUniformLocation(r->quad_shader_program, "uViewProj");
    r->u_quad_color_loc = glGetUniformLocation(r->quad_shader_program, "uColor");
    r->u_quad_light_dir_loc = glGetUniformLocation(r->quad_shader_program, "uLightDir");

    // Build shader unit mesh
    const int rings = 12, sectors = 12;
    int num_vertices = (rings + 1) * (sectors + 1);
    float* vertices = (float*)malloc(num_vertices * 3 * sizeof(float));

    int v_idx = 0;
    for (int ring = 0; ring <= rings; ++ring) {
        float phi = PI * (float)ring / (float)rings;
        for (int s = 0; s <= sectors; ++s) {
            float theta = 2.0f * PI * (float)s / (float)sectors;
            vertices[v_idx++] = sinf(phi) * cosf(theta);
            vertices[v_idx++] = cosf(phi);
            vertices[v_idx++] = sinf(phi) * sinf(theta);
        }
    }

    r->sphere_index_count = rings * sectors * 6;
    unsigned int* indices = (unsigned int*)malloc(r->sphere_index_count * sizeof(unsigned int));

    int i_idx = 0;
    for (int ring = 0; ring < rings; ++ring) {
        for (int s = 0; s < sectors; ++s) {
            int current = ring * (sectors + 1) + s;
            int next = current + sectors + 1;
            indices[i_idx++] = current;     indices[i_idx++] = next;     indices[i_idx++] = current + 1;
            indices[i_idx++] = current + 1; indices[i_idx++] = next;     indices[i_idx++] = next + 1;
        }
    }

    glGenVertexArrays(1, &r->sphere_vao);
    glGenBuffers(1, &r->sphere_vbo);
    glGenBuffers(1, &r->sphere_ebo);

    glBindVertexArray(r->sphere_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->sphere_vbo);
    glBufferData(GL_ARRAY_BUFFER, num_vertices * 3 * sizeof(float), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->sphere_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, r->sphere_index_count * sizeof(unsigned int), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    free(vertices);
    free(indices);

    // Line buffer
    glGenVertexArrays(1, &r->line_vao);
    glGenBuffers(1, &r->line_vbo);

    glBindVertexArray(r->line_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->line_vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);

    // Quad buffer
    glGenVertexArrays(1, &r->quad_vao);
    glGenBuffers(1, &r->quad_vbo);

    glBindVertexArray(r->quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->quad_vbo);
    
    glBufferData(GL_ARRAY_BUFFER, 6 * 6 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void renderer_cleanup(renderer_t* r) {
    glDeleteVertexArrays(1, &r->sphere_vao);
    glDeleteBuffers(1, &r->sphere_vbo);
    glDeleteBuffers(1, &r->sphere_ebo);

    glDeleteVertexArrays(1, &r->line_vao);
    glDeleteBuffers(1, &r->line_vbo);

    glDeleteProgram(r->sphere_shader_program);
    glDeleteProgram(r->line_shader_program);
}

void renderer_draw_sphere(
    renderer_t* r, vector3_t position, float radius,
    uint8_t color_r, uint8_t color_g, uint8_t color_b,
    const float* view_proj) {
    glUseProgram(r->sphere_shader_program);
    glUniformMatrix4fv(r->u_view_proj_loc, 1, GL_FALSE, view_proj);
    glUniform3f(r->u_offset_loc, position.x, position.y, position.z);
    glUniform1f(r->u_scale_loc, radius);
    glUniform3f(r->u_color_loc, color_r / 255.0f, color_g / 255.0f, color_b / 255.0f);
    glUniform3f(r->u_light_dir_loc, 0.4f, 1.0f, 0.3f); // pointing toward the light

    glBindVertexArray(r->sphere_vao);
    glDrawElements(GL_TRIANGLES, r->sphere_index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void renderer_draw_line(renderer_t* r, vector3_t start, vector3_t end, const float* view_proj) {
    float line_data[6] = {
        start.x, start.y, start.z,
        end.x,   end.y,   end.z
    };

    glBindBuffer(GL_ARRAY_BUFFER, r->line_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line_data), line_data);

    glUseProgram(r->line_shader_program);
    glUniformMatrix4fv(r->u_line_view_proj_loc, 1, GL_FALSE, view_proj);
    glUniform3f(r->u_line_color_loc, 0.85f, 0.85f, 0.85f);

    glBindVertexArray(r->line_vao);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}

void renderer_draw_quad(
    renderer_t* r, 
    vector3_t p0, vector3_t p1, vector3_t p2, vector3_t p3,
    vector3_t n0, vector3_t n1, vector3_t n2, vector3_t n3,
    uint8_t color_r, uint8_t color_g, uint8_t color_b,
    const float* view_proj) {

    vector3_t verts[6] = { p0, p1, p2, p0, p2, p3 };
    vector3_t normals[6] = { n0, n1, n2, n0, n2, n3 };

    float data[36];
    int idx = 0;
    for (int i = 0; i < 6; ++i) {
        data[idx++] = verts[i].x;
        data[idx++] = verts[i].y;
        data[idx++] = verts[i].z;
        data[idx++] = normals[i].x;
        data[idx++] = normals[i].y;
        data[idx++] = normals[i].z;
    }

    glBindBuffer(GL_ARRAY_BUFFER, r->quad_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data), data);

    glUseProgram(r->quad_shader_program);
    glUniformMatrix4fv(r->u_quad_view_proj_loc, 1, GL_FALSE, view_proj);
    glUniform3f(r->u_quad_color_loc, color_r / 255.0f, color_g / 255.0f, color_b / 255.0f);
    glUniform3f(r->u_quad_light_dir_loc, 0.4f, 1.0f, 0.3f);

    glBindVertexArray(r->quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}