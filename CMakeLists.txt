cmake_minimum_required(VERSION 3.19)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

project(
    vgmstream
    VERSION 1.0.4
    DESCRIPTION "VGMStream plugin for fooyin"
    HOMEPAGE_URL "https://github.com/fooyin/fooyin-plugin-vgmstream"
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

option(BUILD_PLUGIN_BUNDLE "Enable the package_plugin target using bundled vgmstream" OFF)

find_package(Fooyin)
if(NOT BUILD_PLUGIN_BUNDLE)
    find_package(VGMStream QUIET MODULE)
endif()

if(NOT BUILD_PLUGIN_BUNDLE AND VGMStream_FOUND)
    message(STATUS "Using system vgmstream")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/3rdparty/vgmstream/CMakeLists.txt")
    message(STATUS "Using 3rd-party vgmstream")

    set(VGMSTREAM_BUNDLED ON)

    set(BUILD_CLI OFF CACHE BOOL "Build VGMStream CLI" FORCE)
    set(BUILD_V123 OFF CACHE BOOL "Build vgmstream123" FORCE)
    set(BUILD_AUDACIOUS OFF CACHE BOOL "Build Audacious plugin" FORCE)
    set(BUILD_FB2K OFF CACHE BOOL "Build foobar2000 component" FORCE)
    set(BUILD_WINAMP OFF CACHE BOOL "Build Winamp plugin" FORCE)
    set(BUILD_XMPLAY OFF CACHE BOOL "Build XMPlay plugin" FORCE)

    if(WIN32)
        set(USE_MPEG ON CACHE BOOL "Use libmpg123" FORCE)
        set(USE_VORBIS ON CACHE BOOL "Use libvorbis" FORCE)
        set(USE_FFMPEG ON CACHE BOOL "Use FFmpeg" FORCE)
        set(USE_G719 ON CACHE BOOL "Use G.719 decoder" FORCE)
        set(USE_ATRAC9 ON CACHE BOOL "Use ATRAC9 decoder" FORCE)
        set(USE_CELT ON CACHE BOOL "Use CELT decoder" FORCE)
        set(USE_SPEEX ON CACHE BOOL "Use Speex decoder" FORCE)

        set(vgmstream_ffmpeg_libraries)
        foreach(component IN ITEMS avcodec avformat avutil swresample)
            file(GLOB ffmpeg_definitions CONFIGURE_DEPENDS
                 "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/vgmstream/ext_libs/${component}-vgmstream-*.def"
            )
            list(LENGTH ffmpeg_definitions definition_count)
            if(NOT definition_count EQUAL 1)
                message(FATAL_ERROR "Expected one bundled FFmpeg definition file for ${component}")
            endif()
            list(GET ffmpeg_definitions 0 definition_file)
            get_filename_component(library "${definition_file}" NAME_WE)
            list(APPEND vgmstream_ffmpeg_libraries "${library}")
        endforeach()

        if(MSVC)
            if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
                message(FATAL_ERROR "Bundled vgmstream codec DLLs require a 64-bit Windows build")
            endif()

            # vgmstream normally gets this from CMAKE_GENERATOR_PLATFORM, which is empty with Ninja
            set(MSVC_MACHINE_TYPE x64)

            # Generate import libraries whose embedded names match vgmstream's versioned FFmpeg DLLs
            set(VGMSTREAM_FFMPEG_IMPORT_DIR "${CMAKE_CURRENT_BINARY_DIR}/vgmstream-ffmpeg-imports")
            file(MAKE_DIRECTORY "${VGMSTREAM_FFMPEG_IMPORT_DIR}")

            set(FFMPEG_LIBRARIES)
            foreach(library IN LISTS vgmstream_ffmpeg_libraries)
                set(source_def "${CMAKE_SOURCE_DIR}/3rdparty/vgmstream/ext_libs/${library}.def")
                set(generated_def "${VGMSTREAM_FFMPEG_IMPORT_DIR}/${library}.def")
                set(import_library "${VGMSTREAM_FFMPEG_IMPORT_DIR}/${library}.lib")
                file(READ "${source_def}" definitions)
                file(WRITE "${generated_def}" "LIBRARY ${library}.dll\n${definitions}")
                add_custom_command(
                    OUTPUT "${import_library}"
                    COMMAND "${CMAKE_AR}" "/def:${generated_def}" "/machine:${MSVC_MACHINE_TYPE}"
                            "/out:${import_library}"
                    DEPENDS "${generated_def}"
                    VERBATIM
                )
                list(APPEND FFMPEG_LIBRARIES "${import_library}")
            endforeach()
            add_custom_target(vgmstream_ffmpeg_imports DEPENDS ${FFMPEG_LIBRARIES})
            set(FFMPEG_PATH "${CMAKE_SOURCE_DIR}/3rdparty/vgmstream/ext_libs" CACHE PATH "Path to FFmpeg" FORCE)
            set(FFMPEG_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/3rdparty/vgmstream/ext_includes/ffmpeg")
        endif()
    endif()

    add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/vgmstream vgmstream EXCLUDE_FROM_ALL)

    # Some dependency targets are directory-scoped upstream, but setup_target() is called from this directory.
    # Re-expose them here so CMake resolves their archives instead of emitting bare -l names.
    if(NOT WIN32 AND USE_CELT)
        foreach(version IN ITEMS 0061 0110)
            if(TARGET CELT_${version}_MAKE AND NOT TARGET celt${version})
                if(version STREQUAL "0061")
                    set(celt_archive libcelt.a)
                else()
                    set(celt_archive libcelt0.a)
                endif()
                add_library(celt${version} STATIC IMPORTED GLOBAL)
                set_target_properties(
                    celt${version}
                    PROPERTIES
                        IMPORTED_LOCATION
                        "${CMAKE_CURRENT_BINARY_DIR}/vgmstream/dependencies/celt-${version}/libcelt/.libs/${celt_archive}"
                )
                add_dependencies(celt${version} CELT_${version}_MAKE)
            endif()
        endforeach()
    endif()

    target_include_directories(libvgmstream INTERFACE "${CMAKE_SOURCE_DIR}/3rdparty/vgmstream/src")
    add_library(VGMStream::VGMStream ALIAS libvgmstream)
else()
    if(BUILD_PLUGIN_BUNDLE)
        message(
            FATAL_ERROR
                "Plugin bundles require the vgmstream submodule; initialise it with git submodule update --init --recursive"
        )
    endif()
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

if(VGMSTREAM_BUNDLED)
    # vgmstream's static library leaves optional codec linkage to its final consumer
    setup_target(vgmstream TRUE)
    if(TARGET vgmstream_ffmpeg_imports)
        add_dependencies(vgmstream vgmstream_ffmpeg_imports)
    endif()
endif()

if(BUILD_PLUGIN_BUNDLE)
    set(bundle_runtime_files)
    if(WIN32)
        set(codec_arch "${CMAKE_SYSTEM_PROCESSOR}")
        if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
            set(codec_arch "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
        endif()
        string(TOLOWER "${codec_arch}" codec_arch)
        if(NOT codec_arch MATCHES "^(amd64|x86_64|x64)$" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
            message(FATAL_ERROR "Bundled vgmstream codec DLLs require a Windows x64 build")
        endif()
        foreach(
            codec IN
            ITEMS ${vgmstream_ffmpeg_libraries}
                  libcelt-0061
                  libcelt-0110
                  libmpg123-0
                  libspeex-1
                  libvorbis
        )
            list(APPEND bundle_runtime_files
                 "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/vgmstream/ext_libs/dll-x64/${codec}.dll"
            )
        endforeach()
    endif()

    set(vgmstream_license "${CMAKE_CURRENT_BINARY_DIR}/bundle-licenses/vgmstream.txt")
    configure_file(3rdparty/vgmstream/COPYING "${vgmstream_license}" COPYONLY)
    create_fooyin_plugin_bundle(
        vgmstream
        NAME
        fooyin-vgmstream
        RUNTIME_FILES
        ${bundle_runtime_files}
        FILES
        COPYING
        README.md
        LICENSE_FILES
        "${vgmstream_license}"
        LICENSE_DIRECTORIES
        3rdparty/vgmstream/ext_libs/licenses
    )
endif()
