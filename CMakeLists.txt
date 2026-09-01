cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

project(
    vgmstream
    VERSION 1.0.2
    DESCRIPTION "VGMStream plugin for fooyin"
    HOMEPAGE_URL "https://github.com/fooyin/fooyin-plugin-vgmstream"
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Fooyin)
find_package(VGMStream QUIET MODULE)

if(VGMStream_FOUND)
    message(STATUS "Using system vgmstream")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/3rdparty/vgmstream/CMakeLists.txt")
    message(STATUS "Using 3rd-party vgmstream")

    set(BUILD_CLI OFF CACHE BOOL "Build VGMStream CLI" FORCE)
    set(BUILD_V123 OFF CACHE BOOL "Build vgmstream123" FORCE)
    set(BUILD_AUDACIOUS OFF CACHE BOOL "Build Audacious plugin" FORCE)
    set(BUILD_FB2K OFF CACHE BOOL "Build foobar2000 component" FORCE)
    set(BUILD_WINAMP OFF CACHE BOOL "Build Winamp plugin" FORCE)
    set(BUILD_XMPLAY OFF CACHE BOOL "Build XMPlay plugin" FORCE)

    if(WIN32)
        # Bundled vgmstream's optional codec DLLs are 32-bit and cannot link into fooyin's x64 plugin
        set(USE_MPEG OFF CACHE BOOL "Use libmpg123" FORCE)
        set(USE_VORBIS OFF CACHE BOOL "Use libvorbis" FORCE)
        set(USE_FFMPEG OFF CACHE BOOL "Use FFmpeg" FORCE)
        set(USE_G719 OFF CACHE BOOL "Use G.719 decoder" FORCE)
        set(USE_ATRAC9 OFF CACHE BOOL "Use ATRAC9 decoder" FORCE)
        set(USE_CELT OFF CACHE BOOL "Use CELT decoder" FORCE)
        set(USE_SPEEX OFF CACHE BOOL "Use Speex decoder" FORCE)
    endif()

    add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/vgmstream vgmstream EXCLUDE_FROM_ALL)
    target_include_directories(
        libvgmstream
        INTERFACE "${CMAKE_SOURCE_DIR}/3rdparty/vgmstream/src"
    )
    add_library(VGMStream::VGMStream ALIAS libvgmstream)
else()
    message(STATUS "vgmstream not found; skipping vgmstream plugin")
    return()
endif()

create_fooyin_plugin(
    vgmstream
    JSON_IN src/vgmstream.json.in
    DEPENDS Fooyin::Core
            Fooyin::Gui
            VGMStream::VGMStream
    SOURCES src/vgmstreamdefs.h
            src/vgmstreaminput.cpp
            src/vgmstreaminput.h
            src/vgmstreamplugin.cpp
            src/vgmstreamplugin.h
            src/vgmstreamsettings.cpp
            src/vgmstreamsettings.h
)
