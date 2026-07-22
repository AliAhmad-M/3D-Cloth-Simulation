#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "particle.h"
#include "vector3.h"

static const vector3_t GRAVITY = { 0.0f, -9.81f / 5.0f, 0.0f };
static const float PI = 3.14159265359f;
static const float DAMPING = 0.995f;

void particle_create(particle_t* particle) {
	particle->prev_position = (vector3_t){ 0.0f, 0.0f, 0.0f };
	particle->curr_position = (vector3_t){ 0.0f, 0.0f, 0.0f };
	particle->radius = 1.0f;
	particle->is_fixed = false;

    particle->color_r = 255;
    particle->color_g = 255;
    particle->color_b = 255;
}

void particle_update(particle_t* particle, float dt) {
    if (particle->is_fixed) return;

    vector3_t acceleration_step = vector3_mul(GRAVITY, dt * dt);
    vector3_t velocity = vector3_sub(particle->curr_position, particle->prev_position);

    vector3_t new_position = vector3_add(
        vector3_add(particle->curr_position, vector3_mul(velocity, DAMPING)),
        acceleration_step
    );

    particle->prev_position = particle->curr_position;
    particle->curr_position = new_position;
}