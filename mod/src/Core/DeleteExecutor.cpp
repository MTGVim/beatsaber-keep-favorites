#include "Core/DeleteExecutor.hpp"

#include <filesystem>
#include <functional>
#include <fmt/format.h>

namespace KeepFavorites::Core {

namespace {
std::function<void(int, int, std::string const&)> g_progressCallback;
}

void SetDeleteProgressCallback(std::function<void(int, int, std::string const&)> callback) {
    g_progressCallback = std::move(callback);
}

DeleteRunResult DeleteExecutor::ExecutePlan(SweepPlan const& plan) {
    DeleteRunResult result;
    int total = plan.deleteCount;
    int completed = 0;

    for (auto const& item : plan.items) {
        if (item.action != SongDecisionAction::Delete) continue;

        result.attemptedCount += 1;
        auto label = !item.customLevelPath.empty() ? std::filesystem::path(item.customLevelPath).filename().string() : item.levelId;
        auto displayLabel = label.empty() ? std::string("Deleting item...") : label;

        if (g_progressCallback) {
            g_progressCallback(completed, total, displayLabel);
        }

        if (item.customLevelPath.empty()) {
            result.skippedCount += 1;
            result.notes.emplace_back(fmt::format("missing-path:{}", item.levelId));
            completed += 1;
            if (g_progressCallback) {
                g_progressCallback(completed, total, displayLabel);
            }
            continue;
        }

        std::error_code errorCode;
        auto removedCount = std::filesystem::remove_all(std::filesystem::path(item.customLevelPath), errorCode);
        if (errorCode) {
            result.failedCount += 1;
            result.notes.emplace_back(fmt::format("filesystem-delete-failed:{}:{}", item.customLevelPath, errorCode.message()));
        } else if (removedCount == 0) {
            result.skippedCount += 1;
            result.notes.emplace_back(fmt::format("already-missing:{}", item.customLevelPath));
        } else {
            result.deletedCount += 1;
        }

        completed += 1;
        if (g_progressCallback) {
            g_progressCallback(completed, total, displayLabel);
        }
    }

    return result;
}

std::string BuildDeleteSummary(DeleteRunResult const& result) {
    return fmt::format(
        "attempted={}, deleted={}, skipped={}, failed={}",
        result.attemptedCount,
        result.deletedCount,
        result.skippedCount,
        result.failedCount
    );
}

}
