#pragma once

#include <string>
#include <vector>

namespace KeepFavorites::Core {

enum class SongDecisionAction {
    Keep,
    Delete,
    Unresolved,
};

struct SongDecision {
    std::string levelId;
    std::string customLevelPath;
    SongDecisionAction action;
    std::string reason;
};

struct SweepPlan {
    int favoriteCount = 0;
    int scannedCount = 0;
    int keepCount = 0;
    int deleteCount = 0;
    int unresolvedCount = 0;
    std::vector<SongDecision> items;
};

std::string ToString(SongDecisionAction action);
std::string BuildSummary(SweepPlan const& plan);

}
