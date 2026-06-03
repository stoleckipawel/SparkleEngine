# Sparkle Qt kit discovery.
#
# Prefer explicit user configuration, but make fresh Windows source builds work
# when Qt is installed in the standard Qt Online Installer layout.

set(SPARKLE_QT_ROOT "" CACHE PATH "Qt kit root used by Sparkle, for example a Qt MSVC x64 kit root.")

function(sparkle_path_has_qt6_config candidate_path out_has_config)
    if(EXISTS "${candidate_path}/lib/cmake/Qt6/Qt6Config.cmake")
        set(${out_has_config} TRUE PARENT_SCOPE)
    else()
        set(${out_has_config} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(sparkle_append_qt_candidate candidate_path)
    if("${candidate_path}" STREQUAL "")
        return()
    endif()

    cmake_path(NORMAL_PATH candidate_path OUTPUT_VARIABLE normalized_candidate)

    get_filename_component(candidate_name "${normalized_candidate}" NAME)
    string(TOLOWER "${candidate_name}" candidate_name_lower)
    if(candidate_name_lower STREQUAL "qt6")
        get_filename_component(normalized_candidate "${normalized_candidate}/../.." ABSOLUTE)
        cmake_path(NORMAL_PATH normalized_candidate OUTPUT_VARIABLE normalized_candidate)
        get_filename_component(candidate_name "${normalized_candidate}" NAME)
        string(TOLOWER "${candidate_name}" candidate_name_lower)
    endif()

    sparkle_path_has_qt6_config("${normalized_candidate}" has_qt_config)
    if(NOT has_qt_config)
        return()
    endif()

    if(WIN32)
        if(NOT candidate_name_lower MATCHES "msvc.*64")
            return()
        endif()
        if(candidate_name_lower MATCHES "arm64")
            return()
        endif()
    endif()

    list(FIND CMAKE_PREFIX_PATH "${normalized_candidate}" existing_index)
    if(existing_index EQUAL -1)
        list(APPEND CMAKE_PREFIX_PATH "${normalized_candidate}")
        set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "CMake package prefix paths." FORCE)
        message(STATUS "Sparkle Qt kit discovered: ${normalized_candidate}")
    endif()
endfunction()

function(sparkle_discover_qt_kit)
    if(DEFINED Qt6_DIR AND NOT "${Qt6_DIR}" STREQUAL "")
        return()
    endif()

    if(DEFINED SPARKLE_QT_ROOT AND NOT "${SPARKLE_QT_ROOT}" STREQUAL "")
        sparkle_append_qt_candidate("${SPARKLE_QT_ROOT}")
    endif()

    if(DEFINED ENV{SPARKLE_QT_ROOT} AND NOT "$ENV{SPARKLE_QT_ROOT}" STREQUAL "")
        sparkle_append_qt_candidate("$ENV{SPARKLE_QT_ROOT}")
    endif()

    if(DEFINED ENV{QTDIR} AND NOT "$ENV{QTDIR}" STREQUAL "")
        sparkle_append_qt_candidate("$ENV{QTDIR}")
    endif()

    if(DEFINED ENV{Qt6_DIR} AND NOT "$ENV{Qt6_DIR}" STREQUAL "")
        sparkle_append_qt_candidate("$ENV{Qt6_DIR}")
    endif()

    if(WIN32 AND IS_DIRECTORY "C:/Qt")
        file(GLOB sparkle_qt_version_roots LIST_DIRECTORIES TRUE "C:/Qt/*")
        list(SORT sparkle_qt_version_roots)
        list(REVERSE sparkle_qt_version_roots)
        foreach(version_root IN LISTS sparkle_qt_version_roots)
            if(IS_DIRECTORY "${version_root}")
                file(GLOB sparkle_qt_kit_roots LIST_DIRECTORIES TRUE "${version_root}/msvc*64")
                list(SORT sparkle_qt_kit_roots)
                list(REVERSE sparkle_qt_kit_roots)
                foreach(kit_root IN LISTS sparkle_qt_kit_roots)
                    sparkle_append_qt_candidate("${kit_root}")
                endforeach()
            endif()
        endforeach()
    endif()
endfunction()

sparkle_discover_qt_kit()
