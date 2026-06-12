# Architecture Boundary Guardrails

Status: Stage 3 mechanical guardrail
Date: 2026-06-12

## Purpose

This document records the mechanical boundary checks added before the Renderer/RHI refactor begins. The goal is to make layer direction enforceable instead of relying on memory.

Reference basis:

- NVIDIA NVRHI keeps a focused graphics abstraction layer: https://github.com/NVIDIA-RTX/NVRHI
- NVIDIA NRI presents a narrow graphics abstraction boundary: https://github.com/NVIDIA-RTX/NRI
- AMD Cauldron keeps common code separate from DX12/VK backend folders: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron
- Diligent Engine presents modular backend abstraction components: https://github.com/DiligentGraphics/DiligentEngine

## Commands

Run without configuring or building the editor:

```powershell
cmake -DSPARKLE_REPO_ROOT="$PWD" -P CMake/ArchitectureBoundaryCheck.cmake
```

Run through CMake after a configured build tree exists:

```powershell
cmake --build build --config DevelopmentEditor --target architecture_boundary_check -- /nologo /v:minimal
```

The direct script path is the lowest-friction local and CI entry point because it only needs CMake.

## Checked Rules

| Rule | Scope | Fails when |
| --- | --- | --- |
| `RHI_NO_RENDERER_PRIVATE` | `Engine/RHI` | RHI code includes `Renderer/Private` headers. |
| `RENDERER_NO_BACKEND_NATIVE` | `Engine/Renderer` | Renderer code uses D3D12/Vulkan native headers, native API identifiers, backend-private includes, or direct `Vulkan::Vulkan` linkage outside documented transitional provider paths. |
| `D3D12_NO_VULKAN_BACKEND` | `Engine/RHI/Private/D3D12` | D3D12 backend code includes or uses Vulkan backend/native identifiers. |
| `VULKAN_NO_D3D12_BACKEND` | `Engine/RHI/Private/Vulkan` | Vulkan backend code includes or uses D3D12 backend/native identifiers. |
| `APPLICATION_VALIDATION_NO_BACKEND_NATIVE` | `Engine/Application/Private/Validation` | Application validation grows backend-native capture/API dependencies. |

## Transitional Exceptions

These exceptions are count-limited and stage-labeled. If the count grows, the check fails. If a stage removes the debt, it must remove or tighten the exception in [ArchitectureBoundaryCheck.cmake](../../CMake/ArchitectureBoundaryCheck.cmake).

| Exception | Frozen count | Removal stage | Reason |
| --- | ---: | --- | --- |
| [DirectLightingShaders.cpp](../../Engine/RHI/Private/Shaders/DirectLightingShaders.cpp) includes Renderer-private `RayTracedShadowUniformData`. | 1 | Stage 4 | Renderer-specific shader registration moves out of RHI. |
| [Engine/Renderer/CMakeLists.txt](../../Engine/Renderer/CMakeLists.txt) links `Vulkan::Vulkan` for Streamline. | 2 | Stage 9 | DLSS/native interop wiring moves behind RHI/backend-owned metadata. |
| [StreamlineDlssRuntime.cpp](../../Engine/Renderer/Private/Upscaling/NvidiaDlss/StreamlineDlssRuntime.cpp) uses Vulkan native identifiers for DLSS integration. | 5 | Stage 9 | Provider integration is reviewed and narrowed to documented native interop. |
| [RhiSmokeEditorValidation.cpp](../../Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp) contains D3D12-native capture logic. | 36 | Stage 8 | Backend-native capture/readback moves behind RHI/backend validation services. |

## Acceptance Snapshot

Last run in this environment:

```text
cmake -DSPARKLE_REPO_ROOT="$PWD" -P CMake/ArchitectureBoundaryCheck.cmake
```

Result:

- No new architecture boundary violations.
- Four transitional exception groups reported.
- Full target build validation was not possible in this shell because Visual Studio and C/C++ compilers were not discoverable.

## Change Rules

- Do not add broad allowlists.
- Every exception must name the owning removal stage.
- New ordinary renderer passes must not add `Engine/RHI` dependencies.
- New backend-native validation work belongs behind RHI/backend-owned services, not Application.
- When a later stage fixes a debt row, remove the exception in the same stage.

