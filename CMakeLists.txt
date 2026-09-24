cmake_minimum_required(VERSION 3.20)
project(roller VERSION 1.0.0 LANGUAGES CXX)

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

find_package(LayerShellQt QUIET)

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
target_link_libraries(roller PRIVATE Qt6::Quick Qt6::Gui Qt6::Core Qt6::Concurrent)
if(LayerShellQt_FOUND)
    target_link_libraries(roller PRIVATE LayerShellQt::Interface)
    target_compile_definitions(roller PRIVATE HAS_LAYER_SHELL=1)
endif()

install(TARGETS roller
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
