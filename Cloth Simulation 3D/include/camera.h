#pragma once
#include <vector3.h>

typedef struct {
    vector3_t position;
    vector3_t target;
    vector3_t up;

    float fov_rad;
    float aspect;
    float near_clip;
    float far_clip;

    float view_proj_matrix[16];
} camera_t;

void camera_init(camera_t* cam, vector3_t pos, vector3_t target, vector3_t up, float aspect);
void camera_update_matrix(camera_t* cam);