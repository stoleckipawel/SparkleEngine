if(NOT DEFINED THREADING_READINESS_SOURCE_DIR)
    set(THREADING_READINESS_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

get_filename_component(
    THREADING_READINESS_SOURCE_DIR
    "${THREADING_READINESS_SOURCE_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
)

cmake_path(NORMAL_PATH THREADING_READINESS_SOURCE_DIR)

set_property(GLOBAL PROPERTY SPARKLE_THREADING_READINESS_VIOLATIONS "")

function(append_threading_violation message_text)
    set_property(GLOBAL APPEND PROPERTY SPARKLE_THREADING_READINESS_VIOLATIONS "${message_text}")
endfunction()

function(read_required_threading_file file_path out_text)
    if(NOT EXISTS "${file_path}")
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${THREADING_READINESS_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_threading_violation("${relative_path}: required threading-readiness file is missing")
        set(${out_text} "" PARENT_SCOPE)
        return()
    endif()

    file(READ "${file_path}" file_text)
    set(${out_text} "${file_text}" PARENT_SCOPE)
endfunction()

function(require_threading_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${THREADING_READINESS_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_threading_violation("${relative_path}: missing '${token}': ${description}")
    endif()
endfunction()

function(forbid_threading_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(NOT match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${THREADING_READINESS_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_threading_violation("${relative_path}: found forbidden '${token}': ${description}")
    endif()
endfunction()

function(forbid_threading_token_in_tree tree_path token description)
    if(NOT EXISTS "${tree_path}")
        return()
    endif()

    file(GLOB_RECURSE tree_files
        "${tree_path}/*.h"
        "${tree_path}/*.hpp"
        "${tree_path}/*.cpp"
        "${tree_path}/*.cxx"
    )

    foreach(tree_file IN LISTS tree_files)
        file(READ "${tree_file}" tree_text)
        forbid_threading_text("${tree_file}" "${tree_text}" "${token}" "${description}")
    endforeach()
endfunction()

set(game_scene_snapshot_path "${THREADING_READINESS_SOURCE_DIR}/Engine/GameFramework/Public/Scene/GameSceneSnapshot.h")
set(game_scene_header_path "${THREADING_READINESS_SOURCE_DIR}/Engine/GameFramework/Public/Scene/GameScene.h")
set(game_scene_source_path "${THREADING_READINESS_SOURCE_DIR}/Engine/GameFramework/Private/Scene/GameScene.cpp")
set(mesh_snapshot_path "${THREADING_READINESS_SOURCE_DIR}/Engine/GameFramework/Public/Scene/Meshes/MeshSnapshot.h")
set(scene_meshes_source_path "${THREADING_READINESS_SOURCE_DIR}/Engine/GameFramework/Private/Scene/Meshes/SceneMeshes.cpp")
set(renderer_source_path "${THREADING_READINESS_SOURCE_DIR}/Engine/Renderer/Private/Renderer.cpp")
set(scene_state_coordinator_path "${THREADING_READINESS_SOURCE_DIR}/Engine/Renderer/Private/SceneData/Lifecycle/SceneRenderStateCoordinator.cpp")
set(render_scene_snapshot_path "${THREADING_READINESS_SOURCE_DIR}/Engine/Renderer/Private/SceneData/Lifecycle/RenderSceneSnapshot.h")
set(render_scene_snapshot_source_path "${THREADING_READINESS_SOURCE_DIR}/Engine/Renderer/Private/SceneData/Lifecycle/RenderSceneSnapshot.cpp")
set(render_scene_data_builder_path "${THREADING_READINESS_SOURCE_DIR}/Engine/Renderer/Private/SceneData/Builders/RenderSceneDataBuilder.cpp")
set(frame_context_path "${THREADING_READINESS_SOURCE_DIR}/Engine/Renderer/Private/Frame/FrameContext.h")
set(framegraph_plan_path "${THREADING_READINESS_SOURCE_DIR}/Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphPlan.h")
set(command_context_path "${THREADING_READINESS_SOURCE_DIR}/Engine/Renderer/Private/Commands/RenderCommandContext.h")
set(architecture_doc_path "${THREADING_READINESS_SOURCE_DIR}/docs/architecture/rhi-growth-architecture-review.md")

foreach(forbidden_marker IN ITEMS ThreadOwnership EngineThreadOwnership EngineCacheSynchronization kThreadOwnership kCacheSynchronization)
    forbid_threading_token_in_tree(
        "${THREADING_READINESS_SOURCE_DIR}/Engine"
        "${forbidden_marker}"
        "threading readiness must be represented by data-flow and module ownership boundaries, not static marker metadata"
    )
endforeach()

read_required_threading_file("${game_scene_snapshot_path}" game_scene_snapshot_text)
read_required_threading_file("${game_scene_header_path}" game_scene_header_text)
read_required_threading_file("${game_scene_source_path}" game_scene_source_text)
read_required_threading_file("${mesh_snapshot_path}" mesh_snapshot_text)
read_required_threading_file("${scene_meshes_source_path}" scene_meshes_source_text)
read_required_threading_file("${renderer_source_path}" renderer_source_text)
read_required_threading_file("${scene_state_coordinator_path}" scene_state_coordinator_text)
read_required_threading_file("${render_scene_snapshot_path}" render_scene_snapshot_text)
read_required_threading_file("${render_scene_snapshot_source_path}" render_scene_snapshot_source_text)
read_required_threading_file("${render_scene_data_builder_path}" render_scene_data_builder_text)
read_required_threading_file("${frame_context_path}" frame_context_text)
read_required_threading_file("${framegraph_plan_path}" framegraph_plan_text)
read_required_threading_file("${command_context_path}" command_context_text)
read_required_threading_file("${architecture_doc_path}" architecture_doc_text)

if(game_scene_snapshot_text)
    require_threading_text("${game_scene_snapshot_path}" "${game_scene_snapshot_text}" "struct SPARKLE_ENGINE_API GameSceneSnapshot" "GameFramework must own an explicit game-to-render scene handoff payload")
    require_threading_text("${game_scene_snapshot_path}" "${game_scene_snapshot_text}" "CameraSnapshot camera" "view data must cross the boundary as snapshot data")
    require_threading_text("${game_scene_snapshot_path}" "${game_scene_snapshot_text}" "LightingSnapshot lighting" "lighting data must cross the boundary as snapshot data")
    require_threading_text("${game_scene_snapshot_path}" "${game_scene_snapshot_text}" "MaterialSnapshot materials" "material data must cross the boundary as snapshot data")
    require_threading_text("${game_scene_snapshot_path}" "${game_scene_snapshot_text}" "MeshSnapshot meshes" "mesh data must cross the boundary as snapshot data")
endif()

if(game_scene_header_text AND game_scene_source_text)
    require_threading_text("${game_scene_header_path}" "${game_scene_header_text}" "GameSceneSnapshot CaptureSnapshot() const" "GameScene must expose an explicit extraction contract")
    require_threading_text("${game_scene_source_path}" "${game_scene_source_text}" "GameSceneSnapshot GameScene::CaptureSnapshot() const" "GameScene extraction must be implemented by the GameFramework owner")
    require_threading_text("${game_scene_source_path}" "${game_scene_source_text}" "snapshot.camera = m_sceneCamera.CaptureSnapshot()" "GameFramework extraction must own camera snapshot composition")
    require_threading_text("${game_scene_source_path}" "${game_scene_source_text}" "snapshot.meshes = m_meshes.CaptureSnapshot()" "GameFramework extraction must own mesh snapshot composition")
endif()

if(mesh_snapshot_text AND scene_meshes_source_text)
    require_threading_text("${mesh_snapshot_path}" "${mesh_snapshot_text}" "struct SPARKLE_ENGINE_API MeshInstanceSnapshot" "mesh snapshots must carry per-instance render data instead of live component pointers")
    require_threading_text("${mesh_snapshot_path}" "${mesh_snapshot_text}" "DirectX::XMFLOAT4X4 worldMatrix" "mesh transform data must be copied during extraction")
    require_threading_text("${mesh_snapshot_path}" "${mesh_snapshot_text}" "DirectX::XMFLOAT3X4 worldInvTranspose" "normal transform data must be copied during extraction")
    require_threading_text("${mesh_snapshot_path}" "${mesh_snapshot_text}" "MaterialHandle materialHandle" "material identity must be copied during extraction")
    forbid_threading_text("${mesh_snapshot_path}" "${mesh_snapshot_text}" "MeshComponent" "mesh snapshots must not expose mutable GameFramework component objects")
    require_threading_text("${scene_meshes_source_path}" "${scene_meshes_source_text}" "XMStoreFloat4x4(&meshInstance.worldMatrix" "GameFramework mesh extraction must copy world transforms")
    require_threading_text("${scene_meshes_source_path}" "${scene_meshes_source_text}" "XMStoreFloat3x4(&meshInstance.worldInvTranspose" "GameFramework mesh extraction must copy inverse-transpose transforms")
endif()

if(render_scene_snapshot_text AND render_scene_snapshot_source_text)
    require_threading_text("${render_scene_snapshot_path}" "${render_scene_snapshot_text}" "struct RenderSceneSnapshot : GameSceneSnapshot" "Renderer must consume a renderer-local copy of the GameFramework snapshot")
    require_threading_text("${render_scene_snapshot_path}" "${render_scene_snapshot_text}" "void Capture(GameSceneSnapshot&& gameSceneSnapshot) noexcept" "Renderer snapshot capture must take the extracted payload")
    require_threading_text("${render_scene_snapshot_source_path}" "${render_scene_snapshot_source_text}" "std::move(gameSceneSnapshot)" "Renderer snapshot capture must move the extracted payload into frame-local renderer storage")
endif()

if(renderer_source_text AND scene_state_coordinator_text)
    require_threading_text("${renderer_source_path}" "${renderer_source_text}" "m_sceneSnapshot->Capture(m_gameScene->CaptureSnapshot())" "Renderer setup must consume the GameFramework extraction contract before frame setup/recording")
    require_threading_text("${scene_state_coordinator_path}" "${scene_state_coordinator_text}" "m_sceneSnapshot->Capture(m_gameScene->CaptureSnapshot())" "Renderer lifecycle refresh must use the same extraction contract")
    forbid_threading_text("${renderer_source_path}" "${renderer_source_text}" "m_sceneSnapshot->Capture(*m_gameScene)" "Renderer must not ask RenderSceneSnapshot to traverse mutable GameScene state")
    forbid_threading_text("${scene_state_coordinator_path}" "${scene_state_coordinator_text}" "m_sceneSnapshot->Capture(*m_gameScene)" "Renderer lifecycle refresh must not ask RenderSceneSnapshot to traverse mutable GameScene state")
endif()

if(render_scene_data_builder_text)
    require_threading_text("${render_scene_data_builder_path}" "${render_scene_data_builder_text}" "sceneSnapshot.meshes.meshInstances" "renderer scene-data build must consume mesh snapshot instances")
    require_threading_text("${render_scene_data_builder_path}" "${render_scene_data_builder_text}" "draw.worldMatrix = meshInstance.worldMatrix" "renderer scene-data build must use captured transform data")
    require_threading_text("${render_scene_data_builder_path}" "${render_scene_data_builder_text}" "draw.worldInvTranspose = meshInstance.worldInvTranspose" "renderer scene-data build must use captured normal transform data")
    forbid_threading_text("${render_scene_data_builder_path}" "${render_scene_data_builder_text}" "MeshComponent" "renderer scene-data build must not consume mutable GameFramework components")
endif()

if(frame_context_text AND framegraph_plan_text)
    require_threading_text("${frame_context_path}" "${frame_context_text}" "RenderSceneData sceneData" "frame setup must package scene data into an explicit frame context")
    require_threading_text("${frame_context_path}" "${frame_context_text}" "RenderViewData mainView" "frame setup must package view data into an explicit frame context")
    require_threading_text("${framegraph_plan_path}" "${framegraph_plan_text}" "struct FrameGraphPlan" "FrameGraph compile output must be an explicit plan object")
    require_threading_text("${framegraph_plan_path}" "${framegraph_plan_text}" "std::vector<FrameGraphPassIndex> executionOrder" "FrameGraph pass order must be stored in compile output rather than hidden global state")
endif()

if(command_context_text)
    require_threading_text("${command_context_path}" "${command_context_text}" "explicit RenderCommandContext(RenderCommandList& commandList) noexcept" "command recording must receive an explicit command list context")
    require_threading_text("${command_context_path}" "${command_context_text}" "RenderCommandList* m_commandList" "command recording state must be explicit on the command context, not hidden globally")
endif()

file(GLOB_RECURSE renderer_pass_files
    "${THREADING_READINESS_SOURCE_DIR}/Engine/Renderer/Private/Passes/*.h"
    "${THREADING_READINESS_SOURCE_DIR}/Engine/Renderer/Private/Passes/*.cpp"
    "${THREADING_READINESS_SOURCE_DIR}/Engine/Renderer/Private/FrameGraph/*.h"
    "${THREADING_READINESS_SOURCE_DIR}/Engine/Renderer/Private/FrameGraph/*.cpp"
)

foreach(renderer_pass_file IN LISTS renderer_pass_files)
    file(READ "${renderer_pass_file}" renderer_pass_text)
    forbid_threading_text("${renderer_pass_file}" "${renderer_pass_text}" "Renderer/Public/Renderer.h" "pass setup must not include Renderer as a service locator")
    forbid_threading_text("${renderer_pass_file}" "${renderer_pass_text}" "Renderer&" "pass setup must not accept Renderer as a service locator")
    forbid_threading_text("${renderer_pass_file}" "${renderer_pass_text}" "Renderer*" "pass setup must not store Renderer as a service locator")
endforeach()

if(architecture_doc_text)
    require_threading_text("${architecture_doc_path}" "${architecture_doc_text}" "Phase 9 Implementation Notes" "Phase 9 documentation must describe the implemented threading-readiness boundary")
    require_threading_text("${architecture_doc_path}" "${architecture_doc_text}" "GameSceneSnapshot" "documentation must name the explicit GameFramework-to-Renderer extraction contract")
    require_threading_text("${architecture_doc_path}" "${architecture_doc_text}" "MeshInstanceSnapshot" "documentation must state that mesh instance data is copied out of mutable components")
    require_threading_text("${architecture_doc_path}" "${architecture_doc_text}" "Renderer caches remain render-thread-only" "documentation must define cache synchronization policy without static marker metadata")
    require_threading_text("${architecture_doc_path}" "${architecture_doc_text}" "stale-handle and generation-check" "documentation must identify stale-handle/generation-check work before worker-thread command recording")
endif()

get_property(threading_violations GLOBAL PROPERTY SPARKLE_THREADING_READINESS_VIOLATIONS)
if(threading_violations)
    list(JOIN threading_violations "\n" threading_violations)
    string(PREPEND threading_violations
        "Threading readiness validation failed. GameFramework-to-Renderer extraction, frame-local data, Renderer caches, FrameGraph plans, and RHI backend services must be separated by real data-flow and ownership boundaries rather than static marker metadata.\n")
    message(FATAL_ERROR "${threading_violations}")
endif()

message(STATUS "Threading readiness check passed for extraction contracts, snapshot payloads, Renderer setup boundaries, command context state, pass setup dependencies, and backend service boundaries.")