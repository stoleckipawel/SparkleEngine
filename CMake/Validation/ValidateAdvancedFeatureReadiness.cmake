if(NOT DEFINED ADVANCED_FEATURE_READINESS_SOURCE_DIR)
    set(ADVANCED_FEATURE_READINESS_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

get_filename_component(
    ADVANCED_FEATURE_READINESS_SOURCE_DIR
    "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
)

cmake_path(NORMAL_PATH ADVANCED_FEATURE_READINESS_SOURCE_DIR)

set_property(GLOBAL PROPERTY SPARKLE_ADVANCED_FEATURE_READINESS_VIOLATIONS "")

function(append_advanced_feature_violation message_text)
    set_property(GLOBAL APPEND PROPERTY SPARKLE_ADVANCED_FEATURE_READINESS_VIOLATIONS "${message_text}")
endfunction()

function(read_required_advanced_feature_file file_path out_text)
    if(NOT EXISTS "${file_path}")
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_advanced_feature_violation("${relative_path}: required advanced-feature readiness file is missing")
        set(${out_text} "" PARENT_SCOPE)
        return()
    endif()

    file(READ "${file_path}" file_text)
    set(${out_text} "${file_text}" PARENT_SCOPE)
endfunction()

function(require_advanced_feature_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_advanced_feature_violation("${relative_path}: missing '${token}': ${description}")
    endif()
endfunction()

function(forbid_advanced_feature_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(NOT match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_advanced_feature_violation("${relative_path}: found forbidden '${token}': ${description}")
    endif()
endfunction()

function(forbid_advanced_feature_token_in_tree tree_path token description)
    if(NOT EXISTS "${tree_path}")
        return()
    endif()

    file(GLOB_RECURSE checked_files
        "${tree_path}/*.h"
        "${tree_path}/*.hpp"
        "${tree_path}/*.cpp"
        "${tree_path}/*.cxx"
    )

    foreach(checked_file IN LISTS checked_files)
        file(READ "${checked_file}" checked_text)
        forbid_advanced_feature_text("${checked_file}" "${checked_text}" "${token}" "${description}")
    endforeach()
endfunction()

set(architecture_doc_path "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}/docs/architecture/rhi-growth-architecture-review.md")
set(capabilities_path "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}/Engine/RHI/Public/Core/RhiCapabilities.h")
set(binding_layout_path "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}/Engine/RHI/Public/Pipeline/RhiPipelineStateDesc.h")
set(ray_tracing_desc_path "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}/Engine/RHI/Public/RayTracing/RhiRayTracingDesc.h")
set(shader_runtime_path "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}/Engine/Renderer/Private/Pipeline/RenderPassShaderRuntime.h")
set(framegraph_plan_path "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}/Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphPlan.h")
set(command_context_path "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}/Engine/Renderer/Private/Commands/RenderCommandContext.h")
set(validation_targets_path "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}/CMake/SparkleValidationTargets.cmake")

read_required_advanced_feature_file("${architecture_doc_path}" architecture_doc_text)
read_required_advanced_feature_file("${capabilities_path}" capabilities_text)
read_required_advanced_feature_file("${binding_layout_path}" binding_layout_text)
read_required_advanced_feature_file("${ray_tracing_desc_path}" ray_tracing_desc_text)
read_required_advanced_feature_file("${shader_runtime_path}" shader_runtime_text)
read_required_advanced_feature_file("${framegraph_plan_path}" framegraph_plan_text)
read_required_advanced_feature_file("${command_context_path}" command_context_text)
read_required_advanced_feature_file("${validation_targets_path}" validation_targets_text)

if(architecture_doc_text)
    foreach(required_doc_token IN ITEMS
        "Advanced Feature Proposal Template"
        "Feature category"
        "Capability declaration"
        "Binding model impact"
        "Memory/lifetime impact"
        "FrameGraph impact"
        "Command recording and threading impact"
        "D3D12 parity plan"
        "Vulkan parity plan"
        "Fallback behavior"
        "Validation evidence"
        "Bindless Resources"
        "descriptor indexing"
        "descriptor lifetime"
        "Ray Tracing Expansion"
        "RhiRayTracingCapabilities"
        "D3D12 DXR"
        "Vulkan ray tracing extensions"
        "Async Compute"
        "FrameGraph queue ownership"
        "inter-queue synchronization"
        "command recording ownership"
        "Mesh/Task Shaders"
        "Vendor SDK Integrations"
        "Multi-Adapter"
        "Phase 10 Implementation Notes"
        "Phase 10 source-only validation notes")
        require_advanced_feature_text("${architecture_doc_path}" "${architecture_doc_text}" "${required_doc_token}" "advanced features must have a reusable proposal gate and feature-specific blockers")
    endforeach()
endif()

if(capabilities_text)
    require_advanced_feature_text("${capabilities_path}" "${capabilities_text}" "RhiRayTracingCapabilities RayTracing" "ray tracing expansion must flow through the public capability report")
    require_advanced_feature_text("${capabilities_path}" "${capabilities_text}" "bool SupportsMeshShaders" "mesh shader feature work must have a public capability flag")
    require_advanced_feature_text("${capabilities_path}" "${capabilities_text}" "bool SupportsTaskShaders" "task shader feature work must have a public capability flag")
    require_advanced_feature_text("${capabilities_path}" "${capabilities_text}" "RhiQueueCapabilities Queues" "async compute/copy work must start from queue capability reporting")
    require_advanced_feature_text("${capabilities_path}" "${capabilities_text}" "RhiBindingLimits BindingLimits" "bindless and descriptor-growth proposals must account for binding limits")
endif()

if(binding_layout_text)
    require_advanced_feature_text("${binding_layout_path}" "${binding_layout_text}" "RhiBindlessBindingMetadata" "bindless remains reserved metadata, not a hidden material correctness path")
    require_advanced_feature_text("${binding_layout_path}" "${binding_layout_text}" "BindlessEligible = false" "bindless layout eligibility must default off until an accepted proposal enables it")
    require_advanced_feature_text("${binding_layout_path}" "${binding_layout_text}" "RuntimeSizedArray = false" "runtime-sized descriptor arrays must default off until descriptor indexing is designed")
    require_advanced_feature_text("${binding_layout_path}" "${binding_layout_text}" "ReservedDescriptorCount = 0" "bindless descriptor reservation must default to zero until a lifetime model exists")
endif()

if(ray_tracing_desc_text AND shader_runtime_text)
    require_advanced_feature_text("${ray_tracing_desc_path}" "${ray_tracing_desc_text}" "SupportsRayTracing" "ray tracing feature paths must be capability gated")
    require_advanced_feature_text("${ray_tracing_desc_path}" "${ray_tracing_desc_text}" "SupportsInlineRayQuery" "inline ray query paths must be separately capability gated")
    require_advanced_feature_text("${shader_runtime_path}" "${shader_runtime_text}" "requires acceleration-structure bindings" "renderer shader runtime must reject ray tracing requirements on unsupported backends")
    require_advanced_feature_text("${shader_runtime_path}" "${shader_runtime_text}" "SupportsInlineRayQuery" "renderer shader runtime must reject inline ray query requirements on unsupported backends")
endif()

if(framegraph_plan_text AND command_context_text)
    require_advanced_feature_text("${framegraph_plan_path}" "${framegraph_plan_text}" "FrameGraphPlan" "async compute proposals must extend explicit FrameGraph compile output rather than bypass it")
    require_advanced_feature_text("${framegraph_plan_path}" "${framegraph_plan_text}" "executionOrder" "queue scheduling proposals must preserve reviewable execution order")
    require_advanced_feature_text("${command_context_path}" "${command_context_text}" "RenderCommandList* m_commandList" "parallel command recording proposals must build on explicit command context state")
endif()

if(validation_targets_text)
    require_advanced_feature_text("${validation_targets_path}" "${validation_targets_text}" "advanced_feature_readiness_check" "Phase 10 gate must be wired into the validation target set")
endif()

foreach(forbidden_bindless_token IN ITEMS "BindlessEligible = true" "RuntimeSizedArray = true")
    forbid_advanced_feature_token_in_tree(
        "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}/Engine/Renderer"
        "${forbidden_bindless_token}"
        "Renderer must not enable bindless behavior before an accepted advanced-feature proposal defines descriptor indexing, fallback, and lifetime evidence"
    )
endforeach()

foreach(forbidden_vendor_token IN ITEMS "FidelityFX" "FFX_" "ffx_" "AGS" "NVAPI" "DLSS" "XeSS")
    forbid_advanced_feature_token_in_tree(
        "${ADVANCED_FEATURE_READINESS_SOURCE_DIR}/Engine/Renderer"
        "${forbidden_vendor_token}"
        "vendor SDK integration must not enter Renderer before an accepted advanced-feature proposal defines ownership, fallback, backend parity, and validation evidence"
    )
endforeach()

get_property(advanced_feature_violations GLOBAL PROPERTY SPARKLE_ADVANCED_FEATURE_READINESS_VIOLATIONS)
if(advanced_feature_violations)
    list(JOIN advanced_feature_violations "\n" advanced_feature_violations)
    string(PREPEND advanced_feature_violations
        "Advanced feature readiness validation failed. Bindless, ray tracing expansion, async compute, mesh/task shaders, vendor SDK integrations, and multi-adapter work must pass the proposal template before implementation.\n")
    message(FATAL_ERROR "${advanced_feature_violations}")
endif()

message(STATUS "Advanced feature readiness check passed for proposal template evidence, capability gates, bindless defaults, ray tracing capability checks, queue/FrameGraph blockers, and validation target wiring.")