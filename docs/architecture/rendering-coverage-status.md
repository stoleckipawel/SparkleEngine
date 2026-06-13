# Rendering Coverage Status

Status: Stage 1 baseline
Date: 2026-06-12
Last synchronized: 2026-06-13
Source plan: `docs/plans/rhi-renderer-review-ready-implementation-plan.md`
Source audit: `docs/plans/rhi-renderer-architecture-review.md`
Whole-repository status: `docs/architecture/repository-coverage-status.md`

## Purpose

This document is the detailed status map for the Renderer/RHI review-ready refactor. It converts the prose coverage audit into an evidence table so every rendering subsystem has an owner, a risk, a stage, and a final acceptance target. Whole-repository coverage for GameFramework, tools, launcher, CMake, CI, projects, and docs lives in [repository-coverage-status.md](repository-coverage-status.md).

This is an evidence-freeze stage. No runtime code is changed here.

The structure follows the Stage 1 references:

- arc42: document architecture decisions, risks, quality goals, and concepts.
- CMU SEI ATAM: evaluate architecture against quality attributes and tradeoffs.
- ADR practice: keep decisions traceable when a stage intentionally changes direction.

## Status Legend

| Status | Meaning |
| --- | --- |
| `Accepted` | Current ownership appears aligned with the target architecture. Later stages must preserve it and provide final evidence. |
| `Needs refactor` | The row has a known implementation or ownership gap and links to a later implementation stage. |
| `Needs design decision` | The row has an unresolved ownership or policy question that must be decided before code motion. |

Update rule:

- Every `Needs refactor` row must keep a concrete stage link.
- Every `Needs design decision` row must keep an explicit question.
- A row can move to `Accepted` only after its final acceptance evidence exists.
- Any new file under the coverage scope must either match an existing row in this document or add a new row before a later stage can be accepted.

## File-Level Coverage Verification

The coverage map was confronted against the repository on 2026-06-12 instead of relying only on the prose audit, then reconfirmed on 2026-06-13 before accepting Stage 1.

Inventory command:

```powershell
rg --files Engine/Renderer Engine/RHI Engine/Application/Private/Validation Tools/Launcher/SparkleLauncher/Private/Launch/Smoke Tools/Shaders/ShaderCompiler
```

Verification snapshot:

| Scope | Files | Coverage result |
| --- | ---: | --- |
| `Engine/Renderer` | 252 | Mapped to Renderer private/public/module-support rows, including renderer-owned shader registrations. |
| `Engine/RHI` | 198 | Mapped to RHI public/common/D3D12/Vulkan/module-support rows. |
| `Engine/Application/Private/Validation` | 3 | Mapped to supporting validation rows. |
| `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke` | 2 | Mapped to supporting validation rows. |
| `Tools/Shaders/ShaderCompiler` | 111 | Mapped to shader compiler and cooking rows. |
| Total tracked files | 566 | 566 mapped. |
| Unmapped files | 0 | None. |

Shader-source check:

```powershell
rg --files | rg "\.(hlsl|hlsli|slang|shader|vert|frag|comp|rchit|rgen|rmiss)$"
```

Current result: no standalone shader source files were found in the repository snapshot. Shader authoring coverage therefore tracks the C++ shader registration, reflection, compiler, and cooked-package surfaces until standalone shader source files are introduced.

Mapping rules:

| Path rule | Coverage row |
| --- | --- |
| `Engine/Renderer/CMakeLists.txt` | Renderer module build contract. |
| `Engine/Renderer/Private/PCH.h` | Renderer private module support. |
| `Engine/Renderer/ShaderRegistrations/**` | Renderer-owned shader registration row under module build/support coverage. |
| `Engine/Renderer/Private/Renderer.cpp` | `Renderer.cpp` root orchestration. |
| `Engine/Renderer/Private/{Camera,Commands,Debug,Diagnostics,Frame,FrameGraph,Meshes,Passes,Pipeline,RayTracing,SceneData,Temporal,Textures,Upscaling}/**` | Matching Renderer Private Coverage row. |
| `Engine/Renderer/Public/Renderer.h`, `Engine/Renderer/Public/RendererAPI.h` | `Renderer.h` / `RendererAPI.h`. |
| `Engine/Renderer/Public/{Debug,Denoising,Diagnostics,FrameGraph,Meshes,Resources,SceneData,ShaderParameters,Shaders,Viewport}/**` | Matching Renderer Public Coverage row. |
| `Engine/RHI/CMakeLists.txt` | RHI module build contract. |
| `Engine/RHI/Private/PCH.h` | RHI private common module support. |
| `Engine/RHI/Public/RHIAPI.h` | RHI public module API. |
| `Engine/RHI/Public/{Bindings,Commands,Config,Core,CVars,Descriptors,Device,Diagnostics,Formats,Interop,Memory,Pipeline,RayTracing,Resources,Samplers,ShaderParameters,Shaders,Textures,UI,Validation}/**` | Matching RHI Public Coverage row. |
| `Engine/RHI/Private/{Bindings,Config,Core,CVars,Device,Shaders,Validation}/**` | Matching RHI Private Common Coverage row. |
| `Engine/RHI/Private/D3D12/{D3D12PCH.h,D3D12RenderHardwareInterface.*,D3D12TypeConversions.*}` | D3D12 root facade and type conversions. |
| `Engine/RHI/Private/D3D12/{Commands,Descriptors,Device,Diagnostics,Memory,Pipeline,RayTracing,Resources,Samplers,SwapChain,ThirdParty,UI}/**` | Matching D3D12 Backend Coverage row. |
| `Engine/RHI/Private/D3D12/Textures/**` | D3D12 `Resources` / `Textures`. |
| `Engine/RHI/Private/Vulkan/{VulkanIncludes.h,VulkanPCH.h,VulkanRenderHardwareInterface.*,VulkanTypeConversions.*}` | Vulkan root facade, includes, and type conversions. |
| `Engine/RHI/Private/Vulkan/{Commands,Core,Descriptors,Device,Diagnostics,Memory,Pipeline,RayTracing,Resources,Samplers,SwapChain,UI}/**` | Matching Vulkan Backend Coverage row. |
| `Engine/RHI/Private/Vulkan/Textures/**` | Vulkan `Resources` / `Textures`. |
| `Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp` | Editor smoke validation surface. |
| `Engine/Application/Private/Validation/RhiSmokeValidation.*` | Runtime smoke validation surface. |
| `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/**` | Launcher smoke workflow surface. |
| `Tools/Shaders/ShaderCompiler/{CMakeLists.txt,Source/main.cpp}` | Shader compiler build and CLI entry. |
| `Tools/Shaders/ShaderCompiler/{Backends,Private}/**` | Matching shader compiler and cooking coverage row. |

Confidence rule: a later implementation stage is not considered ready if this inventory produces any unmapped file.

Explicit scope boundary:

- In scope here: `Engine/Renderer`, `Engine/RHI`, RHI smoke validation under `Engine/Application/Private/Validation`, launcher smoke operations, and `Tools/Shaders/ShaderCompiler`.
- Whole-repository scope: use [repository-coverage-status.md](repository-coverage-status.md) for Core, Platform, GameFramework, Editor, Application host surfaces outside RHI smoke validation, launcher UI/workflows, source import, cookers, conversion tools, CMake, CI, projects, and docs.
- If a later implementation stage needs to edit an out-of-scope file, that stage must first update [repository-coverage-status.md](repository-coverage-status.md) or add a linked subsystem status document.
- Stage 35 revisits every row here for threading-readiness hardening: mutable owner, phase, handoff shape, isolation, ordering/synchronization expectation, diagnostics identity, deterministic output, and any code/data-shape change needed to remove blocking private mutable handoffs.

## Module Build And Support Coverage

These rows were added during the file-level confrontation because module build/support files are real architecture files even when they are not runtime subsystems.

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Engine/Renderer/CMakeLists.txt` | Accepted | Renderer module build contract | Build dependencies can hide architecture cycles and make feature ownership unclear. Stage 9 moved NVIDIA DLSS Streamline/Vulkan linkage to `SparkleRendererNvidiaDlssProvider` so common `SparkleRenderer` no longer owns Vulkan policy. | Stage 3, Stage 4, Stage 9, Stage 36 | CMake configure and `SparkleLauncher` build after dependency changes. | Renderer target links only the layers it should own, provider-native linkage is narrow and documented, and dependency intent is documented. |
| `Engine/Renderer/Private/PCH.h` | Accepted | Renderer private module support | PCH can hide include debt if later refactors are not checked. | Preserve through Stage 36 | Include review during final cleanup. | PCH remains a compile-speed helper only and does not become an architecture dependency shortcut. |
| `Engine/Renderer/ShaderRegistrations` | Accepted | Renderer shader registration ownership | Renderer pass registration moved above RHI in Stage 4; Stage 17 shares package/layout identity; Stage 17A removed per-class shader/package/layout constants; Stage 17B removed the hand-written renderer shader registration aggregator and makes registration object files part of executable link inputs through CMake. Stage 35 made registration enumeration deterministic through sorted snapshots. | Stage 4, Stage 17, Stage 17A, Stage 17B, Stage 29, Stage 35, Stage 36 | `ShaderCompiler` package enumeration and runtime smoke. | Ordinary renderer shader packages are owned above RHI, manifest/generated registration removes repeated constants, ShaderCompiler validation reports every package, and runtime/editor executables retain registrations without a central C++ list. |
| `Engine/RHI/CMakeLists.txt` | Accepted | RHI module build contract | Backend-specific dependencies can leak into common RHI or renderer-facing code. | Stage 3, Stage 4, Stage 19, Stage 36 | CMake configure and D3D12/Vulkan runtime builds after dependency changes. | Common RHI, D3D12, Vulkan, shader runtime, and optional SDK dependencies are separated intentionally. |
| `Engine/RHI/Private/PCH.h` | Accepted | RHI private module support | PCH can hide backend include coupling. | Preserve through Stage 36 | Include review during final cleanup. | PCH remains common support and does not include backend policy. |
| `Engine/RHI/Public/RHIAPI.h` | Accepted | RHI public ABI/export boundary | Export macro policy is small but must remain the only public module linkage helper. | Preserve through Stage 36 | Header include/build check. | Public API macro remains isolated and has no renderer/backend policy. |

## Renderer Private Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Renderer.cpp` root orchestration | Accepted | Renderer facade / system root | Stage 11 moved subsystem construction/lifetime to `RendererSystemRoot` and frame lifecycle/graph state to `FramePipeline`; Stage 12 moved viewport presentation/capture internals behind host-facing protocol calls. | Stage 15 | `ShowcaseEditor` build after facade and presentation protocol split. | `Renderer` is a thin host facade; frame execution can be diagrammed as `Renderer -> FramePipeline -> FrameGraph/PassSystem`, and hosts do not manually resolve render-product resources or transitions. |
| `Camera` | Accepted | Renderer frame data | Camera/depth/handedness conventions can drift between D3D12 and Vulkan. | Stage 2, Stage 20 | Lit and normal/debug captures for both backends. | Camera/depth convention doc exists and D3D12/Vulkan captures match within agreed tolerance. |
| `Commands` | Accepted | Renderer command context / RHI command list boundary | Ownership between renderer command intent and RHI GPU commands can blur during future command-system work. | Stage 2, Stage 6, Stage 19 | RHI contract map and command caller map. | Renderer command context expresses render intent; RHI command list exposes GPU/API operations only. |
| `Debug` | Accepted | Renderer diagnostics/debug policy | Debug CVars are tiny but tied to smoke evidence and validation contracts. Stage 20 validated lit and debug/normal paths across D3D12/Vulkan editor/runtime smoke. | Stage 20, Stage 36 | Debug-view smoke evidence and config contract check. | Debug CVars and view modes are documented and stable in smoke validation. |
| `Denoising` | Accepted | Renderer ray tracing / frame graph contract | Stage 13 removed the empty private placeholder folder. The current denoising surface is the public shadow denoise contract and frame graph resource registration path; no private denoiser feature system exists yet. | Stage 18, Stage 36 | Coverage status update and docs decision. | Public denoise contract remains the integration point until Stage 18/36 decides whether to implement a denoiser feature system or keep raw visibility only. |
| `Diagnostics` | Accepted | Renderer diagnostics | Stage 14 made frame graph resource/barrier contract breaks hard development validation failures. Stage 20 smoke evidence records frame graph, DLSS, RT/TLAS, mesh/texture, and backend diagnostic state. | Stage 8, Stage 10, Stage 14, Stage 20 | Smoke log includes diagnostics capability/status. | Smoke report includes renderer memory, frame graph, GPU marker/timing, mesh, texture, DLSS, and RT status. |
| `Frame` | Accepted | Renderer frame pipeline | Stage 11 introduced `FramePipeline` for host frame phases, resize, frame graph lifetime, viewport products, and diagnostics; Stage 15 launcher-shaped D3D12/Vulkan editor/runtime smoke validated frame execution, shader reload, and resize/restore/maximize behavior. | Stage 20 | Frame pipeline smoke evidence in `artifacts/validation/stage15/stage15-smoke-results.json`. | `Frame/*` wires graph resources/passes only; pass execution lives in `Passes`/`Pipeline`, and milestone smoke keeps this path clean. |
| `FrameGraph` | Accepted | Renderer render graph system | Stage 14 converted invalid imports, invalid AS bindings, incompatible imported UAV usage, invalid pass usages, unresolved resource barriers, and unresolved aliasing barriers into hard development validation failures. Stage 15 proved normal D3D12/Vulkan editor/runtime execution has zero graph contract failures and zero unresolved-resource diagnostics. | Stage 20 | Frame graph diagnostic smoke output in `artifacts/validation/stage15`. | Development smoke fails unresolved resources/barriers and emits actionable graph diagnostics. |
| `Meshes` | Accepted | Renderer feature system / scene snapshot input | Stage 13 documented mesh snapshot ownership, Stage 25/26/31 paired runtime cooked mesh records with producer cookers, and Stage 35 keeps render snapshot ownership in `FramePipeline`. | Stage 20, Stage 24, Stage 26, Stage 31, Stage 35 | Mesh diagnostics snapshot in smoke/report. | Renderer mesh staging consumes render-domain snapshots/cooked runtime records without tool or backend-private ownership. |
| `Passes` | Accepted | Renderer pass system | Stage 17 removed central traits; Stage 17A removed repeated shader/package/layout constants; Stage 17B reduced frame insertion boilerplate and preserved ShaderCompiler validation. | Stage 17, Stage 17A, Stage 17B, Stage 20 | Proof pass before/after touch-count audit plus build/package validation. | Simple compute/raster pass can be added within the pass authoring friction budget without RHI/backend/central runtime edits or duplicated shader/package/layout constants. |
| `Pipeline` | Accepted | Renderer pipeline runtime | Stage 16 introduced `PipelineRuntimeLibrary` and explicit PSO/runtime keys; Stage 17 migrated pass definitions away from central traits; Stage 20 validated package/key/backend logging in smoke. | Stage 16, Stage 17, Stage 20 | Pipeline runtime logs include package/key/backend. | `PipelineRuntimeLibrary` owns explicit PSO keys and D3D12/Vulkan normalized descriptors. |
| `RayTracing` | Accepted | Renderer ray tracing feature system | Stage 18 clarified scene AS ownership and added TLAS/BLAS diagnostic counts; Stage 20 D3D12/Vulkan camera-motion shadow smoke passed with valid TLAS evidence. | Stage 18, Stage 20 | RT capability/TLAS/shadow smoke report. | Ray tracing contract explains BLAS/TLAS lifetime, frame graph AS import, pass services, shadow uniform ownership, rejected mesh/BLAS diagnostics, and unsupported RT fallback reasons. |
| `SceneData` | Accepted | Renderer scene bridge | Stage 13 made `RenderSceneSnapshot` an explicit renderer-owned snapshot instead of inheriting `GameSceneSnapshot`, and documented every scene DTO owner in `render-scene-data-contract.md`. | Stage 20, Stage 24 | Scene snapshot and mesh/material diagnostic output. | Renderer consumes immutable render-domain DTOs for meshes, materials, lights, cameras, skinning, and instances; remaining shared-schema extraction is tracked by Stage 24. |
| `Temporal` | Accepted | Renderer frame data / upscaling input | Stage 13 documented jitter pattern, coordinate convention, history ownership, reset triggers, and DLSS/debug validation expectations. | Stage 20 | Temporal settings in debug/smoke notes. | Temporal convention is documented and validated with lit/debug captures. |
| `Textures` | Accepted | Renderer texture feature system | Stage 13 documented texture manager ownership: cooked/runtime paths enter through `TextureSnapshot`, Renderer owns RHI texture resources, and source import/cooking stays in tools. | Stage 20 | Texture diagnostics snapshot. | Texture manager contract covers cooked input, defaults, lifetime, residency, and diagnostics. |
| `Upscaling` | Accepted | Renderer upscaling feature system | Native interop is provider/backend-owned. Stage 9 added `upscaler-provider-contract.md`, provider-target isolation, and structured failure domains; Stage 20 smoke recorded DLSS/upscaler status. | Stage 9, Stage 10, Stage 20 | DLSS capability/provider smoke log. | Provider owns SDK details; RHI owns native metadata; fallback reasons are deterministic and classified by domain. |

## Renderer Public Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Renderer.h` / `RendererAPI.h` | Accepted | Renderer facade | Stage 12 replaced manual render-product texture/resource/transition helpers with host-facing viewport presentation lifecycle and capture request methods. | Stage 15 | Application/editor build after host API migration. | Public renderer API reads as host protocol plus diagnostics, not internal subsystem access. |
| `Debug` | Accepted | Renderer public debug contract | Debug view modes are validated evidence paths, not loose debug-only switches. Stage 20 produced D3D12/Vulkan lit and normal/debug captures. | Stage 10, Stage 20 | D3D12/Vulkan debug/normal captures. | View modes work in both backends and are documented in validation contracts. |
| `Denoising` | Accepted | Renderer public shadow denoise contract | Stage 13 documents the public contract as the current integration point for shadow visibility denoise state; there is no private denoiser implementation folder yet. | Stage 18, Stage 36 | Design decision note. | Stage 18/36 either implement a denoiser feature system, keep the public contract for raw visibility only, or remove it if unused. |
| `Diagnostics` | Accepted | Renderer public diagnostics | Renderer public diagnostics are tied to smoke/report evidence through Stage 20 and Stage 34 validation. | Stage 10, Stage 20, Stage 34 | Memory diagnostics in smoke/report. | Renderer and RHI memory diagnostics appear in final validation evidence. |
| `FrameGraph` | Accepted | Renderer public graph handles/descs | Stage 14 hardened private diagnostics without expanding public handles/descs, and Stage 15 proved runtime behavior through launcher-shaped D3D12/Vulkan editor/runtime smoke. | Stage 20 | Frame graph contract doc and Stage 15 smoke. | Public frame graph types are stable handles/descs only; internals remain private. |
| `Meshes` | Accepted | Renderer public diagnostics | Stage 13 documents mesh diagnostics as Stage 15/20 smoke/report evidence: residency, instance counts, batching, bounds, and byte estimates. | Stage 15, Stage 20 | Mesh diagnostics snapshot. | Mesh diagnostic schema is documented and used by smoke/tools. |
| `Resources` | Accepted | Renderer public resource diagnostics | Stage 13 documents texture diagnostics as Stage 15/20 smoke/report evidence without backend-native handles. | Stage 15, Stage 20 | Texture/resource diagnostics snapshot. | Resource diagnostics expose health without backend-native handles. |
| `SceneData` | Accepted | Renderer public DTO contract | Stage 13 documents renderer scene DTO ownership in `render-scene-data-contract.md`. | Stage 20, Stage 24 | Scene data contract doc. | DTOs are immutable frame inputs and do not expose gameplay internals beyond the tracked runtime mesh identity pressure. |
| `ShaderParameters` | Accepted | Renderer/RHI/neutral shader authoring boundary | Large public surface overlaps RHI shader parameter concepts and must preserve the Stage 17 ownership split. | Stage 4, Stage 17 | Pass authoring contract decision. | One owner exists for shader-visible parameter authoring with no circular dependency. |
| `Shaders` | Accepted | Renderer shader reload/runtime evidence | Shader reload/runtime invalidation is tied to explicit package generation and Stage 20 smoke evidence. Stage 35 keeps catalog ordering deterministic. | Stage 16, Stage 17, Stage 20, Stage 35 | Shader reload smoke log. | Reload result reports affected packages/runtimes and backend invalidation behavior. |
| `Viewport` | Accepted | Renderer presentation bridge | Stage 12 added presentation/capture DTOs to `ViewportContracts.h`; `FramePipeline` owns texture-id resolution, native resource lookup, and render-product state transitions privately. Stage 15 D3D12/Vulkan editor smoke produced Lit and GBufferNormal captures through that protocol. | Stage 20 | `ShowcaseEditor`, `SparkleLauncher`, and Stage 15 editor smoke. | Editor receives viewport products through presentation contract, not ad hoc transitions. |

## RHI Public Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Device` | Accepted | RHI facade / service access | Root facade bloat can return if new methods bypass service ownership. | Stage 6, Stage 7, Stage 19 | RHI method ownership table and Stage 7 service contracts. | Every method has an owner service; facade is slimmed behind service responsibilities. |
| `Commands` | Accepted | RHI command service | Command-list surface must remain GPU/API level and not absorb renderer pass policy. | Stage 6, Stage 19 | Command contract map. | Command list exposes GPU/API operations with no renderer concepts. |
| `Resources` | Accepted | RHI resource service | Resource descriptors/state/view rules must map cleanly to both APIs. | Stage 6, Stage 7, Stage 19 | Resource contract doc. | Resource descriptors, state/layout, view, and subresource rules are documented and parity-tested. |
| `Pipeline` | Accepted | RHI pipeline service | Pipeline desc must align with explicit PSO key/runtime model. | Stage 16, Stage 19, Stage 20 | PSO key/runtime logs. | Pipeline desc is normalized and sufficient for D3D12/Vulkan PSO creation. |
| `Shaders` | Accepted | RHI shader package/runtime primitives | Generic shader package/runtime primitives must not absorb renderer pass policy. | Stage 4, Stage 16, Stage 17 | ShaderCompiler build and package enumeration. | RHI shader public types are generic package/reflection/runtime primitives only. |
| `ShaderParameters` | Accepted | RHI or neutral shader authoring boundary | Ownership overlaps Renderer public shader parameters and must preserve the pass-authoring split. | Stage 4, Stage 17 | Shader authoring design note. | No circular dependency; pass-specific parameters stay above RHI. |
| `Bindings` | Accepted | RHI binding layout service | Binding sets need validation against reflection/layout. | Stage 16, Stage 19 | Binding layout validation logs. | Binding sets are backend-neutral and validated against reflection/layout. |
| `Descriptors` | Accepted | RHI descriptor service | Handle lifetime and shader-visible versus CPU-only policy needs documentation. | Stage 6, Stage 19 | Descriptor ownership table. | Descriptor lifetime and visibility policy are documented and backend-parity checked. |
| `Formats` | Accepted | RHI format contract | Format translation/support matrix must remain explicit across APIs. | Stage 19, Stage 20 | Format support/parity notes. | Format support matrix covers color, depth/stencil, and sRGB rules for both APIs. |
| `Interop` | Accepted | RHI external interop service | Native interop can become a catch-all. Stage 7 introduced `RhiNativeDeviceQueueInterop` with consumer/reason metadata. | Stage 7, Stage 9, Stage 10 | DLSS/native interop service build path and later smoke logs. | Interop structs are consumer-scoped and filled deterministically by backends. |
| `Memory` | Accepted | RHI memory diagnostics/service | Memory categories must map to backend allocators and smoke reports. | Stage 7, Stage 19, Stage 20 | Memory diagnostics report. | Backend allocation stats map to common categories and final validation evidence. |
| `RayTracing` | Accepted | RHI ray tracing service | RHI RT structs must stay GPU/API-only. | Stage 18, Stage 20 | RT contract and TLAS smoke logs. | RHI RT descs expose AS geometry/build/prebuild/scratch/result concepts only. |
| `Samplers` | Accepted | RHI sampler service | Default sampler policy must match across backends. | Stage 19, Stage 20 | Sampler parity note. | Sampler desc maps equivalently to D3D12/Vulkan and default library policy is documented. |
| `Textures` | Accepted | RHI texture upload/runtime asset boundary | Cooked texture asset contracts must not blur asset/cook/runtime responsibilities. | Stage 13, Stage 19 | Texture contract decision. | Source import/cook remains in tools; runtime GPU upload contract has one owner. |
| `Diagnostics` | Accepted | RHI diagnostics service | Diagnostics must remain final evidence, not optional logs. Stage 7 routes Renderer/Application diagnostics through `RhiDiagnosticsService`. | Stage 7, Stage 10, Stage 20 | Backend diagnostics service build path and later smoke report. | Debug layers, markers, names, errors, capability logs flow through RHI diagnostics. |
| `Validation` | Accepted | RHI validation service | Validation should grow into mechanical guardrails. | Stage 3, Stage 7, Stage 14, Stage 20 | Boundary check and runtime validation output. | Resource descs, RT descs, binding compatibility, and feature requirements are validated. |
| `Capture` / `Config` / `Core` / `CVars` / `UI` / `Presentation` | Accepted | RHI capture/config/backend selection/UI/presentation bridge | Config, capture, presentation, and UI bridge can grow into hidden coupling if not owned. Stage 7 introduced capture and presentation services; Stage 8/12 own deeper cleanup. | Stage 6, Stage 7, Stage 8, Stage 12, Stage 19 | Backend selection, capture service, and UI/presentation service build evidence. | Config/capability contracts are stable; capture/presentation/ImGui use RHI service bridges without renderer/application internals. |

## RHI Private Common Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Bindings` | Accepted | RHI binding implementation | Binding set creation must validate layout/reflection errors clearly. | Stage 16, Stage 19 | Binding validation log. | Binding creation reports actionable errors and matches pipeline layout. |
| `Config` | Accepted | RHI coordinate/depth convention | Depth convention is central to D3D12/Vulkan visual parity. | Stage 2, Stage 20 | Convention doc and parity captures. | One convention is documented and validated across lit/normal/debug captures. |
| `Core` | Accepted | RHI backend selection | Backend selection should log selected API and fallback reasons. | Stage 6, Stage 20 | Backend selection log. | Backend selection is policy-only and logs backend/fallback/feature limits. |
| `CVars` | Accepted | RHI runtime config | RHI CVars should not become hidden architecture switches. | Stage 36 | Config contract check. | RHI CVars are documented or removed if not needed. |
| `Device` | Accepted | RHI backend factory/services | Device services are a good boundary but must keep method ownership aligned. | Stage 6, Stage 19 | RHI method ownership table. | Backend service creation is the only backend selection point and capability logs are consistent. |
| `Shaders` | Accepted | RHI shader infrastructure | Renderer registrations have moved above RHI; generic shader infrastructure must not regain pass policy. | Stage 4, Stage 16, Stage 17 | ShaderCompiler build/package enumeration. | Generic shader infrastructure remains in RHI while renderer registrations stay above RHI. |
| `Validation` | Accepted | RHI validation implementation | Validation must remain mandatory for development paths. | Stage 3, Stage 14, Stage 20 | Validation and smoke logs. | Development paths validate resources, bindings, RT metadata, and unsupported features. |

## D3D12 Backend Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| Root facade and type conversions | Accepted | D3D12 backend services | Root facade can regain broad ownership if new backend work bypasses service folders. | Stage 19, Stage 20 | `ShowcaseEditor` build and service map. | Root facade mostly wires service objects; type conversions are total and tested for public enums/descs. |
| `Commands` | Accepted | D3D12 command service | Command behavior must match RHI semantics and Vulkan parity. | Stage 19, Stage 20 | D3D12 smoke with barrier/draw/dispatch/AS logs. | Commands match RHI semantics for barriers, draw/dispatch, copies, AS builds, and debug markers. |
| `Descriptors` | Accepted | D3D12 descriptor service | Descriptor lifetime/recycling/failure policy needs documentation. | Stage 19, Stage 20 | Descriptor diagnostics. | CPU/GPU descriptor heap policy and recycling are documented and validated. |
| `Diagnostics` | Accepted | D3D12 diagnostics service | Diagnostics must appear in final evidence. | Stage 10, Stage 20 | D3D12 diagnostic smoke output. | Debug names/events/errors appear in captures and smoke reports. |
| `Memory` | Accepted | D3D12 memory service | Allocation strategy and transient interaction need review evidence. | Stage 19, Stage 20 | D3D12 memory diagnostics. | Heap type mapping, alignment, budgets, and transient interaction are documented. |
| `Pipeline` | Accepted | D3D12 pipeline service | Backend should consume normalized pipeline desc/PSO keys without renderer policy. | Stage 16, Stage 19, Stage 20 | D3D12 PSO key logs. | Root signature/PSO creation is deterministic from common descriptors/reflection. |
| `RayTracing` | Accepted | D3D12 ray tracing service | AS prebuild/resource creation must stay out of the root facade and parity-tested. | Stage 18, Stage 19, Stage 20 | `ShowcaseEditor` build after `D3D12RayTracingServices.*`; Stage 20 RT smoke. | D3D12 AS prebuild, scratch/result/instance resources, and AS build commands match API-neutral RHI RT descriptors and renderer RT ownership. |
| `Resources` / `Textures` | Accepted | D3D12 resource service | Constants/upload/resource lifetime must stay out of root facade pressure. | Stage 7, Stage 19, Stage 20 | D3D12 resource/capture smoke. | Resource lifetime, state assumptions, upload path, and view creation are documented and validated. |
| `Samplers` | Accepted | D3D12 sampler service | Default sampler set must match Vulkan where possible. | Stage 19, Stage 20 | Sampler parity note. | Default sampler behavior is common or documented as API-specific. |
| `SwapChain` | Accepted | D3D12 presentation service | Presentation states/resize/back-buffer lifetime need explicit contract. | Stage 12, Stage 19, Stage 20 | D3D12 resize/presentation smoke. | Presentation states, resize, back-buffer lifetime, and viewport integration are explicit. |
| `ThirdParty` | Accepted | D3D12 backend third-party isolation | Third-party utility must remain isolated and unmodified for engine policy. | Preserve through Stage 19, Stage 36 | Boundary/source grouping check. | D3DX12 code remains isolated in `ThirdParty` and is not edited for Sparkle policy. |
| `UI` | Accepted | D3D12 UI bridge | ImGui integration must not leak D3D12 objects upward. | Stage 7, Stage 12, Stage 19 | D3D12 editor UI smoke. | ImGui path uses RHI UI bridge without D3D12 objects in Application/Renderer policy. |

## Vulkan Backend Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| Root facade, includes, type conversions | Needs refactor | Vulkan backend services | Root facade mirrors broad RHI; Stage 19 moved ray tracing prebuild/resource work behind a composed backend service while root forwarding remains. Feature/extension setup is critical for DLSS/RT. | Stage 19, Stage 20 | `ShowcaseEditor` build and service map. | Root facade mostly wires services; feature/extension logs explain optional capabilities. |
| `Commands` | Needs refactor | Vulkan command service | Layouts, barriers, descriptors, dynamic rendering, and AS builds are high-risk parity points. | Stage 19, Stage 20 | Vulkan lit/normal/debug smoke logs. | Vulkan commands match RHI semantics and D3D12 captures with diagnosable layouts. |
| `Core` | Needs refactor | Vulkan core utilities | Result handling should report context for every failed call. | Stage 19, Stage 20 | Vulkan validation/error log. | Failed Vulkan calls include context and result string. |
| `Descriptors` | Needs refactor | Vulkan descriptor service | Descriptor lifetime/update policy likely affects DLSS/noise/resource binding bugs. | Stage 19, Stage 20 | Vulkan descriptor diagnostics. | Descriptor set/layout lifetime, pool growth, update validation, and future bindless policy are documented. |
| `Diagnostics` | Needs refactor | Vulkan diagnostics service | Validation warnings must be zero or classified. Stage 10 found runtime present-layout validation errors while editor capture smoke stayed clean. | Stage 10, Stage 12, Stage 19, Stage 20 | Vulkan validation layer smoke output. | Validation warnings are zero in smoke or documented as known exceptions. |
| `Memory` | Needs refactor | Vulkan memory service | Memory type selection/external memory requirements need explicit docs. | Stage 19, Stage 20 | Vulkan memory diagnostics. | Memory type selection, alignment, budgets, transient compatibility, and external memory requirements are documented. |
| `Pipeline` | Needs refactor | Vulkan pipeline service | Winding/cull/depth/viewport mapping is a critical parity point. | Stage 16, Stage 19, Stage 20 | Vulkan PSO key and visual parity logs. | Pipeline layout/PSO creation consumes normalized descs and explicitly maps conventions. |
| `RayTracing` | Needs refactor | Vulkan ray tracing service | AS prebuild/resource creation has moved out of the root facade, but runtime camera-motion parity remains unproven. | Stage 18, Stage 19, Stage 20 | `ShowcaseEditor` build after `VulkanRayTracingServices.*`; Stage 20 RT smoke. | Vulkan AS prebuild, scratch/result/instance resources, and AS build commands match API-neutral RHI RT descriptors and renderer RT ownership. |
| `Resources` / `Textures` | Needs refactor | Vulkan resource service | Image/view/layout/subresource and native view info need formal ownership. | Stage 7, Stage 9, Stage 19, Stage 20 | Vulkan capture/DLSS resource logs. | Image/view/layout/subresource ownership is documented; native view info is deterministic. |
| `Samplers` | Needs refactor | Vulkan sampler service | Default sampler behavior must match common desc policy. | Stage 19, Stage 20 | Sampler parity note. | Vulkan sampler library matches common desc behavior or documents differences. |
| `SwapChain` | Needs refactor | Vulkan presentation service | Stage 15 fixed the runtime smoke present-layout validation error for allocator-untracked acquired swapchain images. Deeper presentation service slimming and documentation remain Stage 19/20. | Stage 12, Stage 15, Stage 19, Stage 20 | Vulkan resize/presentation smoke. | Present layout, resize, acquisition, and back-buffer import into frame graph are documented. |
| `UI` | Needs refactor | Vulkan UI bridge | ImGui descriptor/image state handling must stay isolated. | Stage 7, Stage 12, Stage 19 | Vulkan editor UI smoke. | ImGui descriptor/image state handling does not leak into Renderer/Application policy. |

## Supporting Validation Surfaces

These rows are not part of the Renderer/RHI folder audit, but Stage 1 includes them because later acceptance depends on validation evidence.

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp` | Accepted | Application smoke orchestration / RHI capture service | Stage 8 removed backend-native capture implementation from Application validation; capture routes through RHI services. | Stage 8, Stage 10 | Include scan and D3D12/Vulkan capture smoke. | Application orchestrates smoke only; backend-native capture lives in RHI backend/service. |
| `Engine/Application/Private/Validation/RhiSmokeValidation.*` | Accepted | Runtime smoke orchestration | Runtime smoke records backend, frame, feature, graph, upscaler, RT, and capture evidence. Stage 15/20 fixed and revalidated the Vulkan presentation-layout blocker. | Stage 8, Stage 10, Stage 12, Stage 19, Stage 20 | Runtime smoke logs. | Runtime smoke produces backend, frame, feature, and failure evidence with zero unowned validation-layer errors. |
| `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke` | Accepted | Launcher validation workflow | Launcher smoke workflows expose backend, view mode, frame count, capture, project, target, profile, working directory, environment, logs, and artifacts through launcher-shaped evidence. | Stage 8, Stage 10, Stage 30, Stage 35 | Launcher smoke command/environment report. | Launcher can run documented smoke workflows for backend, view mode, frame count, and capture artifacts. |

## Shader Compiler And Cooking Coverage

These rows were added during the file-level confrontation. They sit outside `Engine/Renderer` and `Engine/RHI`, but the review-ready architecture cannot prove pass authoring, PSO handling, shader reflection, or renderer/RHI separation without them.

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Tools/Shaders/ShaderCompiler/CMakeLists.txt` and `Source/main.cpp` | Accepted | Shader compiler CLI/build entry | Stage 29 made package enumeration contract-based and deterministic without linking the full renderer runtime. | Stage 4, Stage 17, Stage 29, Stage 35 | `ShaderCompiler` build and command listing. | CLI can compile/list/inspect packages needed by renderer pass authoring without RHI-specific pass edits. |
| `Backends/Dxc` | Accepted | Shader compiler backend adapter | Backend compiler details emit normalized package/reflection data behind ShaderCompiler contracts. | Stage 4, Stage 16, Stage 17, Stage 29 | DXC backend compile plus reflection inspection. | DXC backend emits the same normalized reflection/package schema as other backends. |
| `Backends/Slang` | Accepted | Shader compiler backend adapter | Slang path is part of the backend contract/capability surface and does not own runtime shader policy. | Stage 4, Stage 16, Stage 17, Stage 29 | Slang backend compile plus reflection inspection. | Slang backend is either parity-compatible with the normalized schema or explicitly documented as unsupported for a target. |
| `Private/Backend` | Accepted | Shader backend registry/factory | Stage 35 made backend registration enumeration deterministic through sorted snapshots. | Stage 4, Stage 17, Stage 29, Stage 35 | Backend list/target command output. | Backend factory reports capabilities and target support clearly. |
| `Private/Compiler` | Accepted | Shader source compiler front end | Shader source identity and compile jobs are represented by deterministic contract/job data. | Stage 4, Stage 17, Stage 29, Stage 31 | Repeated cook produces stable package/cache keys. | Compiler front end has deterministic include/source identity behavior. |
| `Private/Cooking` | Accepted | Shader cook graph/package pipeline | Shader cooking consumes renderer-owned shader registrations/contracts and produces package/reflection artifacts without RHI pass edits. | Stage 4, Stage 16, Stage 17, Stage 29, Stage 31 | Cook plan/package inspection output. | A new renderer pass is cooked into packages/reflection without adding renderer-specific code to RHI. |
| `Private/Verification` | Accepted | Shader contract verifier | Shader parameter layout verification is a contract validation step before runtime use. | Stage 4, Stage 17, Stage 20, Stage 29 | Verification failure fixture or command output. | Shader parameter structs are checked against reflection before runtime use. |
| `Private/Cli` | Accepted | Shader tooling UX | ShaderCompiler exposes list, inspect, validate, and cook evidence commands used by Stages 29, 31, 34, and 35. | Stage 4, Stage 29, Stage 31, Stage 34, Stage 35 | `ShaderCompiler` help/list/inspect command output. | CLI exposes compile, list, inspect, and failure evidence for final validation. |
| `Private/Analysis` and `Private/Inspection` | Accepted | Shader package diagnostics | Shader package diagnostics and inspection are final evidence surfaces in the artifact validation matrix. | Stage 4, Stage 16, Stage 29, Stage 31 | Package stats/inspection output. | Shader package diagnostics are part of final evidence. |
| `Private/Constants` and private root support | Accepted | Shader compiler private support | Constants/PCH/types should remain support code, not hidden ownership policy. | Preserve through Stage 36 | Include/build check. | Support files remain small and do not own shader pass, RHI, or backend policy. |

## Open Stage 1 Decisions

| Decision | Why it matters | Owning stage |
| --- | --- | --- |
| Canonical camera/depth/viewport/winding convention | Prevents D3D12/Vulkan visual inversion, culling, and lighting parity bugs. | Stage 2, Stage 20 |
| `RenderCommandContext` versus `RenderCommandList` ownership | Prevents renderer intent from blending with GPU/API command contracts. | Stage 2, Stage 6, Stage 19 |
| Denoising ownership | Public shadow denoise contract is the current integration point; private feature implementation remains a Stage 18/36 decision. | Stage 18, Stage 36 |
| Shader parameter authoring owner | Prevents circular ownership between Renderer and RHI shader parameter systems. | Stage 4, Stage 17 |
| Cooked texture runtime asset owner | Prevents asset import/cook/runtime upload responsibilities from blurring. | Stage 13, Stage 19 |
| RHI UI/presentation bridge owner | Prevents ImGui/presentation helpers from becoming renderer/application internals. | Stage 7, Stage 12, Stage 19 |

## Stage 1 Completion Packet

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 1 - Baseline Status And Evidence Freeze. No split checkpoint was required because this stage is documentation-only. |
| Status | Fully completed. Reopen only if a new durable Renderer/RHI, validation smoke, launcher smoke, or shader compiler/cooking source root appears without a coverage row. |
| Target docs opened | `docs/plans/rhi-renderer-review-ready-implementation-plan.md`, `docs/plans/rhi-renderer-architecture-review.md`, `docs/plans/architecture-review-acceptance-rubric.md`, `docs/architecture/rendering-coverage-status.md`, `docs/architecture/repository-coverage-status.md`, `docs/architecture/before/repository-current-state.md`, `docs/architecture/after/repository-target-architecture.md`, `docs/architecture/after/repository-threading-readiness.md`, `docs/plans/implementation/stage-prompt-packets.md`, `docs/plans/tutor/stage-learning-guide.md`. |
| Contract surfaces touched | Documentation state only: rendering coverage, whole-repository coverage delegation, evidence tracking, open decisions, and future validation artifacts. |
| Refactor disposition | Keep and refine the evidence tables. Do not move, rename, or rewrite runtime/tool source during Stage 1. |
| Complexity right to exist | This table earns its complexity by preventing forgotten subsystems: every row names owner layer, risk, consumer stage, validation artifact, and final acceptance evidence. Duplicate prose remains delegated to the architecture review and whole-repository coverage map. |
| Data transfer contract | Architecture state is transferred through this file and linked whole-repository docs. No source dependencies, hidden ownership, or runtime data paths are created by this stage. |
| Threading readiness handoff | Rows keep future threading-sensitive owners visible: frame orchestration, command recording, frame graph, resources, shader cooking, launcher smoke orchestration, and backend services feed Stage 35's immutable handoff and mutable-owner hardening pass. |
| Acceptance proof | All rows from `Whole-Codebase Coverage Audit` are represented here or delegated to `repository-coverage-status.md`; every `Needs refactor` row links to a later stage; every `Needs design decision` row has an explicit question; file inventory is 547 tracked files with 0 unmapped files. |
| Validation | Re-ran the inventory command listed above on 2026-06-13. Docs-only stage; no build or runtime smoke was required. |

## Stage 1 Acceptance Check

- Every coverage row from `Whole-Codebase Coverage Audit` has a status and owner.
- File-level confrontation covers 547 tracked files across Renderer, RHI, validation smoke, launcher smoke, and shader compiler/cooking support.
- Current unmapped file count is 0.
- Every `Needs refactor` row links to one or more later stages.
- Every `Needs design decision` row includes an explicit question.
- Supporting validation and shader compiler/cooking surfaces are tracked because later acceptance depends on them.
- No runtime code was changed for this stage.

## Stage 5 Validation Milestone A

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 5 - Validation Milestone A: Boundaries And Shader Registration. |
| Status | Fully completed for the first boundary/shader-registration validation slice. |
| Target docs opened | `docs/architecture/architecture-boundary-guardrails.md`, `docs/architecture/pass-authoring-contract.md`, `docs/architecture/pipeline-runtime-contract.md`, `docs/architecture/tooling-pipeline-contract.md`, `docs/architecture/rendering-coverage-status.md`, `docs/architecture/after/repository-threading-readiness.md`, `docs/plans/rhi-renderer-review-ready-implementation-plan.md`. |
| External validation basis | NVRHI tutorial validation layer mindset, Donut Samples executable-evidence mindset, and Vulkan validation-layer style of explicit diagnostic gates. |
| Boundary command | `cmake -DSPARKLE_REPO_ROOT="$PWD" -P CMake/ArchitectureBoundaryCheck.cmake` passed with no new violations. Stage 8 removed the Application D3D12 capture/readback exception; Stage 9 narrows Streamline/Vulkan interop to provider-owned counted exceptions. |
| RHI-to-Renderer include check | `rg "Renderer/Private" Engine/RHI` returned no matches. There are no permanent RHI-to-Renderer include exceptions. |
| Configure command | `cmake -S . -B build/windows-vs2026-stage5 -G "Visual Studio 18 2026" -A x64` passed. CMake selected MSVC 19.51.36247.0, Windows SDK 10.0.26100.0, Qt `C:/Qt/6.11.1/msvc2022_64`, and wrote the generated tree to `build/windows-vs2026-stage5`. Configure emitted third-party/dependency developer warnings for Assimp `CMP0175` and FetchContent `CMP0169`; no Sparkle configure failure. |
| ShaderCompiler build | `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target ShaderCompiler -- /nologo /v:minimal /m:1` passed and produced `artifacts/dev/tools/ShaderCompiler/DevelopmentEditor/ShaderCompiler.exe`. |
| Renderer registration build surface | The `ShaderCompiler` target built `SparkleRendererShaderRegistrations.lib`, proving the narrow registration handoff compiles without linking the full renderer runtime. |
| Launcher/editor-facing build surface | `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1` passed and produced `artifacts/dev/launcher/DevelopmentEditor/SparkleLauncher.exe`. Deploy step warned that `VCINSTALLDIR` was not set while deploying runtime dependencies, but the target build completed. |
| Shader package enumeration | `ShaderCompiler.exe list-shaders --validate` reported `17 typed shader registration(s) valid`. `ShaderCompiler.exe list-shaders` listed RHI generic packages `HelloTriangle`, `HelloInlineRayQuery`, `HelloRayTracingLibrary` and renderer packages `GBuffer`, `DirectLighting`, `IndirectLighting`, `LightingComposite`, `Sky`, `VisualizeBuffers`, and `ComputeClear`. |
| Format target | `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target clang_format_check -- /nologo /v:minimal` could not run because `clang-format` was not found during configure (`CLANG_FORMAT_EXE-NOTFOUND`), so CMake did not generate `clang_format_check.vcxproj`. |
| Data transfer contract | Validation evidence transfers through this status row and command outputs. ShaderCompiler consumes renderer package registration through `SparkleRendererShaderRegistrations`; RHI receives only generic cooked shader package/reflection/layout/runtime primitives. |
| Threading readiness handoff | Package enumeration evidence is read-only/static. Future parallel shader jobs should consume immutable package manifests/catalog records and emit deterministic reports rather than querying live renderer runtime state. |
| Remaining risk | Build validation used a fresh VS2026 tree because the existing `build` cache targets `Visual Studio 17 2022`, which is not available in this shell. Stage 17 still owns duplicate pass registration versus runtime package metadata cleanup. |

## Stage 7 RHI Service Extraction Evidence

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 7 - Extract First RHI Services: Interop, Capture, Diagnostics, Presentation. |
| Status | Fully completed for first public service contracts and caller migration slice. |
| Service contracts | `Engine/RHI/Public/Interop/RhiInteropService.h`, `Engine/RHI/Public/Capture/RhiCaptureService.h`, `Engine/RHI/Public/Diagnostics/RhiDiagnosticsService.h`, `Engine/RHI/Public/Presentation/RhiPresentationService.h`. |
| Caller evidence | Renderer upscaling receives `RhiNativeDeviceQueueInterop`; FrameGraph native view and back-buffer resolution use interop/presentation services; Application present/capture/diagnostics paths use services; D3D12/Vulkan backend facades compose service adapters instead of inheriting from all service interfaces. |
| Remaining exceptions | Stage 8 removed the Application D3D12-native validation capture exception; Stage 9 restricts Streamline/Vulkan native interop to `SparkleRendererNvidiaDlssProvider` and `StreamlineDlssRuntime.cpp` under the provider contract. |
| Data transfer contract | Native interop uses consumer-tagged `RhiNativeDeviceQueueInterop`; capture uses `RhiTextureCaptureRequest` and `RhiCaptureResult`; presentation/UI uses `RhiPresentationService`; diagnostics uses `RhiDiagnosticsService`. |
| Threading readiness handoff | Service request/result packets make later queued capture jobs, provider interop validation, and host presentation protocol work possible without exposing mutable backend roots. |
| Validation | `architecture_boundary_check` passed; `SparkleLauncher`, `SparkleApplication`, and `ShowcaseEditor` built in `build/windows-vs2026-stage5` with `DevelopmentEditor`. `SparkleRenderer` reached output before the 120s command timeout and was subsequently rebuilt as a dependency of the successful targets. |

## Stage 8 Smoke Capture Ownership Evidence

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 8 - Move Smoke Capture And Backend-Native Validation Behind RHI. |
| Status | Fully completed for code ownership, boundary exception removal, and targeted builds. Full backend runtime smoke is deferred to Stage 10. |
| Application validation | `RhiSmokeEditorValidation.cpp` contains no D3D12/Vulkan native headers or native capture implementation. It requests capture through `RhiCaptureService` and logs backend, view mode, frame, path, status, and failure reason. |
| Renderer diagnostics | `RendererSmokeDiagnosticsSnapshot` reports backend, frame graph unresolved-barrier warnings, upscaler provider/status/reason, and ray tracing/inline ray-query support without Application reading renderer internals. Smoke validation fails if unresolved frame graph barrier warnings are reported. |
| RHI capture/readback | D3D12 and Vulkan capture services return `RhiCaptureResult` with backend, status, frame, view mode, artifact path, and precise failure reason. |
| Launcher smoke workflow | Launcher CLI/GUI request data now includes smoke backend, frame limit, view mode, capture path, trace, and level-switching controls. Launch plans transfer view mode through `SPARKLE_SMOKE_VIEW_MODE` and capture path through `SPARKLE_SMOKE_SCENE_COLOR_CAPTURE`. |
| Boundary evidence | `architecture_boundary_check` passed after removing the `APPLICATION_VALIDATION_NO_BACKEND_NATIVE` counted exception. |
| Build evidence | `SparkleApplicationEditor` and `SparkleLauncher` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`. Launcher deploy emitted a `VCINSTALLDIR` warning after producing the executable. |

## Stage 10 Validation Milestone B Evidence

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 10 - Validation Milestone B: RHI Services, Capture, Interop, Upscaling. |
| Status | Fully completed. Backend/editor capture, DLSS evidence, runtime smoke, and Application ownership checks pass. Stage 15 fixed and retested the runtime Vulkan present-layout blocker. |
| Build evidence | `SparkleLauncher`, `ShaderCompiler`, `ShowcaseEditor`, `ShowcaseRuntime`, and `architecture_boundary_check` built or ran successfully in `build/windows-vs2026-stage5` with `DevelopmentEditor`. `ShaderCompiler` passed after rerunning alone following a Visual Studio `ZERO_CHECK.lastbuildstate` lock from parallel build activity. |
| Format evidence | `clang_format_check` is not generated in this build tree, so formatting validation could not run as a target. |
| Boundary evidence | Direct CMake boundary check passed with only provider-owned counted exceptions for `SparkleRendererNvidiaDlssProvider` Vulkan linkage and `StreamlineDlssRuntime.cpp` Vulkan identifiers. |
| Application ownership evidence | Native backend scan over `Engine/Application/Private/Validation` returned no D3D12/Vulkan header, type, API identifier, or backend-folder matches. |
| Editor D3D12 evidence | Launcher-shaped direct execution of the `Showcase` editor smoke path. `artifacts/validation/stage10/d3d12-lit.log` and `d3d12-normal.log` exited `0`, report `frameGraphUnresolvedBarrierWarnings=0`, DLSS active with `failureDomain=None`, ray tracing and inline ray query support, and captured frame 30 to `d3d12-lit.bmp` and `d3d12-normal.bmp`. |
| Editor Vulkan evidence | Launcher-shaped direct execution of the `Showcase` editor smoke path. `artifacts/validation/stage10/vulkan-lit.log` and `vulkan-normal.log` exited `0`, report `frameGraphUnresolvedBarrierWarnings=0`, DLSS active with `failureDomain=None`, ray tracing and inline ray query support, and captured frame 30 to `vulkan-lit.bmp` and `vulkan-normal.bmp`. |
| Capture artifacts | `d3d12-lit.bmp`, `d3d12-normal.bmp`, `vulkan-lit.bmp`, and `vulkan-normal.bmp` are 32-bit BMP files with `BM` signatures and 21,608,214-byte payloads. |
| Runtime D3D12 evidence | Launcher-shaped direct execution of the `Showcase` runtime smoke path. `artifacts/validation/stage10/d3d12-runtime.log` exited `0`, reports `frameGraphUnresolvedBarrierWarnings=0`, DLSS active with `failureDomain=None`, ray tracing and inline ray query support, and no error/critical lines. |
| Runtime Vulkan evidence | Initial Stage 10 launcher-shaped runtime smoke exposed validation-layer present-layout errors for acquired swapchain images after resize. Stage 15 fixed the Vulkan back-buffer transition path and reran `runtime-vulkan`; `artifacts/validation/stage15/runtime-vulkan.log` exited `0` with zero diagnostic matches. |
| Artifact index | Editor matrix: `artifacts/validation/stage10/stage10-smoke-results.json`. Runtime matrix: `artifacts/validation/stage10/stage10-runtime-results.json`. Logs and console captures are stored beside the BMP artifacts with backend/view/runtime names. |
| Data transfer contract | Evidence flows through smoke logs, BMP artifacts, JSON result summaries, backend capability reports, and this status table. Capture and DLSS state are structured log data, not visual inference. |
| Threading readiness handoff | Smoke requests are environment/process packets and outputs are immutable logs/artifacts, which keeps future launcher orchestration and backend smoke execution compatible with queued jobs or worker-owned validation runs. |
| Remaining risk | Stage 19 has since closed backend presentation/resource service ownership, and the Stage 10 validation blocker is closed by Stage 15 smoke evidence. |

## Stage 11 Renderer Facade Decomposition Evidence

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 11 - Decompose Renderer Into Facade, System Root, Frame Pipeline. |
| Status | Fully completed for targeted facade decomposition and build validation. |
| Facade evidence | `Engine/Renderer/Private/Renderer.cpp` delegates host lifecycle, viewport products, diagnostics, RHI access, shader reload, and presentation helper calls instead of owning subsystem construction or frame execution details. |
| System-root evidence | `Engine/Renderer/Private/Host/RendererSystemRoot.*` owns backend services, pipeline manager, mesh/texture/material scene systems, ray tracing scene/settings, upscaler subsystem, memory monitor, scene coordinator, and post-load flush. |
| Frame-pipeline evidence | `Engine/Renderer/Private/FramePipeline/FramePipeline.*` owns resize events, frame graph construction/refresh, viewport products, frame diagnostics, begin/setup/record/submit/end frame, frame context build, graph setup/compile/execute, RT/TLAS binding, and upscaler frame input setup. |
| Public API evidence | `Engine/Renderer/Public/Renderer.h` now owns only `RendererSystemRoot` and `FramePipeline` pointers privately. Public methods remain stable for host lifecycle/diagnostics/RHI access and expose viewport presentation/capture protocol methods instead of manual render-product texture/resource/transition helpers. |
| CMake evidence | `Engine/Renderer/CMakeLists.txt` uses `CONFIGURE_DEPENDS` for renderer public/private source globs so new renderer source files are discovered by the generated build graph. |
| Data transfer contract | Host calls still enter through `Renderer`; subsystem dependencies are built in `RendererSystemRoot`; frame state flows through `FramePipeline`, `FrameContext`, frame graph products, pass runtime services, and diagnostics. No new singleton/global cross-system access was added. |
| Threading readiness handoff | Mutable lifetime ownership and per-frame graph/diagnostic state are separated. Future render-thread work can reason about construction/lifetime separately from setup, record, submit, present, and diagnostics phases. |
| Validation | VS2026 configure passed in `build/windows-vs2026-stage5`; `ShowcaseEditor` and `SparkleLauncher` built with `DevelopmentEditor`; direct architecture boundary check passed with only provider-owned counted exceptions. Source text hygiene scan over changed renderer source found no planning/stage/provenance text. |
| Remaining risk | Closed by Stage 15 launcher-shaped D3D12/Vulkan smoke. Stage 16/17 must preserve the facade and frame pipeline split while changing PSO/pass ownership. |

## Stage 12 Viewport Presentation Bridge Evidence

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 12 - Add Viewport Presentation Bridge And Clean Host Protocol. |
| Status | Fully completed for targeted host protocol cleanup and build validation. |
| Host protocol evidence | `Engine/Renderer/Public/Renderer.h` exposes `BeginViewportPresentation`, `EndViewportPresentation`, and `CaptureViewportProductToBmp` instead of public `ResolveRenderProductTextureId`, `ResolveRenderProductResource`, and `TransitionRenderProduct` helpers. |
| Viewport contract evidence | `Engine/Renderer/Public/Viewport/ViewportContracts.h` owns `ViewportPresentationProduct` and `ViewportCaptureRequest`, keeping Application/Editor handoffs as renderer DTOs rather than frame graph or backend-native objects. |
| Frame-pipeline evidence | `Engine/Renderer/Private/FramePipeline/FramePipeline.*` privately resolves frame graph products, calls the RHI presentation service for ImGui texture IDs, owns render-product state transitions, and fills RHI capture requests without exposing native resources to Application. |
| Application/editor evidence | `Engine/Application/Private/EditorApplication.cpp` and `Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp` use the renderer viewport presentation lifecycle; they no longer call manual render-product transitions or native resource resolution. |
| Data transfer contract | Application/Editor send viewport requests and receive presentation DTOs. Texture IDs transfer through RHI presentation service inside Renderer/FramePipeline. Smoke capture transfers through `ViewportCaptureRequest` to renderer-owned RHI capture service calls. |
| Threading readiness handoff | Viewport presentation now has explicit begin/end lifecycle calls and capture request packets. Future render-thread or queued capture work can route by output, frame, view mode, and artifact path without reading Application or frame graph private mutable state. |
| Validation | `ShowcaseEditor` and `SparkleLauncher` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`; `architecture_boundary_check` passed; source text hygiene scan over touched source found no planning/stage/provenance text. `clang_format_check` could not run because the VS2026 build tree has no generated `clang_format_check` target. |
| Remaining risk | Stage 15 closed launcher-shaped D3D12/Vulkan runtime/editor smoke for presentation parity. Stage 19 backend presentation/resource service ownership is complete. |

## Stage 13 Scene Data Ownership Evidence

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 13 - Clean Scene Data, Mesh, Texture, Temporal Ownership. |
| Status | Fully completed for scene snapshot boundary tightening, ownership documentation, and placeholder cleanup. |
| Scene contract evidence | [render-scene-data-contract.md](render-scene-data-contract.md) names camera, light, material, texture, mesh, skinning, temporal, and viewport handoffs with producer, renderer owner, consumer, diagnostics, and remaining pressure. |
| Code evidence | `Engine/Renderer/Private/SceneData/Lifecycle/RenderSceneSnapshot.*` now owns explicit renderer frame snapshot fields rather than deriving from `GameSceneSnapshot`. |
| Denoising evidence | The empty private `Engine/Renderer/Private/Denoising` placeholder folder was removed. Current denoise ownership is the public shadow denoise contract plus frame graph resource registration, with implementation choice deferred to Stage 18/22. |
| Diagnostics plan | Mesh diagnostics, texture diagnostics, material binding health, and temporal/upscaler state are represented in the scene contract as Stage 15/20 smoke/reporting evidence. |
| Data transfer contract | GameFramework produces `GameSceneSnapshot`; Renderer captures `RenderSceneSnapshot`; scene/resource systems consume renderer-owned frame data and send GPU-adjacent work to RHI through public descriptors/resources. |
| Threading readiness handoff | Scene mutation remains GameFramework-owned; renderer frame setup consumes a snapshot copy; cache ownership and temporal history are single-owner renderer state. Remaining mesh runtime identity pressure is explicitly owned by Stage 24. |
| Remaining risk | `MeshSnapshot` still carries runtime `Mesh*` references for morph/dirty geometry and GPU cache identity. Stage 24 must decide whether to extract a render mesh payload/key contract or retain the exception with stronger evidence. |

## Stage 14 Frame Graph Contract Evidence

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 14 - Harden Frame Graph Contract And Diagnostics. |
| Status | Fully completed for code hardening, contract documentation, targeted build validation, and Stage 15 launcher-shaped D3D12/Vulkan smoke closure. |
| Code evidence | `FrameGraphBarrierPlanPlayback.cpp` no longer logs and skips unresolved resource or aliasing barriers; it records the smoke-visible counter and fails with pass, handle, resource name, state/label, physical block, and remediation data. |
| Resource evidence | `FrameGraphTextureRegistration.cpp`, `FrameGraphExternalResources.cpp`, and `FrameGraphAccelerationStructureRegistration.cpp` fail invalid external resource imports, missing UAV support for imported resources, and invalid AS import/bind data at the graph boundary. |
| Declaration evidence | `FrameGraphResourceContractDiagnostics.cpp` uses hard development validation for invalid pass resource usage and invalid acceleration-structure parameter binding. |
| Contract evidence | [frame-graph-contract.md](frame-graph-contract.md) names declare/compile/plan/allocate/execute responsibilities plus each Stage 14 validation class, diagnostic payload, and remediation direction. |
| Data transfer contract | Pass/resource intent enters through typed declarations and usage descriptors; the compiled graph transfers barrier plans, resolved resources/views, and command contexts to RHI; diagnostics transfer pass/resource/usage/state/allocation/remediation evidence. |
| Threading readiness handoff | The graph still executes serially, but invalid mutable/resource state is stopped before command recording continues. Future command-batch work can trust a frozen `FrameGraphPlan` and deterministic validation failures instead of skipped barriers. |
| Source text discipline | No explanatory/provenance/planning comments were added to source. New runtime strings are diagnostic failure text with remediation details. |
| Validation | `ShowcaseEditor` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`; `architecture_boundary_check` passed; source text hygiene scan over touched source found no planning/stage/provenance text. |
| Remaining risk | Closed by Stage 15 launcher-shaped smoke; later pass/PSO work must preserve zero frame graph contract failures and zero unresolved-barrier counters. |

## Stage 15 Renderer Facade, Frame Pipeline, Frame Graph Validation Evidence

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 15 - Validation Milestone C: Renderer Facade, Frame Pipeline, Frame Graph. |
| Status | Fully completed. |
| Code fix | `Engine/RHI/Private/Vulkan/Commands/VulkanRenderCommandList.cpp` now handles allocator-untracked external present images correctly when transitioning from present into a writable Vulkan layout. This closed the previous runtime Vulkan swapchain image layout validation error without changing Renderer or Application ownership. |
| Build evidence | `SparkleLauncher`, `ShowcaseEditor`, and `ShowcaseRuntime` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`. `ShowcaseRuntime` initially hit a parallel Visual Studio `.tlog` lock and passed when rerun alone. |
| Boundary evidence | `cmake -DSPARKLE_REPO_ROOT="$PWD" -P CMake/ArchitectureBoundaryCheck.cmake` passed with only provider-owned counted exceptions. |
| Launch basis | Launcher-shaped direct execution of the `RunProject` smoke path for `Showcase`, `DevelopmentEditor`, working directory `Projects/Showcase`; environment included `SPARKLE_SMOKE_VALIDATE_RHI=1`, `SPARKLE_SMOKE_FRAME_LIMIT=120`, `SPARKLE_SMOKE_SHADER_RELOAD_FRAME=60`, `SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING=1`, `SPARKLE_STARTUP_LEVEL=Sponza`, backend-specific `SPARKLE_RHI_BACKEND`, per-run `SPARKLE_LOG_FILE`, and editor capture fields. |
| Editor D3D12 evidence | `editor-d3d12-lit` and `editor-d3d12-normal` exited `0`, reported zero diagnostic matches, and produced 21,608,214-byte BMP captures in `artifacts/validation/stage15`. |
| Editor Vulkan evidence | `editor-vulkan-lit` and `editor-vulkan-normal` exited `0`, reported zero diagnostic matches, and produced 21,608,214-byte BMP captures in `artifacts/validation/stage15`. |
| Runtime evidence | `runtime-d3d12` and `runtime-vulkan` exited `0` and reported zero diagnostic matches. The previous Vulkan present-layout validation-layer error did not recur. |
| Shader reload and resize evidence | Smoke ran with `SPARKLE_SMOKE_SHADER_RELOAD_FRAME=60`; runtime smoke exercised restore/maximize resize behavior through the existing smoke workflow and retained zero diagnostic matches. |
| Artifact index | `artifacts/validation/stage15/stage15-smoke-results.json` records executable path, working directory, backend, view mode, frame limit, shader reload frame, capture path, logs, exit code, timeout state, and diagnostic matches for all six runs. |
| Format evidence | `clang_format_check` is not generated in this VS2026 build tree; the attempted target build failed with missing `clang_format_check.vcxproj`. |
| Source text discipline | No explanatory/provenance/planning comments were added to source. A source-text scan over the touched Vulkan file found no stage/provenance planning text. |
| Data transfer contract | Validation evidence transfers through smoke logs, BMP captures, the JSON result summary, and this status table. The renderer facade and presentation bridge remain the host-facing protocol; Application does not own frame graph transitions. |
| Threading readiness handoff | The milestone proves deterministic launcher-shaped process packets and immutable artifacts for future queued validation. Frame graph plans remain frozen before command recording, and Vulkan back-buffer native layout handling stays backend-local. |
| Remaining risk | Stage 16/17 can start PSO/pass runtime work from a clean renderer/frame graph baseline. Stage 19 backend presentation/resource service ownership is complete. |

## Stage 16 Pipeline Runtime Key Evidence

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 16 - Introduce Explicit PSO Key And Pipeline Runtime Library. |
| Status | Completed for the explicit runtime key/library bridge. Stage 17 still owns declarative pass definitions and deletion of central traits/type-index entry identity. |
| Code evidence | `Engine/Renderer/Private/PipelineRuntime/PipelineRuntimeKey.*` defines printable backend-normalized pipeline runtime identity. `Engine/Renderer/Private/PipelineRuntime/PipelineRuntimeLibrary.*` owns package loading, package capability validation, binding layout creation, explicit key logging, and RHI PSO creation from normalized descriptors. |
| Compatibility evidence | `RenderPassShaderRuntime.h` now adapts existing pass traits into `PipelineRuntimeLibrary`; `PipelineStateManager.h` names the current type-index storage as legacy until Stage 17 removes that path. |
| Key evidence | The runtime key records pass name, package declaration, package id, binding layout id, backend, required shader binary format, pipeline kind, shader stages, required/package features, shader package generation, package key, source hash, binding-layout hash, vertex layout, pixel-stage presence, wireframe/cull/winding/depth/stencil state, render-target formats, and depth-stencil format. |
| Data transfer contract | Shader package data transfers from `CookedShaderPackageCache` and cooked headers into `PipelineRuntimeKey`; binding/layout creation and PSO creation transfer through `PipelineRuntimePackageRequest`, `RenderBindingLayoutCompileDesc`, `GraphicsPipelineStateDesc`, and `ComputePipelineStateDesc`. D3D12/Vulkan backend code continues to consume normalized RHI descriptors rather than renderer pass types. |
| Threading readiness handoff | Explicit immutable key construction makes later background PSO warmup, package-generation invalidation, and cache reporting possible without reading live pass instances. |
| Validation | `ShowcaseEditor` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`; CMake regenerated after discovering the new `PipelineRuntime` files. `architecture_boundary_check` passed with only provider-owned counted DLSS exceptions. A source text hygiene scan over `Engine/Renderer/Private/Pipeline` and `Engine/Renderer/Private/PipelineRuntime` found no stage/provenance planning text. |
| Format evidence | `clang_format_check` is not generated in the VS2026 build tree; the attempted target build failed with missing `clang_format_check.vcxproj`. |
| Remaining risk | Stage 17 removed central pass traits and type-index entry identity. Stage 20 must include runtime logs as validation artifacts for D3D12/Vulkan lit and debug/normal smoke. |

## Stage 17 Pass Definition Evidence

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 17 - Introduce Declarative Pass Definition And Migrate Passes. |
| Status | Fully completed for ordinary renderer passes. |
| Code evidence | [RenderPassDefinition.h](../../Engine/Renderer/Private/Passes/RenderPassDefinition.h) defines renderer-owned pass intent; [RenderPassDefinitionRuntime.h](../../Engine/Renderer/Private/Pipeline/RenderPassDefinitionRuntime.h) converts definitions to runtime storage and normalized RHI descriptors. |
| Migrated passes | `ComputeClear`, `GBuffer`, `DirectLighting`, `IndirectLighting`, `LightingComposite`, `Sky`, and `VisualizeBuffers` expose `GetDefinition()` and `PipelineRuntime`. |
| Removed path | `Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h` was deleted. Source scans over `Engine/Renderer` found no `RenderPassPipelineTraits`, `DescribeShaderPackage`, `DescribeGBufferShaderPackage`, `std::type_index`, or `m_legacyRuntimeStorageByPass` references. |
| Package identity | [RendererShaderPackages.h](../../Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h) provides shared package/layout names used by pass definitions and renderer shader registrations. |
| Data transfer contract | Pass intent transfers through `RenderPassDefinition`; shader metadata transfers through renderer-owned shader registrations and cooked package reflection; runtime lookup transfers pass definition data into `PipelineRuntimeLibrary` and RHI pipeline descriptors. |
| Threading readiness handoff | Definitions are immutable metadata and can later feed PSO warmup, shader cook planning, or worker command-recording preparation without reading live pass instances. |
| Validation | `ShowcaseEditor`, `ShaderCompiler`, and `architecture_boundary_check` passed in `build/windows-vs2026-stage5`; `ShaderCompiler.exe list-shaders --validate` reported 17 valid typed registrations. A source text hygiene scan over touched pass/pipeline/registration source found no stage/provenance planning text. |
| Remaining risk | Stage 17B must measure and reduce the whole pass-add workflow; Stage 20 must capture D3D12/Vulkan runtime smoke with PSO key logs. Stage 29 can decide whether the Stage 17B manifest/generator becomes the long-term ShaderContracts shape. |

## Stage 17A Shader Registration Boilerplate Target

| Field | Target |
| --- | --- |
| Stage / checkpoint | Stage 17A - Remove Shader Registration Boilerplate With Manifest-Driven Authoring. |
| Status | Fully completed for renderer shader registration constant removal. |
| Current evidence | Renderer shader classes keep typed parameter structs and real package feature flags; package/path/entry/stage metadata is authored once through `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`. Source scans over `Engine/Renderer/ShaderRegistrations` found no `kShaderName`, `kShaderPackageName`, `kBindingLayoutId`, or old `IMPLEMENT_GLOBAL_SHADER(` usage. |
| Target evidence | `ShaderCompiler.exe list-shaders --validate` reported 17 valid typed registrations. `ShaderCompiler.exe list-shaders` confirmed renderer package/layout ids remain `GBuffer`, `DirectLighting`, `IndirectLighting`, `LightingComposite`, `Sky`, `VisualizeBuffers`, and `ComputeClear`. |
| Data transfer contract | Shader authoring metadata transfers through source metadata registration into ShaderCompiler and through shared package/layout identity into `RenderPassDefinition`. |
| Threading readiness handoff | Deterministic manifest/generated records can feed parallel shader cook jobs without scanning live renderer runtime objects or depending on C++ static initialization order. |

## Stage 17B Pass Authoring Friction Target

| Field | Target |
| --- | --- |
| Stage / checkpoint | Stage 17B - Pass Authoring Friction Budget And One-Command Workflow. |
| Status | Fully completed for the Stage 17B implementation slice. Frame insertion boilerplate reduction, shader registration validation, and central renderer registration-list cleanup are complete. The larger catalog/scaffolder decision is intentionally carried by Stage 29. |
| Current evidence | `FrameGraphBuilder` owns `AddComputeShaderPass` and `AddRasterShaderPass`; `VisualizeBuffers`, `LightingComposite`, `DirectLighting`, `IndirectLighting`, `Sky`, and `GBuffer` frame insertion now allocate parameters, declare resources, and call a typed shader-pass helper instead of repeating pass name/runtime lookup/pass construction/execute lambdas. `RendererGlobalShaders.cpp` and `RendererShaderRegistration.h` were deleted; `Engine/Renderer/CMakeLists.txt` now links renderer shader registration object files into runtime consumers without a hand-written C++ package list. |
| Target evidence | `pass-authoring-contract.md` contains current workflow and before/after touch-count evidence for `ComputeClear`, `VisualizeBuffers`, and `GBuffer`. `ShaderCompiler.exe list-shaders --validate` reports 17 valid typed registrations. `ShowcaseEditor` and Stage 20 smoke prove runtime executables retain renderer shader registrations after the aggregator deletion. |
| Data transfer contract | Pass intent transfers from typed pass definitions into frame graph declaration through `AddComputeShaderPass`/`AddRasterShaderPass`; shader authoring metadata transfers through source metadata registration into ShaderCompiler package enumeration and validation evidence. |
| Threading readiness handoff | Pass manifests/records are immutable and deterministic, so future parallel shader cook and PSO warmup jobs can consume them without live renderer state or static registration ordering. |

## Stage 20 Full Renderer/RHI Backend Parity Evidence

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 20 - Validation Milestone D: Full Renderer/RHI Backend Parity. |
| Status | Fully completed. Build, boundary, shader package, editor smoke, runtime smoke, capture, frame graph, DLSS, RT capability, RT scene, PSO runtime, visual comparison, and deterministic camera-motion RT validation passed for D3D12 and Vulkan. |
| Build evidence | `ShaderCompiler`, `SparkleLauncher`, `ShowcaseEditor`, `ShowcaseRuntime`, and `architecture_boundary_check` built or ran successfully with `DevelopmentEditor` in `build/windows-vs2026-stage5`. `SparkleLauncher` emitted a non-blocking `VCINSTALLDIR`/system `icuuc.dll` warning while still building. |
| Boundary evidence | `architecture_boundary_check` passed with only counted provider-owned DLSS integration exceptions: `NVIDIA DLSS provider Vulkan::Vulkan link` and `Streamline DLSS Vulkan bridge`. |
| Shader package evidence | `artifacts/dev/tools/ShaderCompiler/DevelopmentEditor/ShaderCompiler.exe list-shaders --validate` exited `0`; it reports 17 valid typed shader registrations after the renderer shader registration aggregator was deleted. |
| Launch basis | Launcher-shaped direct execution mirrored the validation workflow contract: `Showcase`, `DevelopmentEditor`, working directory `Projects/Showcase`, `SPARKLE_SMOKE_VALIDATE_RHI=1`, `SPARKLE_SMOKE_FRAME_LIMIT=120`, `SPARKLE_SMOKE_SHADER_RELOAD_FRAME=60`, `SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING=1`, `SPARKLE_STARTUP_LEVEL=Sponza`, backend-specific `SPARKLE_RHI_BACKEND`, per-run `SPARKLE_LOG_FILE`, and editor capture fields. |
| Editor D3D12 evidence | `editor-d3d12-lit` and `editor-d3d12-normal` exited `0`, reported zero diagnostic matches, zero frame graph unresolved warnings, active DLSS, RT/inline ray query support, and produced `artifacts/validation/stage20/d3d12-stage20-lit.bmp` and `artifacts/validation/stage20/d3d12-stage20-normal.bmp`. |
| Editor Vulkan evidence | `editor-vulkan-lit` and `editor-vulkan-normal` exited `0`, reported zero diagnostic matches, zero frame graph unresolved warnings, active DLSS, RT/inline ray query support, and produced `artifacts/validation/stage20/vulkan-stage20-lit.bmp` and `artifacts/validation/stage20/vulkan-stage20-normal.bmp`. |
| Runtime evidence | `runtime-d3d12` and `runtime-vulkan` exited `0` with zero diagnostic matches. Runtime logs report zero frame graph unresolved warnings, active DLSS, RT/inline ray query support, and stable RT scene construction. |
| Ray tracing evidence | D3D12 and Vulkan logs report `supportsRT=true`, `inlineRayQuery=true`, `inlineShadowReady=true`, `referencedMeshes=103`, `builtBlas=103`, `candidateInstances=103`, `tlasInstances=103`, `missingGpuMeshData=0`, `rejectedBlas=0`, and `builtTlas=true`. |
| Camera-motion RT evidence | `artifacts/validation/stage20-camera-motion/stage20-camera-motion-results.json` records launcher-shaped direct execution for `editor-d3d12-camera`, `editor-vulkan-camera`, `runtime-d3d12-camera`, and `runtime-vulkan-camera`. Every run exited `0`, emitted `RHI smoke camera motion evidence`, reported `tlasValid=true`, and reported `frameGraphUnresolvedBarrierWarnings=0`; editor runs produced `d3d12-camera-lit.bmp` and `vulkan-camera-lit.bmp`. |
| PSO/runtime evidence | Smoke logs include explicit `PipelineRuntimeLibrary` key lines for D3D12 and Vulkan, including backend, package id, shader format, feature bits, binding layout, shader package generation/hash, render formats, depth format, raster/depth/blend state, and DirectLighting `inlineRayQuery|accelerationStructure` requirements. |
| Visual comparison evidence | `artifacts/validation/stage20/stage20-visual-comparison.json` compares D3D12/Vulkan BMP captures. Lit: 4578x1180, mean absolute byte difference `0.45850590702771543`. Normal: 4578x1180, mean absolute byte difference `0.5269850834129329`. |
| Artifact index | `artifacts/validation/stage20/stage20-smoke-results.json` records executable path, working directory, backend, view mode, frame limit, shader reload frame, capture path, logs, exit code, timeout state, and diagnostic matches for all six smoke runs. Logs, console captures, stderr captures, BMPs, shader validation log, and visual comparison JSON are stored beside it. |
| Format evidence | `clang_format_check` is not generated in the VS2026 build tree; the attempted target build failed with missing `clang_format_check.vcxproj`. |
| Data transfer contract | Stage 20 evidence transfers through command/build output, shader package validation logs, launcher-shaped smoke logs, deterministic camera-motion smoke logs, BMP capture artifacts, visual comparison JSON, PSO runtime key logs, backend capability reports, and this coverage status packet. |
| Threading readiness handoff | Validation inputs are deterministic process/environment packets and outputs are immutable logs/artifacts. Backend parity, package enumeration, PSO key construction, frame graph health, DLSS status, and RT scene diagnostics can be consumed by future queued validation without live editor state. |
| Remaining risk | Stage 19 backend service slimming is complete, and Stage 20's validation milestone is complete for the renderer/RHI parity evidence it owns. |

## Stage 34 Whole-Repository Evidence Gate Reconciliation

| Field | Evidence |
| --- | --- |
| Status | Accepted for the Stage 34 evidence gate. |
| Coverage reconciliation | Remaining detailed `Needs refactor` and `Needs design decision` rows are not unowned blockers. Backend service slimming, final threading handoff hardening, and stale-path/rubric cleanup are complete for the current Stage 19/35/36 track. |
| Boundary evidence | `architecture_boundary_check` passed during Stage 34 with only provider-owned NVIDIA DLSS Vulkan/Streamline counted exceptions. |
| Build evidence | `ShaderCompiler`, `SparkleLauncher`, `AssetCooker`, `TextureCooker`, and `ShowcaseRuntime` built in `DevelopmentEditor` from `build/windows-vs2026-stage5`. |
| Runtime evidence | Launcher-shaped D3D12 `ShowcaseRuntime` smoke from `Projects/Showcase` exited `0`, completed all `5/5` level switch targets, and reported `frameGraphUnresolvedBarrierWarnings=0`, DLSS active, RT available, and valid TLAS evidence. Stage 20 remains the full D3D12/Vulkan parity evidence packet. |
| Tool evidence | `ShaderCompiler.exe list-shaders --validate` reported `17` valid typed registrations across `10` packages; `ShaderCompiler.exe inspect-shader Sky` reported the `Sky` package and `SkyCS` compute entry. |

## Stage 36 Rendering Gate Reconciliation

| Field | Evidence |
| --- | --- |
| Status | Fully completed. Stage 36 reconciled stale accepted renderer/tooling rows and reran targeted validation after Stage 19 backend service closure. |
| Cleanup scan | Source/CMake scans found no retired renderer shader aggregator, pass-traits, shader-registration boilerplate path, one-field lighting wrapper, or old snapshot accessor path. Current `SparkleRendererShaderRegistrations` target names remain valid because they are the narrow renderer shader-registration object target consumed by runtime and ShaderCompiler. |
| Boundary evidence | `architecture_boundary_check` passed in `build-vs2026` with no RHI-to-Renderer include violation, no Application backend-native validation violation, and only counted provider-owned NVIDIA DLSS Vulkan/Streamline exceptions. |
| Shader evidence | `ShaderCompiler.exe list-shaders --validate` reported `17` valid typed registrations and `10` packages. |
| Remaining blocker | The Stage 19 backend-service blocker is closed. Ray tracing, interop, capture, diagnostics, presentation, pipeline, D3D12 descriptor state, and D3D12/Vulkan resource/memory ownership now have backend-owned service evidence. |
| Disposition | Rerun Stage 36 final cleanup, rubric scoring, stale-path scans, and whole-repository validation before marking the repository final review-ready. |
