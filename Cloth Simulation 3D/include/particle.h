#pragma once
#include <stdbool.h>
#include <vector3.h>

typedef struct {
	vector3_t prev_position;
	vector3_t curr_position;

	float radius;
	bool is_fixed;
} particle_t;

void particle_create(particle_t* particle); // Create with default properties
void particle_update(particle_t* particle, float dt);