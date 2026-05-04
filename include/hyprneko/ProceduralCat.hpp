#pragma once

#include "hyprneko/PetStateMachine.hpp"

typedef struct _cairo cairo_t;

namespace hyprneko {

// Built-in fallback renderer used when no PNG sprite sheet is configured.
// Draws a small cat with Cairo paths so we ship a recognizable pet without
// any image asset (and without inheriting any third-party sprite license).
//
// (cx, cy) is the pet center in the current cairo coordinate space.
// `frame` should advance once per state-machine tick — used for tail wag and
// run-cycle leg alternation.
void draw_procedural_cat(cairo_t* cr,
                         double cx, double cy,
                         double scale,
                         PetState state,
                         Direction direction,
                         int frame);

} // namespace hyprneko
