cmake_minimum_required(VERSION 3.14)

project(
    vgmstream
    VERSION 1.0.1
    DESCRIPTION "VGMStream plugin for fooyin"
    HOMEPAGE_URL "https://github.com/fooyin/fooyin-plugin-vgmstream"
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Fooyin)
find_package(VGMStream QUIET MODULE)

if(VGMSTREAM_FOUND)
    message(STATUS "Using system vgmstream")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/3rdparty/vgmstream/CMakeLists.txt")
    message(STATUS "Using 3rd-party vgmstream")

    set(BUILD_CLI OFF CACHE BOOL "Build VGMStream CLI" FORCE)
    set(BUILD_V123 OFF CACHE BOOL "Build vgmstream123" FORCE)
    set(BUILD_AUDACIOUS OFF CACHE BOOL "Build Audacious plugin" FORCE)

    add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/vgmstream vgmstream EXCLUDE_FROM_ALL)
    target_include_directories(
        libvgmstream
        INTERFACE "${CMAKE_SOURCE_DIR}/3rdparty/vgmstream/src"
    )
    add_library(VGMStream::VGMStream ALIAS libvgmstream)
    set(VGMSTREAM_TARGET VGMStream::VGMStream)
else()
    message(STATUS "vgmstream not found; skipping vgmstream plugin")
    return()
endif()

create_fooyin_plugin(
    vgmstream
    JSON_IN src/vgmstream.json.in
    DEPENDS Fooyin::Core
            Fooyin::Gui
            ${VGMSTREAM_TARGET}
    SOURCES src/vgmstreamdefs.h
            src/vgmstreaminput.cpp
            src/vgmstreaminput.h
            src/vgmstreamplugin.cpp
            src/vgmstreamplugin.h
            src/vgmstreamsettings.cpp
            src/vgmstreamsettings.h
)
