#include <stdio.h>
#include <glad/glad.h>

#include "constraint.h"

constraint_t cloth_constraint_add(particle_t* a, particle_t* b) {
	if (!a || !b) {
		fprintf(stderr, "Failed to add constraint, particle is NULL\n");
		return (constraint_t) { NULL, NULL, -1.0f, false };
	}

	float initial_dist = vector3_dist(a->curr_position, b->curr_position);
	return (constraint_t) { a, b, initial_dist, true };
}

void cloth_constraint_resolve(constraint_t* c) {
    if (!c->is_active || (c->a->is_fixed && c->b->is_fixed))
        return;

    // Vector from A to B
    vector3_t delta = vector3_sub(c->b->curr_position, c->a->curr_position);
    float curr_dist = vector3_len(delta);

    // Avoid division by 0
    if (curr_dist < 0.0001f)
        return;

    // Compute correction
    float delta_length = (curr_dist - c->initial_dist) / curr_dist;
    vector3_t correction = vector3_mul(delta, delta_length);

    // Apply correction
    if (c->a->is_fixed) {
        c->b->curr_position = vector3_sub(c->b->curr_position, correction);
    }
    else if (c->b->is_fixed) {
        c->a->curr_position = vector3_add(c->a->curr_position, correction);
    }
    else {
        vector3_t half_correction = vector3_mul(correction, 0.5f);
        c->a->curr_position = vector3_add(c->a->curr_position, half_correction);
        c->b->curr_position = vector3_sub(c->b->curr_position, half_correction);
    }
}