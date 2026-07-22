#include <math.h>

#include "vector3.h"

static const float EPSILON = 0.000001f;

vector3_t vector3_add(vector3_t a, vector3_t b) {
	vector3_t c = { a.x + b.x, a.y + b.y, a.z + b.z };
	return c;
}

vector3_t vector3_sub(vector3_t a, vector3_t b) {
	vector3_t c = { a.x - b.x, a.y - b.y, a.z - b.z };
	return c;
}

vector3_t vector3_mul(vector3_t a, float s) {
	vector3_t c = { a.x * s, a.y * s, a.z * s };
	return c;
}

vector3_t vector3_div(vector3_t a, float s) {
	vector3_t c = { a.x / s, a.y / s, a.z / s };
	return c;
}

vector3_t vector3_neg(vector3_t a) {
	vector3_t c = { -a.x, -a.y, -a.z };
	return c;
}

vector3_t vector3_norm(vector3_t a) {
	float len = vector3_len(a);
	if (len < EPSILON) {
		vector3_t c = { 0.0f, 0.0f, 0.0f };
		return c;
	}
	return vector3_div(a, len);
}

vector3_t vector3_cross(vector3_t a, vector3_t b) {
	vector3_t c = {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
	return c;
}

float vector3_dot(vector3_t a, vector3_t b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float vector3_len_sq(vector3_t a) {
	return vector3_dot(a, a);
}

float vector3_len(vector3_t a) {
	return sqrtf(vector3_len_sq(a));
}

float vector3_dist_sq(vector3_t a, vector3_t b) {
	return vector3_len_sq(vector3_sub(a, b));
}

float vector3_dist(vector3_t a, vector3_t b) {
	return sqrtf(vector3_dist_sq(a, b));
}

bool vector3_equals(vector3_t a, vector3_t b, float epsilon) {
	return fabsf(a.x - b.x) <= epsilon &&
		fabsf(a.y - b.y) <= epsilon &&
		fabsf(a.z - b.z) <= epsilon;
}

bool vector3_is_zero(vector3_t a) {
	return vector3_len_sq(a) < (EPSILON * EPSILON);
}