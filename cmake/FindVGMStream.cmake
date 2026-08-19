# Try to find vgmstream
#
# Once done this will define:

# VGMStream_FOUND
# VGMStream_INCLUDE_DIRS
# VGMStream_LIBRARIES

include(FindPackageHandleStandardArgs)

find_path(VGMStream_INCLUDE_DIR NAMES vgmstream.h PATH_SUFFIXES vgmstream)
find_library(VGMStream_LIBRARY NAMES vgmstream)

find_package_handle_standard_args(VGMStream REQUIRED_VARS VGMStream_INCLUDE_DIR VGMStream_LIBRARY)

if(VGMStream_FOUND)
    set(VGMStream_INCLUDE_DIRS "${VGMStream_INCLUDE_DIR}")
    set(VGMStream_LIBRARIES "${VGMStream_LIBRARY}")

    if(NOT TARGET VGMStream::VGMStream)
        add_library(VGMStream::VGMStream UNKNOWN IMPORTED)

        set_target_properties(
            VGMStream::VGMStream
            PROPERTIES IMPORTED_LOCATION "${VGMStream_LIBRARY}"
                       INTERFACE_INCLUDE_DIRECTORIES "${VGMStream_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(VGMStream_INCLUDE_DIR VGMStream_LIBRARY)