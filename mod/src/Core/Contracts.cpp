#include "Core/Contracts.hpp"

#include <fmt/format.h>

namespace KeepFavorites::Core {

std::string ToString(SongDecisionAction action) {
    switch (action) {
        case SongDecisionAction::Keep:
            return "keep";
        case SongDecisionAction::Delete:
            return "delete";
        case SongDecisionAction::Unresolved:
            return "unresolved";
    }

    return "unknown";
}

std::string BuildSummary(SweepPlan const& plan) {
    return fmt::format(
        "favorites={}, scanned={}, keep={}, delete={}, unresolved={}",
        plan.favoriteCount,
        plan.scannedCount,
        plan.keepCount,
        plan.deleteCount,
        plan.unresolvedCount
    );
}

}
