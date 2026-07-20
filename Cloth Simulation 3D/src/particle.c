#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <particle.h>
#include <vector3.h>

static const vector3_t GRAVITY = { 0.0f, -9.81f, 0.0f };
static const float PI = 3.14159265359f;

void particle_create(particle_t* particle) {
	particle->prev_position = (vector3_t){ 0.0f, 0.0f, 0.0f };
	particle->curr_position = (vector3_t){ 0.0f, 0.0f, 0.0f };
	particle->radius = 1.0f;
	particle->is_fixed = false;
}

void particle_update(particle_t* particle, float dt) {
    if (particle->is_fixed) return;

    // Verlet update: x(t + dt) = 2*x(t) - x(t - dt) + a * dt^2
    vector3_t acceleration_step = vector3_mul(GRAVITY, dt * dt);

    vector3_t position_term = vector3_sub(
        vector3_mul(particle->curr_position, 2.0f), 
        particle->prev_position
    );
    vector3_t new_position = vector3_add(position_term, acceleration_step);

    // Update state
    particle->prev_position = particle->curr_position;
    particle->curr_position = new_position;
}