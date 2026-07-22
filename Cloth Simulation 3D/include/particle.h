#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "vector3.h"

typedef struct {
	vector3_t prev_position;
	vector3_t curr_position;

	float radius;
	bool is_fixed;

	uint8_t color_r;
	uint8_t color_g;
	uint8_t color_b;
} particle_t;

void particle_create(particle_t* particle); // Create with default properties
void particle_update(particle_t* particle, float dt);