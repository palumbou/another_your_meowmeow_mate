#pragma once

#include "hyprneko/CursorProvider.hpp"

namespace hyprneko {

// Always returns nullopt. Useful for non-Hyprland sessions or for tests.
class NullCursorProvider final : public CursorProvider {
public:
    std::optional<CursorPos> poll() override { return std::nullopt; }
    const char* name() const override { return "null"; }
};

} // namespace hyprneko
