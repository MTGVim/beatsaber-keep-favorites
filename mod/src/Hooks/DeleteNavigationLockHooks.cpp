#include "Hooks/DeleteNavigationLockHooks.hpp"

#include "UI/KeepFavoritesMenu.hpp"
#include "bsml/shared/BSML/FlowCoordinators/MainMenuHolderFlowCoordinator.hpp"
#include "main.hpp"

namespace KeepFavorites::Hooks {
namespace {

MAKE_HOOK_FIND_CLASS(
    MainMenuHolderFlowCoordinator_BackButtonWasPressed,
    "BSML",
    "MainMenuHolderFlowCoordinator",
    "BackButtonWasPressed",
    void,
    BSML::MainMenuHolderFlowCoordinator* self,
    HMUI::ViewController* topViewController
) {
    if (KeepFavorites::UI::IsDeleteInFlight()) {
        KeepFavorites::UI::NotifyLeaveBlocked();
        PaperLogger.info("KeepFavorites blocked back navigation while batch delete was running");
        return;
    }

    MainMenuHolderFlowCoordinator_BackButtonWasPressed(self, topViewController);
}

}

void InstallDeleteNavigationLockHooks() {
    INSTALL_HOOK(PaperLogger, MainMenuHolderFlowCoordinator_BackButtonWasPressed);
}

}
