#pragma once

#include <string>
#include <functional>
#include <vector>

#include "Core/Contracts.hpp"

namespace KeepFavorites::Core {

struct DeleteRunResult {
    int attemptedCount = 0;
    int deletedCount = 0;
    int skippedCount = 0;
    int failedCount = 0;
    std::vector<std::string> notes;
};

class DeleteExecutor {
  public:
    static DeleteRunResult ExecutePlan(SweepPlan const& plan);
};

void SetDeleteProgressCallback(std::function<void(int, int, std::string const&)> callback);

std::string BuildDeleteSummary(DeleteRunResult const& result);

}
