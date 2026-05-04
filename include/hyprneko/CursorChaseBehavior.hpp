#pragma once

#include "hyprneko/CursorProvider.hpp"
#include "hyprneko/Pet.hpp"

namespace hyprneko {

class CursorChaseBehavior {
public:
    CursorChaseBehavior(Pet& pet, CursorProvider& provider) : pet_(pet), provider_(provider) {}

    // Polls the cursor and updates the pet's target.
    void update(Pet::Clock::time_point now);

private:
    Pet& pet_;
    CursorProvider& provider_;
};

} // namespace hyprneko
