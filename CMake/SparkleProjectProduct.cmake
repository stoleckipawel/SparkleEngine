# Shared configuration for runnable Sparkle project products.

set(SPARKLE_WINDOWS_APPLICATION_MANIFEST
    "${CMAKE_SOURCE_DIR}/Engine/Platform/Private/Window/SparkleApplication.manifest")

function(sparkle_configure_project_product target_name project_name product_role)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Unknown Sparkle project product '${target_name}'")
    endif()

    sparkle_set_product_artifact_directories(
        ${target_name}
        "${SPARKLE_DEV_PROJECTS_ROOT}/${project_name}/${product_role}"
        "projects/${project_name}/${product_role}")

    if(WIN32)
        if(NOT EXISTS "${SPARKLE_WINDOWS_APPLICATION_MANIFEST}")
            message(FATAL_ERROR
                "Sparkle Windows application manifest is missing: '${SPARKLE_WINDOWS_APPLICATION_MANIFEST}'")
        endif()
        target_sources(${target_name} PRIVATE "${SPARKLE_WINDOWS_APPLICATION_MANIFEST}")
    endif()
endfunction()
