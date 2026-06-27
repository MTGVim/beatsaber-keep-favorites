#pragma once

#include <optional>

#include "Core/Contracts.hpp"

namespace KeepFavorites::Core {

class SweepPlanner {
  public:
    static std::optional<SweepPlan> TryBuildPlan();
};

}
