cmake_minimum_required(VERSION 3.24)
project(AetherLauncher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(AETHER_BUILD_TESTS "Build automated unit tests" ON)
set(AETHER_VERSION "${PROJECT_VERSION}" CACHE STRING "Semantic version baked into AetherLauncher")

if(APPLE AND EXISTS "/opt/homebrew")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    QuickEffects
    Multimedia
    Sql
    Network
)

qt_standard_project_setup(REQUIRES 6.5)
if(COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()

find_package(SDL3 QUIET)
if(NOT SDL3_FOUND)
    include(FetchContent)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.8
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL3)
endif()

set(AETHER_CORE_SOURCES
    src/core/Models.h
    src/core/Models.cpp
    src/core/AppPaths.h
    src/core/AppPaths.cpp
    src/core/Database.h
    src/core/Database.cpp
    src/core/SettingsManager.h
    src/core/SettingsManager.cpp
    src/emulators/EmulatorManager.h
    src/emulators/EmulatorManager.cpp
    src/emulators/GameLauncher.h
    src/emulators/GameLauncher.cpp
    src/library/MetadataService.h
    src/library/MetadataService.cpp
    src/library/LibraryManager.h
    src/library/LibraryManager.cpp
    src/input/ControllerManager.h
    src/input/ControllerManager.cpp
    src/audio/AudioManager.h
    src/audio/AudioManager.cpp
    src/system/SystemIntegration.h
    src/system/SystemIntegration.cpp
    src/updater/UpdateService.h
    src/updater/UpdateService.cpp
)

if(WIN32)
    list(APPEND AETHER_CORE_SOURCES src/system/SystemIntegration_win.cpp)
elseif(APPLE)
    list(APPEND AETHER_CORE_SOURCES src/system/SystemIntegration_mac.cpp)
endif()

add_library(aether_core STATIC ${AETHER_CORE_SOURCES})
target_compile_definitions(aether_core PUBLIC AETHER_APP_VERSION="${AETHER_VERSION}")
target_include_directories(aether_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(aether_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Sql
    Qt6::Network
)

if(TARGET SDL3::SDL3)
    target_link_libraries(aether_core PUBLIC SDL3::SDL3)
elseif(TARGET SDL3::SDL3-static)
    target_link_libraries(aether_core PUBLIC SDL3::SDL3-static)
endif()

if(WIN32)
    target_link_libraries(aether_core PUBLIC
        dwmapi
        ole32
        pdh
        shell32
        advapi32
        uuid
    )
endif()

# Main Launcher Application
qt_add_executable(AetherLauncher
    src/main.cpp
)

qt_add_qml_module(AetherLauncher
    URI AetherUI
    VERSION 1.0
    QML_FILES
        src/ui/qml/Main.qml
        src/ui/qml/components/AmbientBackground.qml
        src/ui/qml/components/GlassPanel.qml
        src/ui/qml/components/FocusButton.qml
        src/ui/qml/components/GlyphBadge.qml
        src/ui/qml/components/GameCard.qml
        src/ui/qml/widgets/WidgetBar.qml
        src/ui/qml/views/HomeView.qml
        src/ui/qml/views/GameDetailView.qml
        src/ui/qml/views/EmulatorsView.qml
        src/ui/qml/views/LibraryManagerView.qml
        src/ui/qml/views/SettingsView.qml
        src/ui/qml/views/QuickMenuOverlay.qml
        src/ui/qml/views/LaunchOverlay.qml
)

target_link_libraries(AetherLauncher PRIVATE
    aether_core
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickEffects
)

set_target_properties(AetherLauncher PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE FALSE
)

# Standalone Safe Updater Binary (placed beside AetherLauncher)
add_executable(AetherUpdater
    src/updater/updater_main.cpp
)
target_link_libraries(AetherUpdater PRIVATE Qt6::Core)
set_target_properties(AetherUpdater PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# Automated Unit Test Suite
if(AETHER_BUILD_TESTS)
    enable_testing()
    find_package(Catch2 3 QUIET)
    if(NOT Catch2_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.7.1
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(Catch2)
    endif()

    add_executable(aether_tests tests/test_core.cpp)
    target_link_libraries(aether_tests PRIVATE aether_core Catch2::Catch2)
    add_test(NAME AetherCoreTests COMMAND aether_tests)
endif()
