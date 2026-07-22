#include <math.h>

#include "camera.h"

static void mat4_perspective(float* out, float fov_rad, float aspect, float near_val, float far_val) {
    float tan_half_fov = tanf(fov_rad / 2.0f);
    for (int i = 0; i < 16; ++i) out[i] = 0.0f;
    out[0] = 1.0f / (aspect * tan_half_fov);
    out[5] = 1.0f / tan_half_fov;
    out[10] = -(far_val + near_val) / (far_val - near_val);
    out[11] = -1.0f;
    out[14] = -(2.0f * far_val * near_val) / (far_val - near_val);
}

static void mat4_look_at(float* out, vector3_t eye, vector3_t center, vector3_t up) {
    vector3_t f = vector3_norm(vector3_sub(center, eye));
    vector3_t s = vector3_norm(vector3_cross(f, up));
    vector3_t u = vector3_cross(s, f);

    out[0] = s.x;  out[4] = s.y;  out[8] = s.z;  out[12] = -vector3_dot(s, eye);
    out[1] = u.x;  out[5] = u.y;  out[9] = u.z;  out[13] = -vector3_dot(u, eye);
    out[2] = -f.x; out[6] = -f.y; out[10] = -f.z; out[14] = vector3_dot(f, eye);
    out[3] = 0.0f; out[7] = 0.0f; out[11] = 0.0f; out[15] = 1.0f;
}

static void mat4_multiply(float* out, const float* a, const float* b) {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            out[c * 4 + r] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                out[c * 4 + r] += a[k * 4 + r] * b[c * 4 + k];
            }
        }
    }
}

void camera_init(camera_t* cam, vector3_t pos, vector3_t target, vector3_t up, float aspect) {
    cam->position = pos;
    cam->target = target;
    cam->up = up;
    cam->fov_rad = 45.0f * (3.14159265f / 180.0f);
    cam->aspect = aspect;
    cam->near_clip = 0.1f;
    cam->far_clip = 100.0f;
    camera_update_matrix(cam);
}

void camera_update_matrix(camera_t* cam) {
    float proj[16];
    float view[16];
    mat4_perspective(proj, cam->fov_rad, cam->aspect, cam->near_clip, cam->far_clip);
    mat4_look_at(view, cam->position, cam->target, cam->up);
    mat4_multiply(cam->view_proj_matrix, proj, view);
}