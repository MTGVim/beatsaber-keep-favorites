#include "main.hpp"

#include "UI/KeepFavoritesMenu.hpp"

#include "bsml/shared/BSML.hpp"
#include "custom-types/shared/register.hpp"
#include "scotland2/shared/modloader.h"

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};

Configuration& getConfig() {
    static Configuration config(modInfo);
    return config;
}

MOD_EXTERN_FUNC void setup(CModInfo* info) noexcept {
    *info = modInfo.to_c();

    getConfig().Load();
    Paper::Logger::RegisterFileContextId(PaperLogger.tag);

    PaperLogger.info("Completed setup for KeepFavorites");
}

MOD_EXTERN_FUNC void late_load() noexcept {
    il2cpp_functions::Init();
    custom_types::Register::AutoRegister();
    BSML::Init();

    PaperLogger.info("Registering KeepFavorites main menu UI...");
    KeepFavorites::UI::InstallMenuEntry();
    PaperLogger.info("Registered KeepFavorites main menu UI");
}
