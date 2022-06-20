cmake_minimum_required(VERSION 3.5)

project(video_cutter VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools MultimediaWidgets Gui)

set(TS_FILES video_cutter_ja_JP.ts)

add_subdirectory(3rdparty)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.hpp
    mainwindow.ui
    processwidget.hpp
    processwidget.cpp
    processwidget.ui
    ${TS_FILES}
)

qt_add_executable(video_cutter
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

find_package(fmt QUIET)

if(NOT ${fmt_FOUND})
    include(cmake/CPM.cmake)
    CPMAddPackage(
        NAME fmt
        GITHUB_REPOSITORY fmtlib/fmt
        GIT_TAG 8.1.1
    )
endif()

target_link_libraries(video_cutter PRIVATE
    Qt6::Widgets
    Qt6::MultimediaWidgets
    Qt6::Gui
    fmt
    RangeSlider
)

target_include_directories(video_cutter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Qt-RangeSlider
)

target_compile_definitions(video_cutter PRIVATE $<$<NOT:$<CONFIG:Debug>>:QT_NO_DEBUG_OUTPUT$<SEMICOLON>QT_NO_DEBUG>)
target_compile_options(video_cutter PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic>
    $<$<CXX_COMPILER_ID:GNU>:-pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=2 -Wswitch-default -Wundef -Wno-unused -Wunsafe-loop-optimizations -Wfloat-equal>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

set_target_properties(video_cutter PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

qt_finalize_executable(video_cutter)