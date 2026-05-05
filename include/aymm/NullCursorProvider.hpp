#pragma once

#include "aymm/CursorProvider.hpp"

namespace aymm {

// Always returns nullopt. Useful for non-Hyprland sessions or for tests.
class NullCursorProvider final : public CursorProvider {
public:
    std::optional<CursorPos> poll() override { return std::nullopt; }
    const char* name() const override { return "null"; }
};

} // namespace aymm
