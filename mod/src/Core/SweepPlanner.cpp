#include "Core/SweepPlanner.hpp"

#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "System/Collections/Generic/HashSet_1.hpp"
#include "UnityEngine/Object.hpp"
#include "songcore/shared/SongCore.hpp"
#include "songcore/shared/SongLoader/CustomBeatmapLevel.hpp"

namespace KeepFavorites::Core {
namespace {

bool IsCustomLevelId(std::string const& levelId) {
    return levelId.starts_with("custom_level_");
}

}

std::optional<SweepPlan> SweepPlanner::TryBuildPlan() {
    auto* playerDataModel = UnityEngine::Object::FindObjectOfType<GlobalNamespace::PlayerDataModel*>();
    if (!playerDataModel) return std::nullopt;

    auto* playerData = playerDataModel->get_playerData();
    if (!playerData) return std::nullopt;

    auto* favorites = playerData->get_favoritesLevelIds();
    if (!favorites) return std::nullopt;

    SweepPlan plan;
    plan.favoriteCount = favorites->get_Count();

    auto levels = SongCore::API::Loading::GetAllLevels();
    plan.items.reserve(levels.size());

    for (auto* level : levels) {
        if (!level) continue;

        SongDecision decision;
        auto rawLevelId = level->levelID;
        decision.levelId = rawLevelId ? static_cast<std::string>(rawLevelId) : "";
        decision.customLevelPath = std::string(level->get_customLevelPath());

        plan.scannedCount += 1;

        if (decision.levelId.empty() || decision.customLevelPath.empty()) {
            decision.action = SongDecisionAction::Unresolved;
            decision.reason = "missing-level-id-or-path";
            plan.unresolvedCount += 1;
            plan.items.emplace_back(std::move(decision));
            continue;
        }

        if (!IsCustomLevelId(decision.levelId)) {
            decision.action = SongDecisionAction::Unresolved;
            decision.reason = "non-custom-level-id";
            plan.unresolvedCount += 1;
            plan.items.emplace_back(std::move(decision));
            continue;
        }

        bool isFavorite = favorites->Contains(rawLevelId);
        if (isFavorite) {
            decision.action = SongDecisionAction::Keep;
            decision.reason = "favoritesLevelIds-contains-level-id";
            plan.keepCount += 1;
        } else {
            decision.action = SongDecisionAction::Delete;
            decision.reason = "not-present-in-favoritesLevelIds";
            plan.deleteCount += 1;
        }

        plan.items.emplace_back(std::move(decision));
    }

    return plan;
}

}
