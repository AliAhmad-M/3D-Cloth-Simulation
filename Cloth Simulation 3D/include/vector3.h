#pragma once
#include <stdbool.h>

typedef struct 
{
	float x;
	float y;
	float z;
} vector3_t;

vector3_t vector3_add(vector3_t a, vector3_t b);
vector3_t vector3_sub(vector3_t a, vector3_t b);
vector3_t vector3_mul(vector3_t a, float s);
vector3_t vector3_div(vector3_t a, float s);
vector3_t vector3_neg(vector3_t a);
vector3_t vector3_norm(vector3_t a);
vector3_t vector3_cross(vector3_t a, vector3_t b);

float vector3_dist_sq(vector3_t a, vector3_t b);
float vector3_dist(vector3_t a, vector3_t b);
float vector3_len_sq(vector3_t a);
float vector3_len(vector3_t a);
float vector3_dot(vector3_t a, vector3_t b);

bool vector3_equals(vector3_t a, vector3_t b, float epsilon);
bool vector3_is_zero(vector3_t a);