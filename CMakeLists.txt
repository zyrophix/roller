cmake_minimum_required(VERSION 3.20)
project(roller VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

include(GNUInstallDirs)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick Gui Core Concurrent)
qt_standard_project_setup(REQUIRES 6.8)

# layer-shell-qt is not optional in practice: Main.qml imports
# org.kde.layershell unconditionally and there is no fallback window type, so a
# binary built without it can answer --version and then exits 1 on a real run.
# Ubuntu only packages it on 22.04, which is why CI opts out explicitly instead
# of the dependency being silently absent.
option(ROLLER_ALLOW_NO_LAYER_SHELL "Build a binary that cannot open a layer surface. CI only." OFF)
find_package(LayerShellQt QUIET)
if(NOT LayerShellQt_FOUND AND NOT ROLLER_ALLOW_NO_LAYER_SHELL)
    message(FATAL_ERROR
        "layer-shell-qt not found, and roller requires it: it is a "
        "wlr-layer-shell overlay and will not start without it. "
        "On Arch: pacman -S layer-shell-qt")
endif()
if(NOT LayerShellQt_FOUND)
    message(WARNING
        "layer-shell-qt not found. The binary will compile but cannot run. "
        "Expected in CI and nowhere else.")
endif()

qt_add_executable(roller
    src/main.cpp
    src/backend/WallpaperRepository.cpp
    src/backend/WallpaperModel.cpp
    src/backend/WallpaperFilterProxy.cpp
    src/backend/ThumbnailCache.cpp
    src/backend/WallpaperBackend.cpp
    src/backend/PickerController.cpp
    src/backend/AppConfig.cpp
)

qt_add_qml_module(roller
    URI Roller
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Carousel.qml
        qml/SearchBar.qml
)

target_compile_options(roller PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(roller PRIVATE ROLLER_VERSION="${PROJECT_VERSION}")
target_link_libraries(roller PRIVATE Qt6::Quick Qt6::Gui Qt6::Core Qt6::Concurrent)
if(LayerShellQt_FOUND)
    target_link_libraries(roller PRIVATE LayerShellQt::Interface)
    target_compile_definitions(roller PRIVATE HAS_LAYER_SHELL=1)
endif()

install(TARGETS roller
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# --- tests ---
# Pure logic only. Rendering and animation stay hand-verified: the overlay
# cannot be reached by synthetic input and a layer surface needs a compositor.
option(BUILD_TESTING "Build the regression tests" ON)
if(BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
    enable_testing()

    add_executable(roller_tests
        tests/test_backend.cpp
        src/backend/AppConfig.cpp
        src/backend/ThumbnailCache.cpp
        src/backend/WallpaperModel.cpp
        src/backend/WallpaperFilterProxy.cpp
        src/backend/WallpaperRepository.cpp
        src/backend/WallpaperBackend.cpp
    )
    set_target_properties(roller_tests PROPERTIES AUTOMOC ON)
    target_compile_definitions(roller_tests PRIVATE HAS_LAYER_SHELL=0)
    target_link_libraries(roller_tests PRIVATE Qt6::Test Qt6::Core Qt6::Gui Qt6::Quick Qt6::Concurrent)
    target_include_directories(roller_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

    add_test(NAME roller_tests COMMAND roller_tests)
endif()
