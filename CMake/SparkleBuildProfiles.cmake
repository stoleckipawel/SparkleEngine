# Sparkle build profiles
#
# This is the canonical, inspectable list of supported build configurations.
# Each profile combines a build state with a target shape:
#
#   Debug      - no optimization, full debug information, highest diagnostics.
#   Development - optimized build with debug information and developer diagnostics.
#   Shipping   - optimized for runtime performance with shipping diagnostics policy.
#
#   Editor     - builds editor-capable launch targets.
#   Game       - builds editorless runtime launch targets.

set(SPARKLE_BUILD_CONFIGURATIONS
    DevelopmentEditor
    DevelopmentGame
    DebugEditor
    DebugGame
    ShippingEditor
    ShippingGame
)

set(SPARKLE_DEBUG_CONFIGURATIONS DebugEditor DebugGame)
set(SPARKLE_DEVELOPMENT_CONFIGURATIONS DevelopmentEditor DevelopmentGame)
set(SPARKLE_SHIPPING_CONFIGURATIONS ShippingEditor ShippingGame)
set(SPARKLE_EDITOR_CONFIGURATIONS DevelopmentEditor DebugEditor ShippingEditor)
set(SPARKLE_GAME_CONFIGURATIONS DevelopmentGame DebugGame ShippingGame)

set(SPARKLE_DEFAULT_CONFIGURATION DevelopmentEditor)

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "${SPARKLE_BUILD_CONFIGURATIONS}" CACHE STRING "Sparkle build profiles" FORCE)
else()
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE ${SPARKLE_DEFAULT_CONFIGURATION} CACHE STRING "Sparkle build profile" FORCE)
    endif()
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${SPARKLE_BUILD_CONFIGURATIONS})
    if(NOT CMAKE_BUILD_TYPE IN_LIST SPARKLE_BUILD_CONFIGURATIONS)
        message(FATAL_ERROR "Unsupported CMAKE_BUILD_TYPE '${CMAKE_BUILD_TYPE}'. Use one of: ${SPARKLE_BUILD_CONFIGURATIONS}")
    endif()
endif()

function(sparkle_set_profile_flags profile c_flags cxx_flags exe_linker_flags shared_linker_flags module_linker_flags static_linker_flags)
    string(TOUPPER "${profile}" profile_upper)
    set(CMAKE_C_FLAGS_${profile_upper} "${c_flags}" CACHE STRING "${profile} C flags" FORCE)
    set(CMAKE_CXX_FLAGS_${profile_upper} "${cxx_flags}" CACHE STRING "${profile} CXX flags" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_${profile_upper} "${exe_linker_flags}" CACHE STRING "${profile} executable linker flags" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_${profile_upper} "${shared_linker_flags}" CACHE STRING "${profile} shared linker flags" FORCE)
    set(CMAKE_MODULE_LINKER_FLAGS_${profile_upper} "${module_linker_flags}" CACHE STRING "${profile} module linker flags" FORCE)
    set(CMAKE_STATIC_LINKER_FLAGS_${profile_upper} "${static_linker_flags}" CACHE STRING "${profile} static linker flags" FORCE)
endfunction()

function(sparkle_join_profile_options option_list output_variable)
    list(JOIN ${option_list} " " joined_options)
    set(${output_variable} "${joined_options}" PARENT_SCOPE)
endfunction()

set(SPARKLE_MSVC_DEBUG_COMPILE_OPTIONS
    /Od      # Disable optimization for straightforward source-level debugging.
    /Ob0     # Disable inline expansion so stepping follows source structure.
    /Zi      # Emit complete PDB debug information.
    /RTC1    # Enable runtime checks for stack frames and uninitialized locals.
)

set(SPARKLE_MSVC_DEVELOPMENT_COMPILE_OPTIONS
    /O2      # Enable regular speed optimizations for realistic development performance.
    /Ob2     # Let the compiler expand suitable inline functions.
    /Zi      # Keep PDB debug information for profiling and practical debugging.
    /DNDEBUG # Disable standard assert paths while keeping Sparkle developer diagnostics.
)

set(SPARKLE_MSVC_SHIPPING_COMPILE_OPTIONS
    /O2      # Enable regular speed optimizations for final runtime performance.
    /Ob2     # Let the compiler expand suitable inline functions.
    /DNDEBUG # Disable standard assert paths for shipping-style builds.
)

set(SPARKLE_CLANG_DEBUG_COMPILE_OPTIONS
    -O0      # Disable optimization for straightforward source-level debugging.
    -g       # Emit debug information.
)

set(SPARKLE_CLANG_DEVELOPMENT_COMPILE_OPTIONS
    -O2      # Enable regular speed optimizations for realistic development performance.
    -g       # Keep debug information for profiling and practical debugging.
    -DNDEBUG # Disable standard assert paths while keeping Sparkle developer diagnostics.
)

set(SPARKLE_CLANG_SHIPPING_COMPILE_OPTIONS
    -O3      # Prefer maximum optimization for final runtime performance.
    -DNDEBUG # Disable standard assert paths for shipping-style builds.
)

sparkle_join_profile_options(SPARKLE_MSVC_DEBUG_COMPILE_OPTIONS SPARKLE_MSVC_DEBUG_COMPILE_FLAGS)
sparkle_join_profile_options(SPARKLE_MSVC_DEVELOPMENT_COMPILE_OPTIONS SPARKLE_MSVC_DEVELOPMENT_COMPILE_FLAGS)
sparkle_join_profile_options(SPARKLE_MSVC_SHIPPING_COMPILE_OPTIONS SPARKLE_MSVC_SHIPPING_COMPILE_FLAGS)
sparkle_join_profile_options(SPARKLE_CLANG_DEBUG_COMPILE_OPTIONS SPARKLE_CLANG_DEBUG_COMPILE_FLAGS)
sparkle_join_profile_options(SPARKLE_CLANG_DEVELOPMENT_COMPILE_OPTIONS SPARKLE_CLANG_DEVELOPMENT_COMPILE_FLAGS)
sparkle_join_profile_options(SPARKLE_CLANG_SHIPPING_COMPILE_OPTIONS SPARKLE_CLANG_SHIPPING_COMPILE_FLAGS)

foreach(profile IN LISTS SPARKLE_DEBUG_CONFIGURATIONS)
    if(MSVC)
        sparkle_set_profile_flags(${profile} "${SPARKLE_MSVC_DEBUG_COMPILE_FLAGS}" "${SPARKLE_MSVC_DEBUG_COMPILE_FLAGS}" "" "" "" "")
    else()
        sparkle_set_profile_flags(${profile} "${SPARKLE_CLANG_DEBUG_COMPILE_FLAGS}" "${SPARKLE_CLANG_DEBUG_COMPILE_FLAGS}" "" "" "" "")
    endif()
endforeach()

foreach(profile IN LISTS SPARKLE_DEVELOPMENT_CONFIGURATIONS)
    if(MSVC)
        sparkle_set_profile_flags(${profile} "${SPARKLE_MSVC_DEVELOPMENT_COMPILE_FLAGS}" "${SPARKLE_MSVC_DEVELOPMENT_COMPILE_FLAGS}" "" "" "" "")
    else()
        sparkle_set_profile_flags(${profile} "${SPARKLE_CLANG_DEVELOPMENT_COMPILE_FLAGS}" "${SPARKLE_CLANG_DEVELOPMENT_COMPILE_FLAGS}" "" "" "" "")
    endif()
endforeach()

foreach(profile IN LISTS SPARKLE_SHIPPING_CONFIGURATIONS)
    if(MSVC)
        sparkle_set_profile_flags(${profile} "${SPARKLE_MSVC_SHIPPING_COMPILE_FLAGS}" "${SPARKLE_MSVC_SHIPPING_COMPILE_FLAGS}" "" "" "" "")
    else()
        sparkle_set_profile_flags(${profile} "${SPARKLE_CLANG_SHIPPING_COMPILE_FLAGS}" "${SPARKLE_CLANG_SHIPPING_COMPILE_FLAGS}" "" "" "" "")
    endif()
endforeach()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<OR:$<CONFIG:DebugEditor>,$<CONFIG:DebugGame>>:Debug>DLL")

add_compile_definitions(
    "$<$<OR:$<CONFIG:DebugEditor>,$<CONFIG:DebugGame>>:SPARKLE_BUILD_DEBUG=1>"
    "$<$<OR:$<CONFIG:DebugEditor>,$<CONFIG:DebugGame>>:SPARKLE_BUILD_DEVELOPMENT=0>"
    "$<$<OR:$<CONFIG:DebugEditor>,$<CONFIG:DebugGame>>:SPARKLE_BUILD_SHIPPING=0>"
    "$<$<OR:$<CONFIG:DevelopmentEditor>,$<CONFIG:DevelopmentGame>>:SPARKLE_BUILD_DEBUG=0>"
    "$<$<OR:$<CONFIG:DevelopmentEditor>,$<CONFIG:DevelopmentGame>>:SPARKLE_BUILD_DEVELOPMENT=1>"
    "$<$<OR:$<CONFIG:DevelopmentEditor>,$<CONFIG:DevelopmentGame>>:SPARKLE_BUILD_SHIPPING=0>"
    "$<$<OR:$<CONFIG:ShippingEditor>,$<CONFIG:ShippingGame>>:SPARKLE_BUILD_DEBUG=0>"
    "$<$<OR:$<CONFIG:ShippingEditor>,$<CONFIG:ShippingGame>>:SPARKLE_BUILD_DEVELOPMENT=0>"
    "$<$<OR:$<CONFIG:ShippingEditor>,$<CONFIG:ShippingGame>>:SPARKLE_BUILD_SHIPPING=1>"
    "$<$<OR:$<CONFIG:DebugEditor>,$<CONFIG:DevelopmentEditor>,$<CONFIG:ShippingEditor>>:SPARKLE_TARGET_EDITOR=1>"
    "$<$<OR:$<CONFIG:DebugEditor>,$<CONFIG:DevelopmentEditor>,$<CONFIG:ShippingEditor>>:SPARKLE_TARGET_GAME=0>"
    "$<$<OR:$<CONFIG:DebugGame>,$<CONFIG:DevelopmentGame>,$<CONFIG:ShippingGame>>:SPARKLE_TARGET_EDITOR=0>"
    "$<$<OR:$<CONFIG:DebugGame>,$<CONFIG:DevelopmentGame>,$<CONFIG:ShippingGame>>:SPARKLE_TARGET_GAME=1>"
)

list(JOIN SPARKLE_BUILD_CONFIGURATIONS ", " SPARKLE_BUILD_CONFIGURATION_LIST)
message(STATUS "Sparkle build profiles: default=${SPARKLE_DEFAULT_CONFIGURATION}; available=${SPARKLE_BUILD_CONFIGURATION_LIST}")