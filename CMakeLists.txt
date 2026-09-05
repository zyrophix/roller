cmake_minimum_required(VERSION 3.20)
project(hyprroll VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_BUILD_TYPE Release CACHE STRING "")

include(GNUInstallDirs)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick Gui Core)
qt_standard_project_setup(REQUIRES 6.8)

find_package(LayerShellQt QUIET)

qt_add_executable(hyprroll
    src/main.cpp
    src/backend/Color.cpp
    src/backend/Repository.cpp
    src/backend/Metadata.cpp
    src/backend/Model.cpp
    src/backend/Awww.cpp
    src/backend/Backend.cpp
    src/backend/Config.cpp
)

qt_add_qml_module(hyprroll
    URI Hyprroll
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Carousel.qml
        qml/ColorFilter.qml
)

target_compile_options(hyprroll PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(hyprroll PRIVATE Qt6::Quick Qt6::Gui Qt6::Core)
if(LayerShellQt_FOUND)
    target_link_libraries(hyprroll PRIVATE LayerShellQt::Interface)
    target_compile_definitions(hyprroll PRIVATE HAS_LAYER_SHELL=1)
endif()

install(TARGETS hyprroll
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
