#include "Hooks/MainFlowCoordinatorHooks.hpp"

#include <algorithm>
#include <optional>
#include <ranges>
#include <string>

#include <fmt/format.h>
#include "Core/DeleteExecutor.hpp"
#include "Core/SweepPlanner.hpp"
#include "GlobalNamespace/MainFlowCoordinator.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "bsml/shared/BSML/Components/ModalView.hpp"
#include "main.hpp"

namespace KeepFavorites::Hooks {
namespace {

BSML::ModalView* modal = nullptr;
HMUI::CurvedTextMeshPro* statusText = nullptr;
UnityEngine::UI::Button* confirmButton = nullptr;
UnityEngine::UI::Button* dismissButton = nullptr;
std::optional<KeepFavorites::Core::SweepPlan> displayedPlan;
bool promptShownThisActivation = false;

bool IsAlive(UnityEngine::Object* object) {
    return object && !UnityEngine::Object::CompareBaseObjects(object, nullptr);
}

void SetStatusText(std::string const& message) {
    if (!IsAlive(statusText)) return;
    statusText->set_text(il2cpp_utils::newcsstr(message));
}

void SetButtonLabels(std::string const& confirmLabel, std::string const& dismissLabel) {
    if (IsAlive(confirmButton)) {
        BSML::Lite::SetButtonText(confirmButton, il2cpp_utils::newcsstr(confirmLabel));
    }
    if (IsAlive(dismissButton)) {
        BSML::Lite::SetButtonText(dismissButton, il2cpp_utils::newcsstr(dismissLabel));
    }
}

std::string BuildPromptText(KeepFavorites::Core::SweepPlan const& plan) {
    std::string summary = KeepFavorites::Core::BuildSummary(plan);
    if (plan.deleteCount <= 0) {
        return fmt::format("KeepFavorites scan complete. {}. No non-favorite custom songs need deletion.", summary);
    }

    return fmt::format(
        "KeepFavorites scan complete. {}. Confirm to delete {} non-favorite custom song folders.",
        summary,
        plan.deleteCount
    );
}

void HidePrompt() {
    promptShownThisActivation = false;
    if (!IsAlive(modal)) return;
    modal->Hide();
}

void EnsureModal(GlobalNamespace::MainFlowCoordinator* self) {
    if (IsAlive(modal) || !self) return;

    modal = nullptr;
    statusText = nullptr;
    confirmButton = nullptr;
    dismissButton = nullptr;

    modal = BSML::Lite::CreateModal(self->get_transform(), UnityEngine::Vector2{86.0f, 52.0f}, []() { HidePrompt(); });
    auto* container = BSML::Lite::CreateScrollableModalContainer(modal);
    auto* layout = BSML::Lite::CreateVerticalLayoutGroup(container);

    statusText = BSML::Lite::CreateText(layout, il2cpp_utils::newcsstr("KeepFavorites is preparing the sweep plan..."), 4.0f);
    confirmButton = BSML::Lite::CreateUIButton(layout, il2cpp_utils::newcsstr("Delete non-favorites"), []() {
        if (!displayedPlan.has_value()) {
            SetStatusText("KeepFavorites has no confirmed sweep plan yet. Reopen the main menu to scan again.");
            PaperLogger.info("KeepFavorites delete aborted: no displayed plan was available during confirm");
            return;
        }

        if (displayedPlan->deleteCount <= 0) {
            SetStatusText("KeepFavorites scan says there are no non-favorite custom songs to delete.");
            SetButtonLabels("Scan again", "Close");
            return;
        }

        SetStatusText(fmt::format("Deleting {} non-favorite custom songs...", displayedPlan->deleteCount));
        auto result = KeepFavorites::Core::DeleteExecutor::ExecutePlan(*displayedPlan);
        auto summary = KeepFavorites::Core::BuildDeleteSummary(result);
        PaperLogger.info("KeepFavorites delete summary: {}", summary);
        for (auto const& note : result.notes | std::views::take(5)) {
            PaperLogger.info("KeepFavorites delete note: {}", note);
        }

        auto rescan = KeepFavorites::Core::SweepPlanner::TryBuildPlan();
        if (rescan.has_value()) {
            displayedPlan = *rescan;
            SetStatusText(fmt::format("Delete run finished. {}. Rescan: {}", summary, KeepFavorites::Core::BuildSummary(*rescan)));
        } else {
            displayedPlan.reset();
            SetStatusText(fmt::format("Delete run finished. {}. Rescan unavailable until SongCore is ready again.", summary));
        }
        SetButtonLabels("Scan again", "Close");
    });
    dismissButton = BSML::Lite::CreateUIButton(layout, il2cpp_utils::newcsstr("Not now"), []() { HidePrompt(); });
}

void ShowPrompt(GlobalNamespace::MainFlowCoordinator* self, KeepFavorites::Core::SweepPlan const& plan) {
    EnsureModal(self);
    if (!IsAlive(modal)) return;

    displayedPlan = plan;
    SetStatusText(BuildPromptText(plan));
    SetButtonLabels("Delete non-favorites", "Not now");

    if (promptShownThisActivation) return;
    promptShownThisActivation = true;
    modal->Show();
}

MAKE_HOOK_MATCH(
    MainFlowCoordinator_DidActivate,
    &GlobalNamespace::MainFlowCoordinator::DidActivate,
    void,
    GlobalNamespace::MainFlowCoordinator* self,
    bool firstActivation,
    bool addedToHierarchy,
    bool screenSystemEnabling
) {
    MainFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    auto plan = KeepFavorites::Core::SweepPlanner::TryBuildPlan();
    if (!plan.has_value()) {
        PaperLogger.info("KeepFavorites scan not ready yet: favorites or SongCore runtime was unavailable");
        return;
    }

    PaperLogger.info("KeepFavorites scan summary: {}", KeepFavorites::Core::BuildSummary(*plan));

    int sampleBudget = 5;
    for (auto const& item : plan->items) {
        if (item.action != KeepFavorites::Core::SongDecisionAction::Delete) continue;
        PaperLogger.info("Delete candidate: {} ({})", item.customLevelPath, item.reason);
        sampleBudget -= 1;
        if (sampleBudget <= 0) break;
    }

    // Temporarily avoid auto-showing BSML modal here.
    // Main menu lifecycle timing on Quest 1.40.8 is sensitive, and showing a modal
    // immediately after DidActivate can black-screen the menu before the content is stable.
    PaperLogger.info("KeepFavorites prompt suppressed for now: scan completed without presenting the legacy modal");
}

}

void InstallMainFlowCoordinatorHooks() {
    INSTALL_HOOK(PaperLogger, MainFlowCoordinator_DidActivate);
}

}
