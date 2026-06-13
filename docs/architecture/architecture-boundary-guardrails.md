# Architecture Boundary Guardrails

Status: Stage 3 mechanical guardrail
Date: 2026-06-12
Last synchronized: 2026-06-13

Navigation:

- Before/current architecture: [before/repository-current-state.md](before/repository-current-state.md)
- After/target architecture: [after/repository-target-architecture.md](after/repository-target-architecture.md)
- Target folder architecture: [after/repository-target-folder-architecture.md](after/repository-target-folder-architecture.md)
- Threading readiness: [after/repository-threading-readiness.md](after/repository-threading-readiness.md)
- Target system detail index: [after/system-design-index.md](after/system-design-index.md)

## Purpose

This document records the mechanical boundary checks added before the Renderer/RHI refactor begins. The goal is to make layer direction enforceable instead of relying on memory.

Repository-wide ownership is tracked in [repository-system-map.md](repository-system-map.md), [repository-coverage-status.md](repository-coverage-status.md), and [sparkle-whole-repository-architecture-review.md](../plans/sparkle-whole-repository-architecture-review.md). This check is the rendering/RHI guardrail; Stage 28 extends the same mechanical treatment to runtime-to-tools, GameFramework, launcher, and content-pipeline boundaries.

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
| `RENDERER_NO_BACKEND_NATIVE` | `Engine/Renderer` | Renderer code uses D3D12/Vulkan native headers, native API identifiers, backend-private includes, or direct `Vulkan::Vulkan` linkage outside documented provider paths. |
| `D3D12_NO_VULKAN_BACKEND` | `Engine/RHI/Private/D3D12` | D3D12 backend code includes or uses Vulkan backend/native identifiers. |
| `VULKAN_NO_D3D12_BACKEND` | `Engine/RHI/Private/Vulkan` | Vulkan backend code includes or uses D3D12 backend/native identifiers. |
| `APPLICATION_VALIDATION_NO_BACKEND_NATIVE` | `Engine/Application/Private/Validation` | Application validation grows backend-native capture/API dependencies. |

## Stage 28 Expansion From Disposition Pass

Stage 3 intentionally checks only the first RHI/Renderer boundary. The whole-repository disposition pass adds future checks that should become local/CI-friendly Stage 28 rules.

| Future rule | Scope | Fails when |
| --- | --- | --- |
| `RUNTIME_NO_TOOLS_IMPL` | `Engine/*` | Runtime/editor modules include or link `Tools/*` implementation headers or targets. |
| `GAMEFRAMEWORK_NO_RENDERER_PRIVATE` | `Engine/GameFramework` | GameFramework includes `Engine/Renderer/Private` or renderer pass/pipeline internals. |
| `TOOLS_NO_GAMEFRAMEWORK_PRIVATE` | `Tools/*` | Importers/cookers depend on GameFramework private loaders instead of `AssetContracts`. |
| `SHADERCOMPILER_NO_RENDERER_RUNTIME` | `Tools/Shaders/ShaderCompiler` | ShaderCompiler links full renderer runtime instead of `ShaderContracts`/package primitives. |
| `LAUNCHER_GUI_NO_TOOL_ALGORITHMS` | Launcher Qt GUI folders | Widgets/models include focused cooker/compiler implementation or own build/cook/shader algorithms. |
| `NO_PARALLEL_ASSET_CONVERTER_PIPELINE` | `Tools/Conversion/AssetConverter`, `Tools/Cooking` | AssetConverter returns as a production cook path instead of explicit AssetCooker inspect/debug commands or focused cooker dispatch. |
| `NO_VAGUE_COMMON_POLICY` | Durable source/CMake targets | New permanent `Common`, `Utils`, `Helper`, `Bridge`, or `Manager` owner names appear without a documented contract and stage disposition. |
| `NO_AMBIGUOUS_ASSET_ROOT` | `Engine/Assets`, `Engine/*/Shaders`, `Projects/*/Data`, `Projects/*/Shaders`, and future content roots | Asset/source roots mix built-ins, renderer shaders, RHI fixtures, project content, or generated output without a concrete owner and validation policy. |
| `NO_UNOWNED_SOURCE_ROOT` | Durable `Engine`, `Tools`, `Projects`, `CMake`, `.github`, and `docs` roots | A durable root exists without owner, allowed dependencies, forbidden dependencies, and validation command. |
| `NO_UNEARNED_COMPATIBILITY_LAYER` | Engine/tools/CMake/docs | A compatibility adapter, duplicate registry, or old/new parallel body exists without owner, reason, validation value, and removal stage. |
| `NO_ABSTRACTION_WITHOUT_CALLER_EVIDENCE` | New public APIs, targets, schemas, and helper libraries | A new abstraction is added for hypothetical future use without current callers, contract ownership, and diagnostics/validation benefit. |
| `NO_THREADING_HOSTILE_HANDOFF` | Cross-module contracts and new mutable services | A new edge requires workers/future jobs to access private mutable owner state instead of snapshots, DTOs, manifests, command batches, queue packets, requests, or reports. |
| `NO_THREADING_BY_GLOBAL_LOCK` | Runtime, renderer, RHI, tools, launcher | A design claims future thread safety by adding broad locks around shared mutable state without naming phase ownership and handoff shape. |

## Counted Provider Exceptions

These exceptions are count-limited and owner-labeled. Renderer/provider exceptions are also line-pattern limited, so unrelated native API usage in the same file still fails. If the count grows, the check fails. If a future provider wrapper removes the need for the exception, tighten it in [ArchitectureBoundaryCheck.cmake](../../CMake/ArchitectureBoundaryCheck.cmake).

| Exception | Frozen count | Owner | Reason |
| --- | ---: | --- | --- |
| [Engine/Renderer/CMakeLists.txt](../../Engine/Renderer/CMakeLists.txt) links `Vulkan::Vulkan` only from `SparkleRendererNvidiaDlssProvider`. | 2 | [upscaler-provider-contract.md](upscaler-provider-contract.md) | Streamline Vulkan integration requires SDK-visible Vulkan types; common `SparkleRenderer` no longer links Vulkan directly. |
| [StreamlineDlssRuntime.cpp](../../Engine/Renderer/Private/Upscaling/NvidiaDlss/StreamlineDlssRuntime.cpp) uses Vulkan native identifiers for DLSS integration. | 5 | [upscaler-provider-contract.md](upscaler-provider-contract.md) | Provider integration is narrowed to the NVIDIA DLSS runtime file and RHI-owned native metadata. |

## Acceptance Snapshot

Latest Stage 36 run in this environment:

```text
cmake --build build-vs2026 --config DevelopmentEditor --target architecture_boundary_check -- /nologo /v:minimal /m:1
```

Result:

- No new architecture boundary violations.
- Counted provider exceptions remain limited to the NVIDIA DLSS provider `Vulkan::Vulkan` link and Streamline DLSS Vulkan bridge.
- Stage 36 reran after completed Stage 19 backend service slimming and accepted the current architecture track.

Earlier direct script entry point:

```text
cmake -DSPARKLE_REPO_ROOT="$PWD" -P CMake/ArchitectureBoundaryCheck.cmake
```

Earlier result:

- No new architecture boundary violations.
- Stage 9 narrowed the former Renderer CMake Vulkan exception to the NVIDIA DLSS provider target. The former Application validation D3D12 capture exception was removed in Stage 8.
- Stage 9 validation also passed through the VS2026 build tree with `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target architecture_boundary_check -- /nologo /v:minimal`.
- Stage 5 build validation used a fresh VS2026 build tree: `cmake -S . -B build/windows-vs2026-stage5 -G "Visual Studio 18 2026" -A x64`.
- Stage 9 reconfigured the same VS2026 tree with `cmake -S . -B build/windows-vs2026-stage5 -G "Visual Studio 18 2026" -A x64`.
- Stage 9 built `SparkleApplicationEditor`; the build output includes `SparkleRendererNvidiaDlssProvider.lib` before `SparkleRenderer.lib`.
- `ShaderCompiler` and `SparkleLauncher` built with `DevelopmentEditor`.
- `ShaderCompiler list-shaders --validate` reported 17 valid typed shader registrations.
- `clang_format_check` was not generated because `clang-format` was not found during configure.

## Change Rules

- Do not add broad allowlists.
- Every exception must name the owning contract or removal stage.
- Every exception and compatibility path must explain why its complexity earns temporary right to exist.
- Every new cross-module edge must pass the threading-readiness handoff check from [after/repository-threading-readiness.md](after/repository-threading-readiness.md).
- New ordinary renderer passes must not add `Engine/RHI` dependencies.
- New backend-native validation work belongs behind RHI/backend-owned services, not Application.
- When a later stage fixes a debt row, remove the exception in the same stage.
