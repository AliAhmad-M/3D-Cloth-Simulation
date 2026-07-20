#include <stdlib.h>
#include <math.h>

#include <renderer.h>

static const float PI = 3.14159265359f;

static const char* VS_SRC =
"#version 330 core\n"
"layout(location = 0) in vec3 aPos;\n"
"uniform mat4 uViewProj;\n"
"uniform vec3 uOffset;\n"
"uniform float uScale;\n"
"void main() {\n"
"    vec3 worldPos = (aPos * uScale) + uOffset;\n"
"    gl_Position = uViewProj * vec4(worldPos, 1.0);\n"
"}\n";

static const char* FS_SRC =
"#version 330 core\n"
"out vec4 FragColor;\n"
"void main() {\n"
"    FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
"}\n";

void renderer_init(renderer_t* r) {
    // Compile shader
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &VS_SRC, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &FS_SRC, NULL);
    glCompileShader(fs);

    r->shader_program = glCreateProgram();
    glAttachShader(r->shader_program, vs);
    glAttachShader(r->shader_program, fs);
    glLinkProgram(r->shader_program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    r->u_view_proj_loc = glGetUniformLocation(r->shader_program, "uViewProj");
    r->u_offset_loc = glGetUniformLocation(r->shader_program, "uOffset");
    r->u_scale_loc = glGetUniformLocation(r->shader_program, "uScale");

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
}

void renderer_cleanup(renderer_t* r) {
    glDeleteVertexArrays(1, &r->sphere_vao);
    glDeleteBuffers(1, &r->sphere_vbo);
    glDeleteBuffers(1, &r->sphere_ebo);

    glDeleteVertexArrays(1, &r->line_vao);
    glDeleteBuffers(1, &r->line_vbo);

    glDeleteProgram(r->shader_program);
}

void renderer_draw_sphere(renderer_t* r, vector3_t position, float radius, const float* view_proj) {
    glUseProgram(r->shader_program);
    glUniformMatrix4fv(r->u_view_proj_loc, 1, GL_FALSE, view_proj);
    glUniform3f(r->u_offset_loc, position.x, position.y, position.z);
    glUniform1f(r->u_scale_loc, radius);

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

    glUseProgram(r->shader_program);
    glUniformMatrix4fv(r->u_view_proj_loc, 1, GL_FALSE, view_proj);
    glUniform3f(r->u_offset_loc, 0.0f, 0.0f, 0.0f);
    glUniform1f(r->u_scale_loc, 1.0f);

    glBindVertexArray(r->line_vao);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}