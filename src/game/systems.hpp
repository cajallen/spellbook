#pragma once

namespace spellbook {

struct Scene;

// Order does not represent execution order.
// Could order these as a DAG.... ...hmm....

// Handles wanderer behavior
void travel_system(Scene* scene);

// Creates the frame renderable for health bars
void health_draw_system(Scene* scene);

// Uses the transform for models
void transform_system(Scene* scene);

void consumer_system(Scene* scene);
void health_system(Scene* scene);
void disposal_system(Scene* scene);
void selection_id_system(Scene* scene);
void dragging_update_system(Scene* scene);
void dragging_system(Scene* scene);
void collision_update_system(Scene* scene);

void tower_system(Scene* scene);
void pyro_system(Scene* scene);
void roller_system(Scene* scene);
void rollee_system(Scene* scene);

void spawner_draw_system(Scene* scene);

}
