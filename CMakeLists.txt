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

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.hpp
        mainwindow.ui
        RangeSlider.cpp
        RangeSlider.h
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
    )

target_compile_definitions(video_cutter PRIVATE $<$<NOT:$<CONFIG:Debug>>:QT_NO_DEBUG_OUTPUT$<SEMICOLON>QT_NO_DEBUG>)

set_target_properties(video_cutter PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(video_cutter)
endif()
