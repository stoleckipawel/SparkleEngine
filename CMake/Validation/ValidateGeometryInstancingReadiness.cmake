if(NOT DEFINED GEOMETRY_INSTANCING_SOURCE_DIR)
    set(GEOMETRY_INSTANCING_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

get_filename_component(
    GEOMETRY_INSTANCING_SOURCE_DIR
    "${GEOMETRY_INSTANCING_SOURCE_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
)

cmake_path(NORMAL_PATH GEOMETRY_INSTANCING_SOURCE_DIR)

set_property(GLOBAL PROPERTY SPARKLE_GEOMETRY_INSTANCING_VIOLATIONS "")

function(append_geometry_instancing_violation message_text)
    set_property(GLOBAL APPEND PROPERTY SPARKLE_GEOMETRY_INSTANCING_VIOLATIONS "${message_text}")
endfunction()

function(read_required_geometry_instancing_file file_path out_text)
    if(NOT EXISTS "${file_path}")
        append_geometry_instancing_violation("missing required file: ${file_path}")
        set(${out_text} "" PARENT_SCOPE)
        return()
    endif()

    file(READ "${file_path}" file_text)
    set(${out_text} "${file_text}" PARENT_SCOPE)
endfunction()

function(require_geometry_instancing_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${GEOMETRY_INSTANCING_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_geometry_instancing_violation("${relative_path}: missing '${token}': ${description}")
    endif()
endfunction()

function(forbid_geometry_instancing_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(NOT match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${GEOMETRY_INSTANCING_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_geometry_instancing_violation("${relative_path}: found forbidden '${token}': ${description}")
    endif()
endfunction()

set(source_import_result_path "${GEOMETRY_INSTANCING_SOURCE_DIR}/Tools/SourceImportAdapters/Public/SourceImportResult.h")
read_required_geometry_instancing_file("${source_import_result_path}" source_import_result_text)
if(source_import_result_text)
    require_geometry_instancing_text("${source_import_result_path}" "${source_import_result_text}" "ImportedMeshInstanceGroupKind" "source import must preserve importer-neutral instance group kinds")
    require_geometry_instancing_text("${source_import_result_path}" "${source_import_result_text}" "std::vector<ImportedMeshInstanceGroup> meshInstanceGroups" "source import must carry imported instance groups")
endif()

set(source_import_diagnostics_path "${GEOMETRY_INSTANCING_SOURCE_DIR}/Tools/SourceImportAdapters/Public/SourceImportDiagnostics.h")
read_required_geometry_instancing_file("${source_import_diagnostics_path}" source_import_diagnostics_text)
if(source_import_diagnostics_text)
    require_geometry_instancing_text("${source_import_diagnostics_path}" "${source_import_diagnostics_text}" "SourceGeometryInstancingDiagnostics" "import diagnostics must expose geometry-instancing counters")
    require_geometry_instancing_text("${source_import_diagnostics_path}" "${source_import_diagnostics_text}" "authoredInstanceGroupCount" "import diagnostics must distinguish authored instance groups")
endif()

set(cooked_scene_manifest_path "${GEOMETRY_INSTANCING_SOURCE_DIR}/Engine/GameFramework/Public/Assets/Cooked/CookedSceneManifest.h")
read_required_geometry_instancing_file("${cooked_scene_manifest_path}" cooked_scene_manifest_text)
if(cooked_scene_manifest_text)
    require_geometry_instancing_text("${cooked_scene_manifest_path}" "${cooked_scene_manifest_text}" "CookedSceneInstanceGroupRecord" "cooked scene manifests must preserve instance groups")
    require_geometry_instancing_text("${cooked_scene_manifest_path}" "${cooked_scene_manifest_text}" "kCookedSceneManifestVersion" "manifest versioning must stay explicit for recook-required failures")
endif()

set(scene_manifest_loader_path "${GEOMETRY_INSTANCING_SOURCE_DIR}/Engine/GameFramework/Private/Assets/Loaders/SceneManifestLoader.cpp")
read_required_geometry_instancing_file("${scene_manifest_loader_path}" scene_manifest_loader_text)
if(scene_manifest_loader_text)
    require_geometry_instancing_text("${scene_manifest_loader_path}" "${scene_manifest_loader_text}" "Recook the scene asset" "old or incompatible cooked scene manifests must fail with a recook message")
    require_geometry_instancing_text("${scene_manifest_loader_path}" "${scene_manifest_loader_text}" "CookedSceneInstanceGroupKind::AuthoredInstanceGroup" "manifest loading must validate supported instance group kinds")
endif()

set(mesh_snapshot_path "${GEOMETRY_INSTANCING_SOURCE_DIR}/Engine/GameFramework/Public/Scene/Meshes/MeshSnapshot.h")
read_required_geometry_instancing_file("${mesh_snapshot_path}" mesh_snapshot_text)
if(mesh_snapshot_text)
    require_geometry_instancing_text("${mesh_snapshot_path}" "${mesh_snapshot_text}" "MeshInstanceGroupSnapshot" "runtime snapshots must expose instance group records to Renderer")
    require_geometry_instancing_text("${mesh_snapshot_path}" "${mesh_snapshot_text}" "std::vector<MeshInstanceGroupSnapshot> meshInstanceGroups" "runtime snapshots must carry mesh instance groups")
endif()

set(render_scene_data_path "${GEOMETRY_INSTANCING_SOURCE_DIR}/Engine/Renderer/Private/SceneData/RenderSceneData.h")
read_required_geometry_instancing_file("${render_scene_data_path}" render_scene_data_text)
if(render_scene_data_text)
    require_geometry_instancing_text("${render_scene_data_path}" "${render_scene_data_text}" "std::vector<MeshDraw> meshInstances" "renderer scene data must carry shader instance records")
    require_geometry_instancing_text("${render_scene_data_path}" "${render_scene_data_text}" "std::vector<MeshInstanceBatch> meshInstanceBatches" "renderer scene data must carry instanced draw batches")
    forbid_geometry_instancing_text("${render_scene_data_path}" "${render_scene_data_text}" "meshDraws" "legacy per-object draw lists must not return to RenderSceneData")
endif()

set(mesh_batch_builder_path "${GEOMETRY_INSTANCING_SOURCE_DIR}/Engine/Renderer/Private/SceneData/Builders/MeshInstanceBatchBuilder.cpp")
read_required_geometry_instancing_file("${mesh_batch_builder_path}" mesh_batch_builder_text)
if(mesh_batch_builder_text)
    require_geometry_instancing_text("${mesh_batch_builder_path}" "${mesh_batch_builder_text}" "CanShareBatch" "renderer batch-key construction must stay explicit")
    require_geometry_instancing_text("${mesh_batch_builder_path}" "${mesh_batch_builder_text}" "MeshInstanceBatchSource::SingleInstance" "single-instance draws must still flow through the batch path")
    require_geometry_instancing_text("${mesh_batch_builder_path}" "${mesh_batch_builder_text}" "RejectedInvalidInstanceGroupCount" "invalid group references must be counted by reason")
endif()

set(gbuffer_pass_path "${GEOMETRY_INSTANCING_SOURCE_DIR}/Engine/Renderer/Private/Passes/GBufferPass.cpp")
read_required_geometry_instancing_file("${gbuffer_pass_path}" gbuffer_pass_text)
if(gbuffer_pass_text)
    require_geometry_instancing_text("${gbuffer_pass_path}" "${gbuffer_pass_text}" "frame.meshInstances.IsValid()" "GBuffer must fail closed when the frame instance buffer is unavailable")
    require_geometry_instancing_text("${gbuffer_pass_path}" "${gbuffer_pass_text}" "DrawIndexedInstanced" "GBuffer must submit instanced draws")
    require_geometry_instancing_text("${gbuffer_pass_path}" "${gbuffer_pass_text}" "references instance range" "GBuffer must validate batch instance ranges before drawing")
endif()

set(gbuffer_shader_metadata_path "${GEOMETRY_INSTANCING_SOURCE_DIR}/Engine/RHI/Private/Shaders/GBufferShaders.cpp")
read_required_geometry_instancing_file("${gbuffer_shader_metadata_path}" gbuffer_shader_metadata_text)
if(gbuffer_shader_metadata_text)
    require_geometry_instancing_text("${gbuffer_shader_metadata_path}" "${gbuffer_shader_metadata_text}" "SHADER_PARAMETER_RDG_BUFFER_SRV(MeshInstanceData, MeshInstances)" "GBuffer shader metadata must reflect the instance structured buffer")
    require_geometry_instancing_text("${gbuffer_shader_metadata_path}" "${gbuffer_shader_metadata_text}" "MeshInstanceDrawConstantBufferData" "GBuffer shader metadata must reflect first-instance draw constants")
endif()

set(gbuffer_vs_path "${GEOMETRY_INSTANCING_SOURCE_DIR}/Engine/Assets/Shaders/Passes/Deferred/GBufferVS.hlsl")
read_required_geometry_instancing_file("${gbuffer_vs_path}" gbuffer_vs_text)
if(gbuffer_vs_text)
    require_geometry_instancing_text("${gbuffer_vs_path}" "${gbuffer_vs_text}" "MeshInstances[FirstInstance + Input.InstanceId]" "GBuffer vertex shader must use first-instance offset plus SV_InstanceID")
endif()

set(vertex_input_path "${GEOMETRY_INSTANCING_SOURCE_DIR}/Engine/Assets/Shaders/Geometry/VertexInput.hlsli")
read_required_geometry_instancing_file("${vertex_input_path}" vertex_input_text)
if(vertex_input_text)
    require_geometry_instancing_text("${vertex_input_path}" "${vertex_input_text}" "SV_InstanceID" "GBuffer vertex input must expose the draw instance id without changing mesh vertex layouts")
endif()

set(mesh_diagnostics_path "${GEOMETRY_INSTANCING_SOURCE_DIR}/Engine/Renderer/Public/Meshes/MeshDiagnostics.h")
read_required_geometry_instancing_file("${mesh_diagnostics_path}" mesh_diagnostics_text)
if(mesh_diagnostics_text)
    require_geometry_instancing_text("${mesh_diagnostics_path}" "${mesh_diagnostics_text}" "RuntimeAuthoredGroupCount" "editor diagnostics must distinguish authored runtime groups")
    require_geometry_instancing_text("${mesh_diagnostics_path}" "${mesh_diagnostics_text}" "RejectedInvalidInstanceGroupCount" "editor diagnostics must expose rejected candidates by invalid group reason")
    require_geometry_instancing_text("${mesh_diagnostics_path}" "${mesh_diagnostics_text}" "MaxInstancesPerBatch" "editor diagnostics must expose instances-per-batch density")
endif()

get_property(GEOMETRY_INSTANCING_VIOLATIONS GLOBAL PROPERTY SPARKLE_GEOMETRY_INSTANCING_VIOLATIONS)
if(GEOMETRY_INSTANCING_VIOLATIONS)
    list(JOIN GEOMETRY_INSTANCING_VIOLATIONS "\n" GEOMETRY_INSTANCING_VIOLATIONS)
    string(PREPEND GEOMETRY_INSTANCING_VIOLATIONS
        "Geometry instancing readiness validation failed. Import, cook, runtime, renderer batching, GBuffer shader reflection, and editor diagnostics must stay wired as one backend-neutral data path.\n")
    message(FATAL_ERROR "${GEOMETRY_INSTANCING_VIOLATIONS}")
endif()

message(STATUS "Geometry instancing readiness check passed for source import, cooked manifest, runtime snapshot, renderer batches, GBuffer shader binding, and editor diagnostics.")