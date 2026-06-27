#include "UI/KeepFavoritesMenu.hpp"

#include <chrono>
#include <exception>
#include <future>
#include <mutex>
#include <optional>
#include <ranges>
#include <string>

#include <fmt/format.h>
#include "Core/DeleteExecutor.hpp"
#include "Core/SweepPlanner.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/Touchable.hpp"
#include "HMUI/ViewController.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/TextAnchor.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/FlowCoordinators/MainMenuHolderFlowCoordinator.hpp"
#include "bsml/shared/BSML/SharedCoroutineStarter.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "main.hpp"
#include "songcore/shared/SongCore.hpp"

namespace KeepFavorites::UI {
namespace {

HMUI::CurvedTextMeshPro* progressText = nullptr;
HMUI::CurvedTextMeshPro* fileText = nullptr;
HMUI::ViewController* activeViewController = nullptr;
UnityEngine::UI::Button* scanButton = nullptr;
UnityEngine::UI::Button* deleteButton = nullptr;
std::optional<KeepFavorites::Core::SweepPlan> currentPlan;
bool deleteInFlight = false;
bool scanInFlight = false;
std::future<KeepFavorites::Core::DeleteRunResult> deleteFuture;
std::shared_future<void> scanFuture;
std::mutex deleteStateMutex;
int deleteProgressCompleted = 0;
int deleteProgressTotal = 0;
std::string deleteCurrentItem = "Press Scan first.";
std::string statusLine1 = "KeepFavorites ready.";
std::string statusLine2 = "Press Scan first.";

void SyncStatusToUI();
void SetStatusLines(std::string line1, std::string line2);

bool IsAlive(UnityEngine::Object* object) {
    return object && !UnityEngine::Object::CompareBaseObjects(object, nullptr);
}

void SetButtonsInteractable(bool interactable) {
    if (IsAlive(scanButton)) {
        scanButton->set_interactable(interactable);
    }
    if (IsAlive(deleteButton)) {
        deleteButton->set_interactable(interactable);
    }
}

void SetBackNavigationLocked(bool locked) {
    auto holders = UnityEngine::Resources::FindObjectsOfTypeAll<BSML::MainMenuHolderFlowCoordinator*>();
    for (auto holder : holders) {
        if (!holder) continue;
        holder->set_showBackButton(!locked);
    }
}

void UpdateDeleteSnapshot(int completed, int total, std::string item) {
    std::scoped_lock lock(deleteStateMutex);
    deleteProgressCompleted = completed;
    deleteProgressTotal = total;
    deleteCurrentItem = std::move(item);
}

void ReadDeleteSnapshot(int& completed, int& total, std::string& item) {
    std::scoped_lock lock(deleteStateMutex);
    completed = deleteProgressCompleted;
    total = deleteProgressTotal;
    item = deleteCurrentItem;
}

void ConfigureText(HMUI::CurvedTextMeshPro* text, float preferredHeight, float fontSize, bool enableWordWrapping) {
    if (!text) {
        return;
    }

    auto rect = text->transform.cast<UnityEngine::RectTransform>();
    if (rect) {
        rect->set_anchorMin(UnityEngine::Vector2(0.0f, 1.0f));
        rect->set_anchorMax(UnityEngine::Vector2(1.0f, 1.0f));
        rect->set_pivot(UnityEngine::Vector2(0.5f, 0.5f));
        rect->set_anchoredPosition(UnityEngine::Vector2(0.0f, 0.0f));
        rect->set_sizeDelta(UnityEngine::Vector2(0.0f, preferredHeight));
    }

    auto* layoutElement = text->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>();
    if (!layoutElement) {
        layoutElement = text->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
    }
    if (layoutElement) {
        layoutElement->set_ignoreLayout(false);
        layoutElement->set_minHeight(preferredHeight);
        layoutElement->set_preferredHeight(preferredHeight);
        layoutElement->set_flexibleHeight(0.0f);
        layoutElement->set_flexibleWidth(1.0f);
    }

    text->set_fontSize(fontSize);
    text->set_alignment(TMPro::TextAlignmentOptions::TopLeft);
    text->set_enableWordWrapping(enableWordWrapping);
}

void ApplyPlanToStatus() {
    if (!currentPlan.has_value()) {
        SetStatusLines(
            "KeepFavorites could not read favorites or SongCore yet.",
            "Open this screen again after the main menu settles."
        );
        return;
    }

    auto summary = KeepFavorites::Core::BuildSummary(*currentPlan);
    if (currentPlan->deleteCount <= 0) {
        SetStatusLines(
            fmt::format("Scan complete. {}", summary),
            "No non-favorite custom songs need deletion."
        );
        return;
    }

    SetStatusLines(
        fmt::format("Scan complete. {}", summary),
        fmt::format("Delete will remove {} non-favorite song folders.", currentPlan->deleteCount)
    );
}

void SyncStatusToUI() {
    if (IsAlive(progressText)) {
        progressText->set_text(il2cpp_utils::newcsstr(statusLine1));
    }
    if (IsAlive(fileText)) {
        fileText->set_text(il2cpp_utils::newcsstr(statusLine2));
    }
}

void SetStatusLines(std::string line1, std::string line2) {
    statusLine1 = std::move(line1);
    statusLine2 = std::move(line2);
    SyncStatusToUI();
}

custom_types::Helpers::Coroutine AwaitDeleteCoroutine() {
    using namespace std::chrono_literals;

    while (deleteFuture.valid() && deleteFuture.wait_for(0ms) != std::future_status::ready) {
        int completed = 0;
        int total = 0;
        std::string item;
        ReadDeleteSnapshot(completed, total, item);
        SetStatusLines(
            fmt::format("Deleting {}/{}...", completed, total),
            item
        );
        co_yield nullptr;
    }

    KeepFavorites::Core::DeleteRunResult result;
    if (deleteFuture.valid()) {
        try {
            result = deleteFuture.get();
        } catch (std::exception const& ex) {
            result.failedCount += 1;
            result.notes.emplace_back(fmt::format("batch-delete-exception:{}", ex.what()));
        } catch (...) {
            result.failedCount += 1;
            result.notes.emplace_back("batch-delete-exception:unknown");
        }
    }
    KeepFavorites::Core::SetDeleteProgressCallback({});

    for (auto const& note : result.notes | std::views::take(5)) {
        PaperLogger.info("KeepFavorites delete note: {}", note);
    }

    auto deleteSummary = KeepFavorites::Core::BuildDeleteSummary(result);
    PaperLogger.info("KeepFavorites delete summary: {}", deleteSummary);

    SetStatusLines("Refreshing SongCore...", deleteSummary);
    auto refreshFuture = SongCore::API::Loading::RefreshSongs(true);
    if (refreshFuture.valid()) {
        while (refreshFuture.wait_for(0ms) != std::future_status::ready) {
            co_yield nullptr;
        }

        try {
            refreshFuture.get();
        } catch (std::exception const& ex) {
            result.notes.emplace_back(fmt::format("refresh-exception:{}", ex.what()));
            PaperLogger.info("KeepFavorites refresh exception: {}", ex.what());
        } catch (...) {
            result.notes.emplace_back("refresh-exception:unknown");
            PaperLogger.info("KeepFavorites refresh exception: unknown");
        }
    } else {
        result.notes.emplace_back("refresh-future-invalid");
        PaperLogger.info("KeepFavorites refresh future was invalid after batch delete");
    }

    currentPlan.reset();
    SetStatusLines(
        fmt::format("Delete finished. {}", KeepFavorites::Core::BuildDeleteSummary(result)),
        "Press Scan favorites for a fresh post-delete rescan."
    );

    deleteInFlight = false;
    SetButtonsInteractable(true);
    SetBackNavigationLocked(false);
    co_return;
}

custom_types::Helpers::Coroutine AwaitScanCoroutine() {
    using namespace std::chrono_literals;

    while (scanFuture.valid() && scanFuture.wait_for(0ms) != std::future_status::ready) {
        co_yield nullptr;
    }

    bool refreshSucceeded = true;
    if (scanFuture.valid()) {
        try {
            scanFuture.get();
        } catch (std::exception const& ex) {
            refreshSucceeded = false;
            PaperLogger.info("KeepFavorites scan refresh exception: {}", ex.what());
            SetStatusLines("SongCore refresh failed.", fmt::format("Exception: {}", ex.what()));
        } catch (...) {
            refreshSucceeded = false;
            PaperLogger.info("KeepFavorites scan refresh exception: unknown");
            SetStatusLines("SongCore refresh failed.", "Unknown refresh exception.");
        }
    } else {
        refreshSucceeded = false;
        PaperLogger.info("KeepFavorites scan refresh future was invalid");
        SetStatusLines("SongCore refresh failed.", "Refresh future was invalid.");
    }

    if (refreshSucceeded) {
        currentPlan = KeepFavorites::Core::SweepPlanner::TryBuildPlan();
        ApplyPlanToStatus();
    } else {
        currentPlan.reset();
    }

    scanInFlight = false;
    SetButtonsInteractable(true);
    co_return;
}

void RefreshPlan() {
    if (deleteInFlight) {
        SetStatusLines(
            fmt::format("Deleting {}/{}...", deleteProgressCompleted, deleteProgressTotal),
            "Delete is running. Scan will unlock when it finishes."
        );
        return;
    }

    if (scanInFlight) {
        SetStatusLines("Refreshing SongCore...", "Fresh scan is already running.");
        return;
    }

    scanInFlight = true;
    currentPlan.reset();
    SetButtonsInteractable(false);
    SetStatusLines("Refreshing SongCore...", "Building a fresh scan result...");

    scanFuture = SongCore::API::Loading::RefreshSongs(true);
    BSML::SharedCoroutineStarter::StartCoroutine(custom_types::Helpers::CoroutineHelper::New(AwaitScanCoroutine()));
}

void ExecuteDelete() {
    if (scanInFlight) {
        SetStatusLines("Refreshing SongCore...", "Wait for the fresh scan result before deleting.");
        return;
    }

    if (deleteInFlight) {
        SetStatusLines(
            fmt::format("Deleting {}/{}...", deleteProgressCompleted, deleteProgressTotal),
            "Delete is already running. Stay on this screen until it finishes."
        );
        return;
    }

    if (!currentPlan.has_value()) {
        SetStatusLines("No sweep plan is loaded yet.", "Press Scan favorites first.");
        return;
    }

    if (currentPlan->deleteCount <= 0) {
        SetStatusLines("There are no non-favorite custom songs to delete.", "Nothing to remove right now.");
        return;
    }

    deleteInFlight = true;
    SetButtonsInteractable(false);
    SetBackNavigationLocked(true);
    UpdateDeleteSnapshot(0, currentPlan->deleteCount, fmt::format("Queued {} song folders for batch delete.", currentPlan->deleteCount));
    SetStatusLines(
        fmt::format("Deleting 0/{}...", currentPlan->deleteCount),
        fmt::format("Queued {} song folders for batch delete.", currentPlan->deleteCount)
    );

    auto plan = *currentPlan;
    KeepFavorites::Core::SetDeleteProgressCallback([](int completed, int total, std::string const& item) {
        UpdateDeleteSnapshot(completed, total, item);
    });
    deleteFuture = std::async(std::launch::async, [plan]() {
        return KeepFavorites::Core::DeleteExecutor::ExecutePlan(plan);
    });
    BSML::SharedCoroutineStarter::StartCoroutine(custom_types::Helpers::CoroutineHelper::New(AwaitDeleteCoroutine()));
}

void MainMenuViewDidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    activeViewController = self;

    if (!firstActivation) {
        SetButtonsInteractable(!(deleteInFlight || scanInFlight));
        SetBackNavigationLocked(deleteInFlight);
        SyncStatusToUI();
        return;
    }

    if (!self->get_gameObject()->GetComponent<HMUI::Touchable*>()) {
        self->get_gameObject()->AddComponent<HMUI::Touchable*>();
    }

    auto* container = BSML::Lite::CreateScrollableSettingsContainer(self->get_transform());
    UnityEngine::Transform* parent = container->get_transform();

    auto* layoutGroup = container->GetComponent<UnityEngine::UI::VerticalLayoutGroup*>();
    if (layoutGroup) {
        layoutGroup->set_childAlignment(UnityEngine::TextAnchor::UpperLeft);
        layoutGroup->set_childControlWidth(true);
        layoutGroup->set_childControlHeight(true);
        layoutGroup->set_childForceExpandWidth(true);
        layoutGroup->set_childForceExpandHeight(false);
        layoutGroup->set_spacing(0.75f);
    }

    scanButton = BSML::Lite::CreateUIButton(parent, il2cpp_utils::newcsstr("Scan favorites"), []() { RefreshPlan(); });
    deleteButton = BSML::Lite::CreateUIButton(parent, il2cpp_utils::newcsstr("Delete non-favorites"), []() { ExecuteDelete(); });

    progressText = BSML::Lite::CreateText(parent, il2cpp_utils::newcsstr("KeepFavorites ready."), 3.0f, {0.0f, 0.0f}, {0.0f, 6.0f});
    fileText = BSML::Lite::CreateText(parent, il2cpp_utils::newcsstr("Press Scan first."), 2.35f, {0.0f, 0.0f}, {0.0f, 12.0f});
    ConfigureText(progressText, 6.0f, 3.0f, false);
    ConfigureText(fileText, 12.0f, 2.35f, true);

    PaperLogger.info(
        "KeepFavorites settings menu activated: firstActivation={}, addedToHierarchy={}, screenSystemEnabling={}",
        firstActivation,
        addedToHierarchy,
        screenSystemEnabling
    );
    SetButtonsInteractable(!(deleteInFlight || scanInFlight));
    SetBackNavigationLocked(deleteInFlight);
    SyncStatusToUI();
}

}

void InstallMenuEntry() {
    BSML::Register::RegisterMainMenu(
        "Keep Favorites",
        "Keep Favorites",
        "Scan favorites and delete non-favorites",
        MainMenuViewDidActivate
    );

    PaperLogger.info("KeepFavorites registered direct main menu view");
}

bool IsDeleteInFlight() {
    return deleteInFlight;
}

void NotifyLeaveBlocked() {
    if (!deleteInFlight) {
        return;
    }

    SetStatusLines(
        statusLine1,
        "Delete is still running. Stay on this screen until it finishes."
    );
}

}
