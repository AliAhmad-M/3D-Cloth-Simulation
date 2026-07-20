#pragma once
#include <stdbool.h>
#include <particle.h>

typedef struct {
	particle_t* a;
	particle_t* b;

	float initial_dist;
	bool is_active;
} cloth_constraint_t;

cloth_constraint_t cloth_constraint_add(particle_t* a, particle_t* b);
void cloth_constraint_resolve(cloth_constraint_t* c);