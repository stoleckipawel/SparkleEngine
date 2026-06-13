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
| `Engine/Renderer` | 241 | Mapped to Renderer private/public/module-support rows, including renderer-owned shader registrations. |
| `Engine/RHI` | 190 | Mapped to RHI public/common/D3D12/Vulkan/module-support rows. |
| `Engine/Application/Private/Validation` | 3 | Mapped to supporting validation rows. |
| `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke` | 2 | Mapped to supporting validation rows. |
| `Tools/Shaders/ShaderCompiler` | 111 | Mapped to shader compiler and cooking rows. |
| Total tracked files | 547 | 547 mapped. |
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
| `Engine/RHI/Private/D3D12/{Commands,Descriptors,Device,Diagnostics,Memory,Pipeline,Resources,Samplers,SwapChain,ThirdParty,UI}/**` | Matching D3D12 Backend Coverage row. |
| `Engine/RHI/Private/D3D12/Textures/**` | D3D12 `Resources` / `Textures`. |
| `Engine/RHI/Private/Vulkan/{VulkanIncludes.h,VulkanPCH.h,VulkanRenderHardwareInterface.*,VulkanTypeConversions.*}` | Vulkan root facade, includes, and type conversions. |
| `Engine/RHI/Private/Vulkan/{Commands,Core,Descriptors,Device,Diagnostics,Memory,Pipeline,Resources,Samplers,SwapChain,UI}/**` | Matching Vulkan Backend Coverage row. |
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
- Stage 30 revisits every row here for threading-readiness: mutable owner, phase, handoff shape, isolation, ordering/synchronization expectation, diagnostics identity, and deterministic output.

## Module Build And Support Coverage

These rows were added during the file-level confrontation because module build/support files are real architecture files even when they are not runtime subsystems.

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Engine/Renderer/CMakeLists.txt` | Needs refactor | Renderer module build contract | Build dependencies can hide architecture cycles and make feature ownership unclear. Stage 9 moved NVIDIA DLSS Streamline/Vulkan linkage to `SparkleRendererNvidiaDlssProvider` so common `SparkleRenderer` no longer owns Vulkan policy. | Stage 3, Stage 4, Stage 9, Stage 22 | CMake configure and `SparkleLauncher` build after dependency changes. | Renderer target links only the layers it should own, provider-native linkage is narrow and documented, and dependency intent is documented. |
| `Engine/Renderer/Private/PCH.h` | Accepted | Renderer private module support | PCH can hide include debt if later refactors are not checked. | Preserve through Stage 22 | Include review during final cleanup. | PCH remains a compile-speed helper only and does not become an architecture dependency shortcut. |
| `Engine/Renderer/ShaderRegistrations` | Needs refactor | Renderer shader registration ownership | Renderer pass registration moved above RHI in Stage 4, but package declarations still duplicate pass runtime descriptions. | Stage 4, Stage 17, Stage 22 | `ShaderCompiler` package enumeration. | Ordinary renderer shader packages are owned above RHI and final pass definition work removes unnecessary duplication. |
| `Engine/RHI/CMakeLists.txt` | Needs refactor | RHI module build contract | Backend-specific dependencies can leak into common RHI or renderer-facing code. | Stage 3, Stage 4, Stage 19, Stage 22 | CMake configure and D3D12/Vulkan runtime builds after dependency changes. | Common RHI, D3D12, Vulkan, shader runtime, and optional SDK dependencies are separated intentionally. |
| `Engine/RHI/Private/PCH.h` | Accepted | RHI private module support | PCH can hide backend include coupling. | Preserve through Stage 22 | Include review during final cleanup. | PCH remains common support and does not include backend policy. |
| `Engine/RHI/Public/RHIAPI.h` | Accepted | RHI public ABI/export boundary | Export macro policy is small but must remain the only public module linkage helper. | Preserve through Stage 22 | Header include/build check. | Public API macro remains isolated and has no renderer/backend policy. |

## Renderer Private Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Renderer.cpp` root orchestration | Needs refactor | Renderer facade / system root | Too many lifecycle, frame, feature, diagnostics, and presentation responsibilities in one class. | Stage 11, Stage 12, Stage 15 | `SparkleLauncher` build and D3D12/Vulkan smoke after facade split. | `Renderer` is a thin host facade; frame execution can be diagrammed without reading `Renderer.cpp`. |
| `Camera` | Needs design decision | Renderer frame data | Camera/depth/handedness conventions can drift between D3D12 and Vulkan. | Question: what is the single documented projection, depth, viewport-Y, winding, and jitter convention? Stage 2, Stage 20 | Lit and normal/debug captures for both backends. | Camera/depth convention doc exists and D3D12/Vulkan captures match within agreed tolerance. |
| `Commands` | Needs design decision | Renderer command context / RHI command list boundary | Ownership between renderer command intent and RHI GPU commands is not yet crisp. | Question: which commands belong to `RenderCommandContext` versus `RenderCommandList` services? Stage 2, Stage 6, Stage 19 | RHI contract map and command caller map. | Renderer command context expresses render intent; RHI command list exposes GPU/API operations only. |
| `Debug` | Needs refactor | Renderer diagnostics/debug policy | Debug CVars are tiny but not yet tied to reviewer docs or smoke evidence. | Stage 21, Stage 22 | README/debug-view documentation check. | Debug CVars and view modes are documented and stable in smoke validation. |
| `Denoising` | Needs design decision | Renderer feature system | Private folder is empty while a public denoising contract exists. | Question: should denoising be implemented now, removed, or documented as future-owned by ray tracing/upscaling? Stage 13, Stage 22 | Coverage status update and docs decision. | Empty placeholder removed or documented with a concrete owner and integration point. |
| `Diagnostics` | Needs refactor | Renderer diagnostics | Diagnostics exist but are not yet the central acceptance evidence path. | Stage 8, Stage 10, Stage 14, Stage 20 | Smoke log includes diagnostics capability/status. | Smoke report includes renderer memory, frame graph, GPU marker/timing, mesh, texture, DLSS, and RT status. |
| `Frame` | Needs refactor | Renderer frame pipeline | Frame orchestration and pass setup naming are not self-documenting enough. | Stage 11, Stage 14, Stage 15 | Frame pipeline build and smoke. | `Frame/*` wires graph resources/passes only; pass execution lives in `Passes`/`Pipeline`. |
| `FrameGraph` | Needs refactor | Renderer render graph system | Unresolved resource/barrier warnings can remain warnings instead of contract failures. | Stage 14, Stage 15, Stage 20 | Frame graph diagnostic smoke output. | Development smoke fails unresolved resources/barriers and emits actionable graph diagnostics. |
| `Meshes` | Needs refactor | Renderer feature system / scene snapshot input | Boundary with scene snapshots and material cache needs clearer ownership. | Stage 13, Stage 20 | Mesh diagnostics snapshot in smoke/report. | Mesh cache accepts render-domain DTOs and documents upload/lifetime/skinning data flow. |
| `Passes` | Needs refactor | Renderer pass system | Ordinary pass authoring is too ceremonial and tied to central traits/runtime mechanics. | Stage 17, Stage 20 | Proof pass migration build. | Simple compute/raster pass can be added without RHI edits or central trait edits. |
| `Pipeline` | Needs refactor | Renderer pipeline runtime | PSO/runtime identity is implicit and mixes package loading, validation, layout, and PSO creation. | Stage 16, Stage 17, Stage 20 | Pipeline runtime logs include package/key/backend. | `PipelineRuntimeLibrary` owns explicit PSO keys and D3D12/Vulkan normalized descriptors. |
| `RayTracing` | Needs refactor | Renderer ray tracing feature system | Names blur scene AS ownership, pass services, and shadow-specific shader data. | Stage 18, Stage 20 | RT capability/TLAS/shadow smoke report. | Ray tracing contract explains BLAS/TLAS lifetime, frame graph AS import, pass services, and shadow uniform ownership. |
| `SceneData` | Needs refactor | Renderer scene bridge | Renderer may couple to gameplay internals instead of immutable render snapshots. | Stage 13, Stage 20 | Scene snapshot and mesh/material diagnostic output. | Renderer consumes immutable render-domain DTOs for meshes, materials, lights, cameras, skinning, and instances. |
| `Temporal` | Needs design decision | Renderer frame data / upscaling input | Jitter/reset/motion-vector behavior affects DLSS and debug parity but is not formally owned. | Question: what owns jitter convention, reset policy, and native-resolution/upscaler interaction? Stage 13, Stage 20 | Temporal settings in debug/smoke notes. | Temporal convention is documented and validated with lit/debug captures. |
| `Textures` | Needs refactor | Renderer texture feature system | Texture lifetime/fallback diagnostics need to be tied to RHI resource ownership. | Stage 13, Stage 20 | Texture diagnostics snapshot. | Texture manager contract covers cooked input, defaults, lifetime, residency, and diagnostics. |
| `Upscaling` | Needs refactor | Renderer upscaling feature system | Native interop is sensitive and provider/backend ownership must be explicit. Stage 9 added `upscaler-provider-contract.md`, provider-target isolation, and structured failure domains. | Stage 9, Stage 10, Stage 20 | DLSS capability/provider smoke log. | Provider owns SDK details; RHI owns native metadata; fallback reasons are deterministic and classified by domain. |

## Renderer Public Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Renderer.h` / `RendererAPI.h` | Needs refactor | Renderer facade | Public API exposes too much internal renderer/RHI/presentation surface. | Stage 11, Stage 12, Stage 15 | Application/editor build after host API migration. | Public renderer API reads as host protocol plus diagnostics, not internal subsystem access. |
| `Debug` | Needs refactor | Renderer public debug contract | Debug view modes are key evidence but not yet fully validated across APIs. | Stage 10, Stage 20, Stage 21 | D3D12/Vulkan debug/normal captures. | View modes work in both backends and are documented in reviewer path. |
| `Denoising` | Needs design decision | Renderer public feature contract | Public contract exists without private implementation ownership. | Question: is denoising a current feature, future ray tracing feature, or contract to remove? Stage 13, Stage 22 | Design decision note. | Public denoising contract is either implemented, documented as future-owned, or removed. |
| `Diagnostics` | Needs refactor | Renderer public diagnostics | Memory diagnostics exist but are not yet tied to final smoke artifacts. | Stage 10, Stage 20 | Memory diagnostics in smoke/report. | Renderer and RHI memory diagnostics appear in final validation evidence. |
| `FrameGraph` | Needs refactor | Renderer public graph handles/descs | Public handles/descs must stay stable without exposing compiler/execution internals. | Stage 14, Stage 20 | Frame graph contract doc and smoke. | Public frame graph types are stable handles/descs only; internals remain private. |
| `Meshes` | Needs refactor | Renderer public diagnostics | Mesh diagnostics are narrow but need reviewer-visible schema/evidence. | Stage 13, Stage 21 | Mesh diagnostics snapshot. | Mesh diagnostic schema is documented and used by smoke/tools. |
| `Resources` | Needs refactor | Renderer public resource diagnostics | Resource diagnostics must not leak backend objects. | Stage 13, Stage 21 | Texture/resource diagnostics snapshot. | Resource diagnostics expose health without backend-native handles. |
| `SceneData` | Needs refactor | Renderer public DTO contract | Public scene DTOs should remain render-domain only. | Stage 13, Stage 20 | Scene data contract doc. | DTOs are immutable frame inputs and do not expose gameplay internals. |
| `ShaderParameters` | Needs design decision | Renderer/RHI/neutral shader authoring boundary | Large public surface overlaps RHI shader parameter concepts. | Question: should shader parameter authoring live in Renderer, RHI, or a neutral shader authoring module? Stage 4, Stage 17 | Pass authoring contract decision. | One owner exists for shader-visible parameter authoring with no circular dependency. |
| `Shaders` | Needs refactor | Renderer shader reload/runtime evidence | Reload result is small but tied to runtime invalidation evidence. | Stage 16, Stage 17, Stage 20 | Shader reload smoke log. | Reload result reports affected packages/runtimes and backend invalidation behavior. |
| `Viewport` | Needs refactor | Renderer presentation bridge | Editor presentation can leak resource transitions and frame graph handles. | Stage 12, Stage 15 | Editor viewport smoke. | Editor receives viewport products through presentation contract, not ad hoc transitions. |

## RHI Public Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Device` | Needs refactor | RHI facade / service access | Root facade is broad and backend implementations are large, although Stage 7 added first service accessors. | Stage 6, Stage 7, Stage 19 | RHI method ownership table and Stage 7 service contracts. | Every method has an owner service; facade is slimmed behind service responsibilities. |
| `Commands` | Needs design decision | RHI command service | Command-list surface must remain GPU/API level and not absorb renderer pass policy. | Question: which commands stay root command-list operations versus service helpers? Stage 6, Stage 19 | Command contract map. | Command list exposes GPU/API operations with no renderer concepts. |
| `Resources` | Needs refactor | RHI resource service | Resource descriptors/state/view rules must map cleanly to both APIs. | Stage 6, Stage 7, Stage 19 | Resource contract doc. | Resource descriptors, state/layout, view, and subresource rules are documented and parity-tested. |
| `Pipeline` | Needs refactor | RHI pipeline service | Pipeline desc must align with explicit PSO key/runtime model. | Stage 16, Stage 19, Stage 20 | PSO key/runtime logs. | Pipeline desc is normalized and sufficient for D3D12/Vulkan PSO creation. |
| `Shaders` | Needs refactor | RHI shader package/runtime primitives | Stage 4 moved renderer pass registration above RHI; the remaining risk is generic shader package/runtime primitives absorbing pass policy. | Stage 4, Stage 16, Stage 17 | ShaderCompiler build and package enumeration. | RHI shader public types are generic package/reflection/runtime primitives only. |
| `ShaderParameters` | Needs design decision | RHI or neutral shader authoring boundary | Ownership overlaps Renderer public shader parameters. | Question: what is the lower shared owner for parameter primitives and what stays pass-specific? Stage 4, Stage 17 | Shader authoring design note. | No circular dependency; pass-specific parameters stay above RHI. |
| `Bindings` | Needs refactor | RHI binding layout service | Binding sets need validation against reflection/layout. | Stage 16, Stage 19 | Binding layout validation logs. | Binding sets are backend-neutral and validated against reflection/layout. |
| `Descriptors` | Needs refactor | RHI descriptor service | Handle lifetime and shader-visible versus CPU-only policy needs documentation. | Stage 6, Stage 19 | Descriptor ownership table. | Descriptor lifetime and visibility policy are documented and backend-parity checked. |
| `Formats` | Needs refactor | RHI format contract | Format translation/support matrix must be explicit across APIs. | Stage 19, Stage 20 | Format support/parity notes. | Format support matrix covers color, depth/stencil, and sRGB rules for both APIs. |
| `Interop` | Needs refactor | RHI external interop service | Native interop can become a catch-all. Stage 7 introduced `RhiNativeDeviceQueueInterop` with consumer/reason metadata. | Stage 7, Stage 9, Stage 10 | DLSS/native interop service build path and later smoke logs. | Interop structs are consumer-scoped and filled deterministically by backends. |
| `Memory` | Needs refactor | RHI memory diagnostics/service | Memory categories must map to backend allocators and smoke reports. | Stage 7, Stage 19, Stage 20 | Memory diagnostics report. | Backend allocation stats map to common categories and final validation evidence. |
| `RayTracing` | Needs refactor | RHI ray tracing service | RHI RT structs must stay GPU/API-only. | Stage 18, Stage 20 | RT contract and TLAS smoke logs. | RHI RT descs expose AS geometry/build/prebuild/scratch/result concepts only. |
| `Samplers` | Needs refactor | RHI sampler service | Default sampler policy must match across backends. | Stage 19, Stage 20 | Sampler parity note. | Sampler desc maps equivalently to D3D12/Vulkan and default library policy is documented. |
| `Textures` | Needs design decision | RHI texture upload/runtime asset boundary | Cooked texture asset contract may blur asset/cook/runtime responsibilities. | Question: should cooked texture runtime upload contract remain in RHI or move to a lower asset/runtime module? Stage 13, Stage 19 | Texture contract decision. | Source import/cook remains in tools; runtime GPU upload contract has one owner. |
| `Diagnostics` | Needs refactor | RHI diagnostics service | Diagnostics must become final evidence, not optional logs. Stage 7 routes Renderer/Application diagnostics through `RhiDiagnosticsService`. | Stage 7, Stage 10, Stage 20 | Backend diagnostics service build path and later smoke report. | Debug layers, markers, names, errors, capability logs flow through RHI diagnostics. |
| `Validation` | Needs refactor | RHI validation service | Validation should grow into mechanical guardrails. | Stage 3, Stage 7, Stage 14, Stage 20 | Boundary check and runtime validation output. | Resource descs, RT descs, binding compatibility, and feature requirements are validated. |
| `Capture` / `Config` / `Core` / `CVars` / `UI` / `Presentation` | Needs refactor | RHI capture/config/backend selection/UI/presentation bridge | Config, capture, presentation, and UI bridge can grow into hidden coupling if not owned. Stage 7 introduced capture and presentation services; Stage 8/12 own deeper cleanup. | Stage 6, Stage 7, Stage 8, Stage 12, Stage 19 | Backend selection, capture service, and UI/presentation service build evidence. | Config/capability contracts are stable; capture/presentation/ImGui use RHI service bridges without renderer/application internals. |

## RHI Private Common Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Bindings` | Needs refactor | RHI binding implementation | Binding set creation must validate layout/reflection errors clearly. | Stage 16, Stage 19 | Binding validation log. | Binding creation reports actionable errors and matches pipeline layout. |
| `Config` | Needs design decision | RHI coordinate/depth convention | Depth convention is central to D3D12/Vulkan visual parity. | Question: what exact depth, clip, viewport, culling, and winding policy is canonical? Stage 2, Stage 20 | Convention doc and parity captures. | One convention is documented and validated across lit/normal/debug captures. |
| `Core` | Needs refactor | RHI backend selection | Backend selection should log selected API and fallback reasons. | Stage 6, Stage 20 | Backend selection log. | Backend selection is policy-only and logs backend/fallback/feature limits. |
| `CVars` | Needs refactor | RHI runtime config | RHI CVars should not be hidden architecture switches. | Stage 21, Stage 22 | README/config docs. | RHI CVars are documented or removed if not needed. |
| `Device` | Needs refactor | RHI backend factory/services | Device services are a good boundary but need method ownership alignment. | Stage 6, Stage 19 | RHI method ownership table. | Backend service creation is the only backend selection point and capability logs are consistent. |
| `Shaders` | Needs refactor | RHI shader infrastructure | Renderer registrations have moved above RHI; generic shader infrastructure still needs package/layout/runtime ownership tightening. | Stage 4, Stage 16, Stage 17 | ShaderCompiler build/package enumeration. | Generic shader infrastructure remains in RHI while renderer registrations stay above RHI. |
| `Validation` | Needs refactor | RHI validation implementation | Validation must become mandatory for development paths. | Stage 3, Stage 14, Stage 20 | Validation and smoke logs. | Development paths validate resources, bindings, RT metadata, and unsupported features. |

## D3D12 Backend Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| Root facade and type conversions | Needs refactor | D3D12 backend services | Root facade mirrors broad RHI and hides service ownership. | Stage 19, Stage 20 | D3D12 build and service map. | Root facade mostly wires service objects; type conversions are total and tested for public enums/descs. |
| `Commands` | Needs refactor | D3D12 command service | Command behavior must match RHI semantics and Vulkan parity. | Stage 19, Stage 20 | D3D12 smoke with barrier/draw/dispatch/AS logs. | Commands match RHI semantics for barriers, draw/dispatch, copies, AS builds, and debug markers. |
| `Descriptors` | Needs refactor | D3D12 descriptor service | Descriptor lifetime/recycling/failure policy needs documentation. | Stage 19, Stage 20 | Descriptor diagnostics. | CPU/GPU descriptor heap policy and recycling are documented and validated. |
| `Diagnostics` | Needs refactor | D3D12 diagnostics service | Diagnostics are strong but must appear in final evidence. | Stage 10, Stage 20 | D3D12 diagnostic smoke output. | Debug names/events/errors appear in captures and smoke reports. |
| `Memory` | Needs refactor | D3D12 memory service | Allocation strategy and transient interaction need review evidence. | Stage 19, Stage 20 | D3D12 memory diagnostics. | Heap type mapping, alignment, budgets, and transient interaction are documented. |
| `Pipeline` | Needs refactor | D3D12 pipeline service | Backend should consume normalized pipeline desc/PSO keys without renderer policy. | Stage 16, Stage 19, Stage 20 | D3D12 PSO key logs. | Root signature/PSO creation is deterministic from common descriptors/reflection. |
| `Resources` / `Textures` | Needs refactor | D3D12 resource service | Constants/upload/resource lifetime should move out of root facade pressure. | Stage 7, Stage 19, Stage 20 | D3D12 resource/capture smoke. | Resource lifetime, state assumptions, upload path, and view creation are documented and validated. |
| `Samplers` | Needs refactor | D3D12 sampler service | Default sampler set must match Vulkan where possible. | Stage 19, Stage 20 | Sampler parity note. | Default sampler behavior is common or documented as API-specific. |
| `SwapChain` | Needs refactor | D3D12 presentation service | Presentation states/resize/back-buffer lifetime need explicit contract. | Stage 12, Stage 19, Stage 20 | D3D12 resize/presentation smoke. | Presentation states, resize, back-buffer lifetime, and viewport integration are explicit. |
| `ThirdParty` | Accepted | D3D12 backend third-party isolation | Third-party utility must remain isolated and unmodified for engine policy. | Preserve through Stage 19, Stage 22 | Boundary/source grouping check. | D3DX12 code remains isolated in `ThirdParty` and is not edited for Sparkle policy. |
| `UI` | Needs refactor | D3D12 UI bridge | ImGui integration must not leak D3D12 objects upward. | Stage 7, Stage 12, Stage 19 | D3D12 editor UI smoke. | ImGui path uses RHI UI bridge without D3D12 objects in Application/Renderer policy. |

## Vulkan Backend Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| Root facade, includes, type conversions | Needs refactor | Vulkan backend services | Root facade mirrors broad RHI; feature/extension setup is critical for DLSS/RT. | Stage 19, Stage 20 | Vulkan build and service map. | Root facade mostly wires services; feature/extension logs explain optional capabilities. |
| `Commands` | Needs refactor | Vulkan command service | Layouts, barriers, descriptors, dynamic rendering, and AS builds are high-risk parity points. | Stage 19, Stage 20 | Vulkan lit/normal/debug smoke logs. | Vulkan commands match RHI semantics and D3D12 captures with diagnosable layouts. |
| `Core` | Needs refactor | Vulkan core utilities | Result handling should report context for every failed call. | Stage 19, Stage 20 | Vulkan validation/error log. | Failed Vulkan calls include context and result string. |
| `Descriptors` | Needs refactor | Vulkan descriptor service | Descriptor lifetime/update policy likely affects DLSS/noise/resource binding bugs. | Stage 19, Stage 20 | Vulkan descriptor diagnostics. | Descriptor set/layout lifetime, pool growth, update validation, and future bindless policy are documented. |
| `Diagnostics` | Needs refactor | Vulkan diagnostics service | Validation warnings must be zero or classified. Stage 10 found runtime present-layout validation errors while editor capture smoke stayed clean. | Stage 10, Stage 12, Stage 19, Stage 20 | Vulkan validation layer smoke output. | Validation warnings are zero in smoke or documented as known exceptions. |
| `Memory` | Needs refactor | Vulkan memory service | Memory type selection/external memory requirements need explicit docs. | Stage 19, Stage 20 | Vulkan memory diagnostics. | Memory type selection, alignment, budgets, transient compatibility, and external memory requirements are documented. |
| `Pipeline` | Needs refactor | Vulkan pipeline service | Winding/cull/depth/viewport mapping is a critical parity point. | Stage 16, Stage 19, Stage 20 | Vulkan PSO key and visual parity logs. | Pipeline layout/PSO creation consumes normalized descs and explicitly maps conventions. |
| `Resources` / `Textures` | Needs refactor | Vulkan resource service | Image/view/layout/subresource and native view info need formal ownership. | Stage 7, Stage 9, Stage 19, Stage 20 | Vulkan capture/DLSS resource logs. | Image/view/layout/subresource ownership is documented; native view info is deterministic. |
| `Samplers` | Needs refactor | Vulkan sampler service | Default sampler behavior must match common desc policy. | Stage 19, Stage 20 | Sampler parity note. | Vulkan sampler library matches common desc behavior or documents differences. |
| `SwapChain` | Needs refactor | Vulkan presentation service | Present layout, image acquisition, and frame graph import must be explicit. | Stage 12, Stage 19, Stage 20 | Vulkan resize/presentation smoke. | Present layout, resize, acquisition, and back-buffer import into frame graph are documented. |
| `UI` | Needs refactor | Vulkan UI bridge | ImGui descriptor/image state handling must stay isolated. | Stage 7, Stage 12, Stage 19 | Vulkan editor UI smoke. | ImGui descriptor/image state handling does not leak into Renderer/Application policy. |

## Supporting Validation Surfaces

These rows are not part of the Renderer/RHI folder audit, but Stage 1 includes them because later acceptance depends on validation evidence.

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp` | Needs refactor | Application smoke orchestration / RHI capture service | Application currently owns D3D12-native capture code. | Stage 8, Stage 10 | Include scan and D3D12/Vulkan capture smoke. | Application orchestrates smoke only; backend-native capture lives in RHI backend/service. |
| `Engine/Application/Private/Validation/RhiSmokeValidation.*` | Needs refactor | Runtime smoke orchestration | Runtime smoke now records backend parity artifacts; Stage 10 exposed Vulkan runtime presentation-layout validation errors that must not be hidden by exit code success. | Stage 8, Stage 10, Stage 12, Stage 19, Stage 20 | Runtime smoke logs. | Runtime smoke produces backend, frame, feature, and failure evidence with zero unowned validation-layer errors. |
| `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke` | Needs refactor | Launcher validation workflow | Launcher smoke inputs do not yet expose all final evidence knobs. | Stage 8, Stage 10, Stage 21 | Launcher smoke command/environment report. | Launcher can run documented smoke workflows for backend, view mode, frame count, and capture artifacts. |

## Shader Compiler And Cooking Coverage

These rows were added during the file-level confrontation. They sit outside `Engine/Renderer` and `Engine/RHI`, but the review-ready architecture cannot prove pass authoring, PSO handling, shader reflection, or renderer/RHI separation without them.

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Tools/Shaders/ShaderCompiler/CMakeLists.txt` and `Source/main.cpp` | Needs refactor | Shader compiler CLI/build entry | CLI/build options may not expose the evidence needed by pass authoring and runtime package validation. | Stage 4, Stage 17, Stage 21 | `ShaderCompiler` build and command listing. | CLI can compile/list/inspect packages needed by renderer pass authoring without RHI-specific pass edits. |
| `Backends/Dxc` | Needs refactor | Shader compiler backend adapter | DXIL/SPIR-V compilation and reflection details can leak into common shader package contracts. | Stage 4, Stage 16, Stage 17 | DXC backend compile plus reflection inspection. | DXC backend emits the same normalized reflection/package schema as other backends. |
| `Backends/Slang` | Needs refactor | Shader compiler backend adapter | Slang path may drift from DXC package/reflection output. | Stage 4, Stage 16, Stage 17 | Slang backend compile plus reflection inspection. | Slang backend is either parity-compatible with the normalized schema or explicitly documented as unsupported for a target. |
| `Private/Backend` | Needs refactor | Shader backend registry/factory | Backend selection can become tool-specific policy instead of target capability selection. | Stage 4, Stage 17 | Backend list/target command output. | Backend factory reports capabilities and target support clearly. |
| `Private/Compiler` | Needs refactor | Shader source compiler front end | Include resolution, preprocessing, and source identity must be deterministic for incremental cooking. | Stage 4, Stage 17 | Repeated cook produces stable package/cache keys. | Compiler front end has deterministic include/source identity behavior. |
| `Private/Cooking` | Needs refactor | Shader cook graph/package pipeline | Cooking is the bridge between high-level pass authoring and runtime PSO creation; hidden coupling here blocks simple pass additions. | Stage 4, Stage 16, Stage 17 | Cook plan/package inspection output. | A new renderer pass is cooked into packages/reflection without adding renderer-specific code to RHI. |
| `Private/Verification` | Needs refactor | Shader contract verifier | Parameter layout errors can otherwise surface as backend runtime bugs. | Stage 4, Stage 17, Stage 20 | Verification failure fixture or command output. | Shader parameter structs are checked against reflection before runtime use. |
| `Private/Cli` | Needs refactor | Shader tooling UX | Reviewers need discoverable commands to reproduce shader/package evidence. | Stage 4, Stage 21 | `ShaderCompiler` help/list/inspect command output. | CLI exposes compile, list, inspect, and failure evidence for final review. |
| `Private/Analysis` and `Private/Inspection` | Needs refactor | Shader package diagnostics | Package stats and inspection are useful but not yet required acceptance artifacts. | Stage 4, Stage 16, Stage 21 | Package stats/inspection output. | Shader package diagnostics are part of final reviewer evidence. |
| `Private/Constants` and private root support | Accepted | Shader compiler private support | Constants/PCH/types should remain support code, not hidden ownership policy. | Preserve through Stage 22 | Include/build check. | Support files remain small and do not own shader pass, RHI, or backend policy. |

## Open Stage 1 Decisions

| Decision | Why it matters | Owning stage |
| --- | --- | --- |
| Canonical camera/depth/viewport/winding convention | Prevents D3D12/Vulkan visual inversion, culling, and lighting parity bugs. | Stage 2, Stage 20 |
| `RenderCommandContext` versus `RenderCommandList` ownership | Prevents renderer intent from blending with GPU/API command contracts. | Stage 2, Stage 6, Stage 19 |
| Denoising ownership | Avoids a public feature contract with no private owner. | Stage 13, Stage 22 |
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
| Threading readiness handoff | Rows keep future threading-sensitive owners visible: frame orchestration, command recording, frame graph, resources, shader cooking, launcher smoke orchestration, and backend services feed Stage 30's immutable handoff and mutable-owner audit. |
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
| Status | Almost finished. Backend/editor capture and DLSS evidence pass; runtime Vulkan validation-layer presentation-layout errors block full acceptance. |
| Build evidence | `SparkleLauncher`, `ShaderCompiler`, `ShowcaseEditor`, `ShowcaseRuntime`, and `architecture_boundary_check` built or ran successfully in `build/windows-vs2026-stage5` with `DevelopmentEditor`. `ShaderCompiler` passed after rerunning alone following a Visual Studio `ZERO_CHECK.lastbuildstate` lock from parallel build activity. |
| Format evidence | `clang_format_check` is not generated in this build tree, so formatting validation could not run as a target. |
| Boundary evidence | Direct CMake boundary check passed with only provider-owned counted exceptions for `SparkleRendererNvidiaDlssProvider` Vulkan linkage and `StreamlineDlssRuntime.cpp` Vulkan identifiers. |
| Application ownership evidence | Native backend scan over `Engine/Application/Private/Validation` returned no D3D12/Vulkan header, type, API identifier, or backend-folder matches. |
| Editor D3D12 evidence | `artifacts/validation/stage10/d3d12-lit.log` and `d3d12-normal.log` exited `0`, report `frameGraphUnresolvedBarrierWarnings=0`, DLSS active with `failureDomain=None`, ray tracing and inline ray query support, and captured frame 30 to `d3d12-lit.bmp` and `d3d12-normal.bmp`. |
| Editor Vulkan evidence | `artifacts/validation/stage10/vulkan-lit.log` and `vulkan-normal.log` exited `0`, report `frameGraphUnresolvedBarrierWarnings=0`, DLSS active with `failureDomain=None`, ray tracing and inline ray query support, and captured frame 30 to `vulkan-lit.bmp` and `vulkan-normal.bmp`. |
| Capture artifacts | `d3d12-lit.bmp`, `d3d12-normal.bmp`, `vulkan-lit.bmp`, and `vulkan-normal.bmp` are 32-bit BMP files with `BM` signatures and 21,608,214-byte payloads. |
| Runtime D3D12 evidence | `artifacts/validation/stage10/d3d12-runtime.log` exited `0`, reports `frameGraphUnresolvedBarrierWarnings=0`, DLSS active with `failureDomain=None`, ray tracing and inline ray query support, and no error/critical lines. |
| Runtime Vulkan evidence | `artifacts/validation/stage10/vulkan-runtime.log` exited `0` and reports `frameGraphUnresolvedBarrierWarnings=0`, DLSS active with `failureDomain=None`, ray tracing and inline ray query support, but contains validation-layer errors at `VulkanRhi.cpp:750` because submitted swapchain images are expected in `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` while current layout is `VK_IMAGE_LAYOUT_UNDEFINED`. |
| Artifact index | Editor matrix: `artifacts/validation/stage10/stage10-smoke-results.json`. Runtime matrix: `artifacts/validation/stage10/stage10-runtime-results.json`. Logs and console captures are stored beside the BMP artifacts with backend/view/runtime names. |
| Data transfer contract | Evidence flows through smoke logs, BMP artifacts, JSON result summaries, backend capability reports, and this status table. Capture and DLSS state are structured log data, not visual inference. |
| Threading readiness handoff | Smoke requests are environment/process packets and outputs are immutable logs/artifacts, which keeps future launcher orchestration and backend smoke execution compatible with queued jobs or worker-owned validation runs. |
| Remaining risk | Runtime Vulkan present-layout ownership must be fixed or formally scoped before Stage 10 can be marked fully completed. Candidate owner stages are Stage 12 presentation bridge, Stage 19 backend service symmetry, or Stage 20 full backend parity. |
