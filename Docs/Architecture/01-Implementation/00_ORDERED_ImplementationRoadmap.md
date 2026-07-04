# 00. ORDERED - Implementation Roadmap

Status: master implementation sequence
Date: 2026-07-04
Source set: `Docs/Architecture/00-Review` and `Docs/Architecture/01-Implementation`
Use this as: the first document to open before doing engine work

## Purpose

This document orders the KEEP, MODIFY, ADD, and REMOVE documents into a staged implementation plan you can follow over many sessions or weeks.

The four supporting documents answer different questions:

- `01_KEEP_PreservedCapabilities.md`: what must survive every change
- `02_MODIFY_RefactorExistingSystems.md`: valuable systems to refactor and harden
- `03_ADD_MinimalMissingCapabilities.md`: narrowly allowed additions
- `04_REMOVE_DeletionsAndCleanup.md`: deletion queue and cleanup prompts

This roadmap tells you when to use each one.

The outcome should be a smaller, sharper renderer-first engine that demonstrates:

- explicit D3D12/Vulkan ownership and parity
- strong RHI and frame-graph architecture
- proper Core, RHI, Renderer, GameFramework, and Tools separation
- thin high-level concepts that keep users away from low-level implementation details without hiding rendering behavior behind excessive abstraction
- classic TLAS and PTLAS as product RT features
- shader compiler/cook/runtime ABI strength
- screenshot/BMP capture as a hardened editor/tool capability
- multi-level project support without depot pollution
- renderer feature depth before profiling work
- professional debugging support
- fewer public observation APIs
- fewer diagnostics, reports, logs, wrappers, and future scaffolding

## How To Use These Documents

For every implementation batch:

1. Read `01_KEEP_PreservedCapabilities.md`.
2. Pick exactly one roadmap stage from this document.
3. Open the referenced section in `02_MODIFY_RefactorExistingSystems.md`, `03_ADD_MinimalMissingCapabilities.md`, or `04_REMOVE_DeletionsAndCleanup.md`.
4. Fill the batch prompt before editing.
5. Make the smallest meaningful code/content change.
6. Verify the acceptance criteria from the stage with targeted checks; defer full build/cook/run stabilization to Stage 42 unless you intentionally choose a checkpoint.
7. Update only existing docs if the implementation result makes current instructions stale.

Do not start by creating more plans. This is the plan.

## Batch Prompt Template

Copy this into your working notes before each batch:

```text
Batch:
Roadmap stage:
Persona pillar:
Capability preserved:
Capability improved:
Primary source docs (KEEP/MODIFY/ADD/REMOVE):
Files/paths to inspect:
Search patterns:
Existing capability search:
Similar responsibility found:
Expected removals:
Allowed additions:
Code delta target:
D3D12 impact:
Vulkan impact:
Shader/cook impact:
Runtime impact:
Public/private API impact:
Content impact:
Hardcoded content/name check:
Fallback policy:
Tooling/capture impact:
No-pollution check:
Verification plan:
Full build/cook/run deferred to:
Done criteria:
```

No-pollution check means:

- no new documentation files unless the user explicitly asks
- no new logs
- no new validation systems
- no new diagnostic panels
- no new report formats
- no new wrapper layers
- no thick abstraction layers
- no future-feature scaffolding
- no uncataloged heavy content
- no duplicate responsibility when an existing subsystem already owns the behavior
- no real-code hardcoding of project, level, asset, optional-pack, or sample names
- no fallback chains that hide missing required data

Universal engineering gate means:

- Search first: before adding code, prove the repo does not already have an owner/capability that should be reused, merged, or simplified.
- Net code pressure: every code addition should be paired with deletion or simplification in the same batch. Treat this as applying to completed and future stages; if a batch cannot reach net-zero or net-negative code, record the exact reason and the next removal target before the batch is considered done.
- Single ownership: similar capabilities must converge on one owner instead of creating parallel paths.
- Data owns content names: catalogs, config, level files, and project data may name content; compiled engine/tool code should not hardcode sample or asset names.
- Fail simply: required data should fail clearly at the owning boundary; optional data should use explicit availability metadata rather than layered fallbacks.

## Renderer-First Module Contract

Sparkle is a compact renderer-first engine. Runtime, editor, content, and tools exist to prove explicit graphics API ownership, shader pipeline quality, cooked content workflows, and advanced rendering features. They should not grow into separate products unless they remove more code or make a preserved capability clearer.

Thin high-level concepts are allowed: project, level, scene, render path, backend, capture, cook, package. These concepts should let users operate the engine without touching low-level implementation details. They must stay thin enough that rendering behavior remains traceable from scene data to renderer pass to frame graph to RHI backend.

| Area | Owns | Does Not Own |
| --- | --- | --- |
| Core | foundation utilities, math, files, config, fatal checks, small platform-neutral helpers | renderer policy, backend resources, workflow UI, content schemas beyond common utilities |
| RHI | explicit D3D12/Vulkan resources, descriptors, pipelines, queues, command lists, barriers, uploads/readbacks, ray tracing, native interop, capture, presentation | scene extraction, material policy, frame scheduling, editor panels, generic wrapper layers |
| Renderer | frame graph, passes, render paths, scene extraction, shader registrations, provider boundaries, TLAS/PTLAS policy, render products | backend-private API details, level ownership, launcher/cooker workflows |
| GameFramework | project runtime concepts, levels, scenes, components, cameras, materials, meshes, lights, cooked scene loading | renderer-private implementation details, RHI resources, shader package compilation |
| Tools | shader compile, import, cook, launch, clean, package-if-owned, asset workflow entrypoints | runtime render ownership, diagnostic cockpit behavior, unowned package products |
| Projects | sample projects, selectable levels, scene/content data, optional heavy content packs | engine contracts, backend-specific behavior, required heavy content in the default repo footprint |

## Phase Overview

| Phase | Theme | Primary doc | Result |
| ---: | --- | --- | --- |
| 0 | Baseline and navigation | This doc + KEEP | You know what must not break. |
| 1 | Repository/doc hygiene | REMOVE category 10 | Review set is navigable and neutral. |
| 2 | Content catalog and optional heavy packs | MODIFY 1 + ADD 1 + REMOVE 1 | Multi-level support remains; depot weight can drop. |
| 3 | Screenshot capture hardening | MODIFY 2 + ADD 3 + REMOVE 2 | Capture preserved; smoke/ad hoc capture removed. |
| 4 | Smoke/report/cook artifact cleanup | REMOVE 2 + REMOVE 3 | Default workflows produce assets, not reports. |
| 5 | Launcher slimming | MODIFY 3 + REMOVE 7 | Launcher becomes workflow shell. |
| 6 | Public observation API narrowing | MODIFY 4 + REMOVE 4 | Public API shrinks around behavior. |
| 7 | TLAS/PTLAS refactor | MODIFY 5 + ADD 2 + REMOVE 5 | Classic TLAS and PTLAS both work; PTLAS is smaller. |
| 8 | Reference path tracing cleanup | MODIFY 6 + REMOVE 8 | Reference mode has one clear role. |
| 9 | Shader debug and duplication cleanup | MODIFY 7 + REMOVE 6 | Shader ABI stays strong; default debug artifacts leave. |
| 10 | Core/GameFramework surface cleanup | MODIFY 8 | Public engine boundaries shrink. |
| 11 | Renderer feature hardening | MODIFY 9 + ADD 5 | RT/GI/post/denoise/upscale/framegraph/shaders/passes become cleaner. |
| 12 | Package contract cleanup | MODIFY 10 + ADD 6 + REMOVE 9 | Package outputs are real and intentional. |
| 13 | Late profiling and measurement | ADD late only + review docs | Measurement happens after feature cleanup. |
| 14 | Final readiness pass | All four docs | Persona and engine goals are demonstrably covered. |

## Reference Repository Map

Use these repositories as implementation references. Do not copy architecture blindly. Use them to compare ownership, directory shape, feature boundaries, and how much code is needed for a mature implementation.

| Ref | Repository | Use For |
| --- | --- | --- |
| NV-DONUT | https://github.com/NVIDIA-RTX/Donut | renderer/app/core split, sample framework boundaries |
| NV-DONUT-SAMPLES | https://github.com/NVIDIA-RTX/Donut-Samples | keeping samples separate from framework code |
| NV-NVRHI | https://github.com/NVIDIA-RTX/NVRHI | higher-level RHI conveniences, binding/resource lifetime style |
| NV-NRI | https://github.com/NVIDIA-RTX/NRI | low-level explicit RHI shape and low-overhead philosophy |
| NV-NRI-SAMPLES | https://github.com/NVIDIA-RTX/NRISamples | small NRI usage/test-bench examples |
| NV-SHADERMAKE | https://github.com/NVIDIA-RTX/ShaderMake | shader compilation tool shape |
| NV-STREAMLINE | https://github.com/NVIDIA-RTX/Streamline | provider/resource tagging and integration boundary |
| NV-RTXDI | https://github.com/NVIDIA-RTX/RTXDI | reservoir direct lighting library/application ownership split |
| NV-RTXDI-LIBRARY | https://github.com/NVIDIA-RTX/RTXDI-Library | library-only reservoir direct lighting split |
| NV-RTXDI-ASSETS | https://github.com/NVIDIA-RTX/RTXDI-Assets | optional sample asset split |
| NV-SHARC | https://github.com/NVIDIA-RTX/SHARC | shader-focused GI feature integration |
| NV-NRD | https://github.com/NVIDIA-RTX/NRD | denoising feature/resource boundary |
| NV-NRD-SAMPLE | https://github.com/NVIDIA-RTX/NRD-Sample | denoising sample integration boundary |
| NV-RTXPT | https://github.com/NVIDIA-RTX/RTXPT | path tracing product/sample split |
| NV-RTXNS | https://github.com/NVIDIA-RTX/RTXNS | neural shading/readiness without broad engine ML ownership |
| NV-RTX-KIT | https://github.com/NVIDIA-RTX/RTX-Kit | neural rendering SDK landing and integration grouping |
| AMD-CAULDRON | https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron | D3D12/Vulkan framework split, sample resource management, shader/pipeline cache |
| AMD-FIDELITYFX | https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK | SDK/sample/tool/content separation |
| AMD-D3D12MA | https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator | D3D12 memory allocator integration |
| AMD-VMA | https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator | Vulkan memory allocator integration |
| UE-SOURCE | https://github.com/EpicGames/UnrealEngine | RHI/RenderCore/Renderer/GameFramework module separation; access requires linked Epic/GitHub account |

## Source Coverage Matrix

Use this matrix to confirm the implementation sequence covers the `00-Review` source set. If a later edit adds a requirement to a review document, add it to this matrix or to the relevant stage acceptance criteria instead of creating another plan.

| Source Document | Main Requirements Pulled Forward | Roadmap Stages That Close It | Concrete Result Expected |
| --- | --- | --- | --- |
| `A_PrincipalRenderingRequirements.md` | explicit graphics API ownership, shader quality, debugging fluency, real-time rendering depth, advanced feature readiness | 01, 20-35, 39, 41, 42 | Sparkle shows D3D12/Vulkan competence, shader/cook ABI discipline, ray tracing/GI/path tracing feature ownership, and late measured evidence. |
| `D_WholeRepositoryArchitectureMap.md` | current module map, public/private boundaries, RHI/Renderer flow, memory/GPU/CPU maps, system links | 02, 16-24, 35-38, 41, 42 | Public surface shrinks, RHI/Renderer/GameFramework/Core/Tools boundaries become easier to trace, and final checks prove the map improved. |
| `E_ExternalRendererRepositoryComparison.md` | scope clarity, RHI tradeoff honesty, sample/content separation, SDK boundary discipline, product packaging policy | Reference Repository Map, 01, 04-07, 20-23, 24-35, 40 | Sparkle follows mature repository patterns without copying their bulk: explicit scope, optional content, narrow provider bridges, real package outputs. |
| `F_StagedDeletionFirstImprovementPlan.md` | deletion-first work order, no-new-pollution rule, preserve capabilities while removing scaffolding | 03, 07, 10-19, 22, 27, 32-38, 40 | Every batch starts from preserved capability checks and ends with net simplification or a recorded reason why not. |
| `G_AdvancedGraphicsEngineExecutiveSummary.md` | product identity, shader ABI, capture, ray tracing/GI/path tracing, neural readiness, late performance evidence, wording safety | 01, 03, 08-12, 20-35, 39-42 | The engine direction is advanced graphics first: capture and shader packages remain, RT features deepen, ML-adjacent readiness stays practical, profiling waits until features exist. |
| `H_AdvancedGraphicsEngineerPersona.md` | explicit API ownership, renderer feature depth, shader/kernel craft, GPU architecture thinking, debugging/tool fluency, communication through clean code | 01, 20-35, 39, 41, 42 | A reviewer can infer the owner from the repo: thin public concepts, explicit backend work, preserved advanced RT/capture/shader capabilities, and less code around each feature. |

## Granular Implementation Sequence

Use this sequence as the real execution order. The broad phase notes below remain supporting context only; do not treat them as a second roadmap.

### Stage 00: Navigation And Batch Discipline

References: NV-DONUT, AMD-FIDELITYFX, UE-SOURCE.

Prompt:

- Make `Docs/README.md` point to this roadmap first.
- Keep `01-Implementation` as the active prompt set and `00-Review` as source material.
- Use the batch prompt for every change.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] README opens with `00_ORDERED_ImplementationRoadmap.md`.
- [x] No new planning document is created.
- [x] Every future batch names KEEP/MODIFY/ADD/REMOVE source docs.
- [x] Any final build/cook/run risk is recorded instead of forcing stabilization after each small stage.

### Stage 01: Product Identity And Module Contract

References: NV-DONUT, AMD-CAULDRON, UE-SOURCE.

Prompt:

- Confirm Sparkle is a renderer-first engine.
- Define in existing docs only: Core, RHI, Renderer, GameFramework, Tools, Projects responsibilities.
- Keep high-level concepts thin: project, level, scene, render path, backend, capture, cook.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Core owns foundation utilities only.
- [x] RHI owns explicit D3D12/Vulkan resources, descriptors, pipelines, queues, barriers, ray tracing, capture, presentation.
- [x] Renderer owns frame graph, passes, scene extraction, provider boundaries, TLAS/PTLAS policy, shader registrations.
- [x] GameFramework owns level/scene/assets/components without renderer-private details.
- [x] Tools own shader compile, import, cook, launch, package workflows.
- [x] Projects own sample projects, selectable levels, scene/content data, and optional heavy content packs.
- [x] No new module abstraction is introduced.

### Stage 02: Baseline Repository Shape Snapshot

References: NV-DONUT, AMD-FIDELITYFX, UE-SOURCE.

Prompt:

- Capture current source line/file/depot weight by area.
- Record public/private line counts for Core/RHI/Renderer/GameFramework.
- Save this as working notes or update existing review docs only if stale.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Current largest areas are known.
- [x] Current public API hotspots are known.
- [x] Current Projects depot weight is known.
- [x] No source code changes are made in this stage.

### Stage 03: Preserve Capability Checklist

References: NV-NRI, NV-NVRHI, AMD-CAULDRON, UE-SOURCE.

Prompt:

- Audit every planned deletion against `01_KEEP_PreservedCapabilities.md`.
- Mark preserved capability owners.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] D3D12 and Vulkan parity is listed as a preserved capability.
- [x] Offline cooked shader packages with reflection data are listed as preserved.
- [x] Screenshot/BMP capture is listed as preserved.
- [x] Classic TLAS and PTLAS are listed as preserved.
- [x] Multi-level support is listed as preserved.
- [x] Core/RHI/Renderer/GameFramework/Tools/Projects separation is listed as preserved.

### Stage 04: Content Discovery And Default Level Set

References: NV-DONUT-SAMPLES, AMD-FIDELITYFX, AMD-CAULDRON.

Prompt:

- Inventory `Projects/Showcase` levels, scenes, assets, and heavyweight media.
- Decide the curated default level set.
- Do not remove content yet.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Every in-repo level is listed.
- [x] Default level set is named.
- [x] Heavy optional content candidates are listed by path and approximate size.
- [x] Multi-level support remains explicitly preserved.

### Stage 05: Minimal Level/Content Catalog

References: NV-DONUT-SAMPLES, AMD-FIDELITYFX.

Prompt:

- Add only the metadata needed to select levels and identify optional packs.
- Keep the schema tiny.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Catalog contains level id, display name, source path, default inclusion, optional pack id if needed.
- [x] Launcher/cooker/runtime can resolve the default level set through the catalog.
- [x] Missing optional pack state can be represented.
- [x] No asset database or content browser rewrite is added.

Result:

- Added `Projects/Showcase/Levels.catalog` as the project-owned default level catalog.
- Runtime level discovery now uses the project catalog as the single owner of level discovery; missing catalog is reported at the GameFramework boundary instead of scanning arbitrary level files.
- Asset cook discovery uses catalog `Default` levels to filter project scene sources while preserving engine scene discovery.
- Launcher level sync controls use catalog metadata to make required levels locked-on and optional levels user-selectable.
- Launcher header level selection is the single visible startup-level source for editor/runtime launch; the Launch Project page no longer owns a duplicate startup-level selector.
- Launcher GUI project state is single-active-project state, not user-selectable project state; build/cook/launch requests still carry the active project id only for paths and target ownership.
- Optional pack metadata is represented by `Id`, `Root`, and `Available`; missing or disabled packs can suppress catalog levels without introducing a content database.

### Stage 06: Optional Heavy Content Pack Boundary

References: AMD-FIDELITYFX, NV-DONUT-SAMPLES, NV-RTXDI-ASSETS if used through RTXDI asset split.

Prompt:

- Define optional content pack ownership.
- Make heavy content optional without reducing level capability.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Optional pack root is defined.
- [x] Default build/cook/run path does not require optional heavy content.
- [x] Missing optional content produces a clear non-fatal state.
- [x] Core repo byte reduction target is recorded.

Result:

- Optional pack ownership is project-owned through `Projects/Showcase/Levels.catalog`.
- Optional pack roots are catalog `Root` values relative to `Projects/Showcase`; the current heavy pack is `Bistro` at `Assets/Meshes/Bistro`.
- Default runtime levels remain the seven curated Showcase levels; none reference `Bistro`.
- Default asset cook discovery uses catalog `Default` levels, so project scene source discovery excludes `Bistro` unless a default level references it.
- Missing optional content is non-fatal: catalog levels that name a missing or disabled optional pack are skipped, while default levels and cook discovery continue.
- Stage 07 byte target: remove or externalize at least `Projects/Showcase/Assets/Meshes/Bistro` from the core repo, about 1438.80 MB, reducing `Projects` source content from about 1527.06 MB to about 88.26 MB before other generated-output cleanup.

### Stage 07: Externalize Or Remove Heavy Content

References: AMD-FIDELITYFX, NV-DONUT-SAMPLES.

Prompt:

- Move/delete heavy content after catalog support exists.
- Preserve curated levels.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Heavy Bistro/media content is removed from default repo footprint or marked external.
- [x] Default level set remains selectable.
- [x] Optional heavy levels remain discoverable.
- [x] Depot byte count is materially lower.

Result:

- Removed `Projects/Showcase/Assets/Meshes/Bistro` from the core repo footprint.
- Kept optional pack metadata in `Projects/Showcase/Levels.catalog` with `Id = Bistro`, `Root = Assets/Meshes/Bistro`, `Available = false`, and `External = true`.
- Curated default set remains seven levels; all seven level files resolve and all five referenced default source scenes still exist.
- Launcher optional-pack sync controls expose external/missing pack state without hardcoding any content name in compiled code.
- `Projects` source content, excluding generated logs/cooked output, dropped from about 1527.06 MB to about 88.26 MB.

### Stage 08: Capture Path Inventory

References: AMD-CAULDRON, UE-SOURCE, NV-DONUT.

Prompt:

- Find every screenshot/BMP/readback path.
- Separate product capture from smoke/ad hoc capture.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Every `CaptureTextureToBmp`, `RhiCaptureService`, and BMP writer call site is listed.
- [x] One intended product capture owner is selected.
- [x] Smoke/ad hoc capture call sites are named for removal.
- [x] D3D12/Vulkan readback impact is recorded.

Result:

This stage made no capture source-code changes. Existing capability search found an already-owned product path:

`Renderer::CaptureViewportProductToBmp` -> `FramePipeline::CaptureViewportProductToBmp` -> `RenderHardwareInterface::GetCaptureService()` -> backend `CaptureTextureToBmp` -> private `WriteRhiBmp`.

Product capture owner selected:

- Owner: Renderer viewport product capture.
- Public product entrypoint: `Engine/Renderer/Public/Renderer.h:53`, `Engine/Renderer/Private/Renderer.cpp:98`.
- Renderer implementation: `Engine/Renderer/Private/FramePipeline/FramePipelineViewportProducts.cpp:62`.
- Request type: `Engine/Renderer/Public/Viewport/ViewportContracts.h:138`.
- Ownership rule for Stage 09: editor/tool capture should call the renderer viewport product entrypoint. It should not call backend capture services or the BMP writer directly.

`CaptureTextureToBmp` and `RhiCaptureService` inventory:

| Path | Role | Stage 09 decision |
| --- | --- | --- |
| `Engine/RHI/Public/Capture/RhiCaptureService.h:40` | RHI backend service interface. | Keep, but keep it backend-owned and avoid exposing it as a general app/tool API. |
| `Engine/RHI/Public/Capture/RhiCaptureService.h:45` | `CaptureTextureToBmp` virtual backend operation. | Keep behind renderer product capture. |
| `Engine/RHI/Private/Capture/RhiCaptureService.cpp:3` | Virtual destructor definition. | Keep. |
| `Engine/RHI/Public/Device/RenderHardwareInterface.h:39` | Backend service accessor. | Keep only as RHI service access for renderer/backend code. |
| `Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.h:60` and `.cpp:261` | D3D12 `GetCaptureService`. | Keep for D3D12 parity. |
| `Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp:85` | Creates `D3D12CaptureService`. | Keep for D3D12 parity. |
| `Engine/RHI/Private/D3D12/Capture/D3D12CaptureService.h:7` | D3D12 service type. | Keep, backend-private. |
| `Engine/RHI/Private/D3D12/Capture/D3D12CaptureService.h:12` and `.cpp:34` | D3D12 `CaptureTextureToBmp`. | Keep, harden assumptions in Stage 09. |
| `Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.h:58` and `.cpp:202` | Vulkan `GetCaptureService`. | Keep for Vulkan parity. |
| `Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.cpp:87` | Creates `VulkanCaptureService`. | Keep for Vulkan parity. |
| `Engine/RHI/Private/Vulkan/Capture/VulkanCaptureService.h:8` | Vulkan service type. | Keep, backend-private. |
| `Engine/RHI/Private/Vulkan/Capture/VulkanCaptureService.h:13` and `.cpp:30` | Vulkan `CaptureTextureToBmp`. | Keep, harden assumptions in Stage 09. |
| `Engine/Renderer/Private/FramePipeline/FramePipelineViewportProducts.cpp:85` | Only renderer-to-RHI `CaptureTextureToBmp` call site. | Keep as the only product-to-RHI bridge. |

BMP writer inventory:

| Path | Role | Stage 09 decision |
| --- | --- | --- |
| `Engine/RHI/Private/Capture/RhiBmpWriter.h:14` | Private BMP writer declaration. | Keep private to RHI capture. |
| `Engine/RHI/Private/Capture/RhiBmpWriter.cpp:85` | BMP writer implementation. | Keep only behind backend capture services. |
| `Engine/RHI/Private/D3D12/Capture/D3D12CaptureService.cpp:172` | D3D12 write call. | Keep. |
| `Engine/RHI/Private/Vulkan/Capture/VulkanCaptureService.cpp:242` | Vulkan write call. | Keep. |

Readback inventory and impact:

| Area | Path | Current impact | Stage 09/late-stage action |
| --- | --- | --- | --- |
| Capture capability flags | `Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp:140`, `Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.cpp:272` | Both backends advertise upload/readback support. | Preserve D3D12/Vulkan parity. |
| D3D12 screenshot readback | `Engine/RHI/Private/D3D12/Capture/D3D12CaptureService.cpp:79`, `:128`, `:166` | Allocates a readback heap buffer, transitions texture from `COMMON` to copy source, copies with `CopyTextureRegion`, waits on a fence, maps on CPU, writes BMP. Synchronous and only suitable as a user-triggered tool/editor action. | Keep capability; harden resource-state assumptions and avoid frame-path use. |
| Vulkan screenshot readback | `Engine/RHI/Private/Vulkan/Capture/VulkanCaptureService.cpp:71`, `:184`, `:240` | Allocates host-visible coherent transfer buffer, transitions image from `GENERAL` to transfer source, copies with `vkCmdCopyImageToBuffer`, waits on a fence, maps on CPU, writes BMP. Synchronous and only suitable as a user-triggered tool/editor action. | Keep capability; harden layout/format assumptions and avoid frame-path use. |
| D3D12 timestamp readback | `Engine/RHI/Private/D3D12/Diagnostics/D3D12RenderDiagnostics.cpp:177`, `:275`, `:292` | Diagnostics/profiling readback, not screenshot capture. | Do not mix with screenshot hardening; profiling cleanup is later priority. |
| PTLAS validation readback | `Engine/RHI/Public/RayTracing/RhiPartitionedTlasDesc.h:200`, `Engine/RHI/Private/D3D12/RayTracing/D3D12PartitionedTlasServices.cpp:437`, `Engine/RHI/Private/Vulkan/RayTracing/VulkanPartitionedTlasServices.cpp:438` | Optional validation/CPU readback around PTLAS, not product screenshot capture. | Preserve TLAS/PTLAS feature; do not let capture work delete this capability. |

Smoke/ad hoc capture removal candidates:

- No source call site currently invokes `CaptureViewportProductToBmp`, `CaptureTextureToBmp`, or `WriteRhiBmp` from smoke/ad hoc code.
- `rg -n "SPARKLE_SMOKE|SmokeDiagnostics|smoke|Smoke|Screenshot|screenshot|CaptureViewportProductToBmp|CaptureTextureToBmp" . -g"*.cpp" -g"*.h" -g"*.cmake" -g"CMakeLists.txt" -g"*.ps1" -g"*.py"` found only the product capture declarations/definitions and no smoke-owned screenshot execution path.
- Stage 09 removal rule: if a smoke or validation harness later appears around screenshot/BMP capture, remove that owner and route only through the renderer product capture entrypoint.

Net code pressure:

- No capture code was added in this stage.
- Stage 09 must reduce or hold code count by narrowing public exposure, deleting any stale capture/debug scaffolding discovered during implementation, or recording the exact next removal if hardening requires a small replacement.

### Stage 09: Harden Screenshot/BMP Capture

References: AMD-CAULDRON, UE-SOURCE.

Prompt:

- Preserve capture as an editor/tool capability.
- Narrow ownership and remove smoke coupling.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Capture entrypoint is product-owned.
- [x] BMP writer remains only behind the intended path.
- [x] Smoke/ad hoc capture ownership is gone.
- [x] Public API is smaller or explicitly justified.

Result:

- Product capture remains `Renderer::CaptureViewportProductToBmp`, but the public renderer API now returns `ViewportCaptureResult` instead of `RhiCaptureResult`. This removes `RHI/Public/Capture/RhiCaptureService.h` from `Engine/Renderer/Public/Renderer.h`.
- `ViewportCaptureResult` lives beside `ViewportCaptureRequest` in `Engine/Renderer/Public/Viewport/ViewportContracts.h`, keeping the high-level capture contract renderer-owned.
- The private renderer bridge in `Engine/Renderer/Private/FramePipeline/FramePipelineViewportProducts.cpp` is now the only renderer-to-RHI capture call site. It validates output path, product availability, and capturable product format before entering RHI.
- `RhiTextureCaptureRequest` now carries the renderer-tracked source `ResourceState`; D3D12 and Vulkan capture services transition from that explicit state to `CopySource` and then restore it.
- D3D12 no longer assumes `D3D12_RESOURCE_STATE_COMMON` for screenshot source resources. It uses `D3D12TypeConversions::ToResourceStates` from the provided source state.
- Vulkan no longer assumes `VK_IMAGE_LAYOUT_GENERAL` for screenshot source images. It uses `VulkanTypeConversions::ToResourceStateMapping` from the provided source state.
- `WriteRhiBmp` remains private to `Engine/RHI/Private/Capture` and is still called only by `D3D12CaptureService` and `VulkanCaptureService`.
- No smoke/ad hoc screenshot caller exists in source, and no new smoke, validation, report, or gallery path was added.

Net code pressure:

- This stage added a small renderer-owned result type and explicit source-state field, but removed the public renderer dependency on RHI capture result types and removed hardcoded backend state/layout assumptions.
- The batch is justified as an ownership and correctness hardening step; next removal pressure remains Stage 10 smoke harness removal and Stage 11-12 report/debug artifact cleanup.

Verification:

- `cmake --build build --target SparkleRenderer --config DevelopmentEditor` succeeded.

### Stage 10: Smoke Harness Removal

References: NV-NRI-SAMPLES for test-bench separation, AMD-CAULDRON for framework simplicity.

Prompt:

- Remove smoke harnesses that preserve validation/report scaffolding.
- Preserve fatal checks and product launch paths.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] `RhiSmoke`, `SmokeDiagnostics`, and `SPARKLE_SMOKE` references are gone or intentionally product-owned.
- [x] No screenshot capability is removed.
- [x] No new validation system replaces smoke.
- [x] Final stabilization risk is recorded if launch behavior is touched.

Result:

- No smoke harness source code remains to remove in this stage.
- Source search `rg -n "RhiSmoke|SmokeDiagnostics|SPARKLE_SMOKE|Smoke|smoke|SmokeTest|smoke-test" Engine Tools Projects -g"*.cpp" -g"*.h" -g"*.cmake" -g"CMakeLists.txt" -g"*.ps1" -g"*.py" -g"*.bat"` returned no matches.
- The remaining smoke mentions are planning/review document references that describe deletion policy, not executable product code.
- Screenshot/BMP capture remains preserved through the Stage 09 renderer-owned path.
- No launch behavior was touched in this stage; final stabilization risk is unchanged from earlier stages.
- No replacement validation, smoke, report, or diagnostic system was added.

Net code pressure:

- No source code was added.
- No source code was deleted because the target smoke harnesses were already absent.
- The next actual deletion pressure moved to Stage 13 launcher workflow inventory and Stage 14 launcher workflow slimming.

### Stage 11: AssetCooker Default Report Cleanup

References: AMD-FIDELITYFX, NV-SHADERMAKE.

Prompt:

- Remove default cook reports and timing summaries.
- Keep cooked asset outputs and fatal errors.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Default AssetCooker path writes cooked assets, not plan/timing report artifacts.
- [x] `asset-cooker-plan-v1` and `asset-cooker-summary-v1` are gone from default path.
- [x] Fatal errors remain clear.
- [x] No report replacement is added.

Result:

- Removed `AssetCookerDiscovery::WritePlanSummary`, the `asset-cooker-plan-v1` file writer, and the plan/summary path fields from `AssetCookerProjectCookPlan`.
- Removed `AssetCookerWriteTimingSummary`, `AssetCookerPrintTopStageTimings`, timing structs, elapsed-time fields, and `asset-cooker-summary-v1` JSON emission from `AssetCookerDispatcher`.
- Stopped passing `--summary` to `TextureCooker` from the AssetCooker default texture path, so the launcher-driven cook path no longer creates a texture timing summary as a side effect.
- Moved the transient texture request file out of `artifacts/diagnostics/cook/Temp` into `artifacts/dev/tools/AssetCooker/Temp`.
- Kept cooked output records in `AssetCookerCliPrintResult`, stage progress/status messages, and fatal diagnostics from shader, texture, scene, mesh, and material cook failures.
- No new report, replacement summary, fallback chain, or content hardcode was added.

Net code pressure:

- This stage is net negative: it deletes report/timing artifact code and only updates the existing roadmap result.

Verification:

- `cmake --build build --target AssetCooker --config DevelopmentEditor` succeeded.
- `rg -n "asset-cooker-plan-v1|asset-cooker-summary-v1|planPath|summaryPath|textureSummaryPath|WritePlanSummary|WriteTimingSummary|PrintTopStageTimings|StageTiming|elapsedMs|Timing summary written|AssetCooker stage timings|artifacts.*/diagnostics.*/cook|--summary" Tools/Cooking/AssetCooker Tools/Launcher/SparkleLauncher/Private/Cook` returned no matches.

### Stage 12: Shader Debug Artifact Cleanup

References: NV-SHADERMAKE, AMD-CAULDRON.

Prompt:

- Remove default shader debug bundles/stats.
- Preserve offline cooked packages and reflection data.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Default shader cook writes runtime packages.
- [x] Reflection data remains available to runtime.
- [x] Debug artifacts are opt-in or removed.
- [x] Validation-only shader registrations are removed from product runtime.

Result:

- Launcher and AssetCooker shader cook orchestration now call the runtime package cook directly; the separate `list-shaders --validate` step is no longer part of product cook.
- Launcher cook requests no longer expose or forward cooked shader stats CSV generation. `--analysis cooked-shader-stats` remains only as an intentional low-level ShaderCompiler CLI analysis path, not a default launcher or AssetCooker behavior.
- Default shader cook settings now preserve reflection by default in `ShaderPackageCookSettings`, `CookOperationRequest`, `LauncherOperationRequest`, and launcher settings.
- Runtime package output remains owned by `ShaderCompiler cook`, which writes `.sparkshader` packages and `ShaderPackageRegistry.sreg`.
- Reflection remains serialized into runtime cooked packages through `CookedPackageWriter` and loaded by `CookedShaderPackageCache`.
- Debug artifact bundles remain opt-in through the explicit `--debug-artifacts` path; no default debug bundle directory is used unless the launcher setting is enabled.

Net code pressure:

- No source code was added.
- Source code was removed from launcher stats plumbing, validation-only cook steps, stale validation failure handling, and AssetCooker pre-cook validation.
- The stage reduces default debug/report surface while preserving runtime shader packages and reflection data.

### Stage 13: Launcher Workflow Inventory

References: AMD-FIDELITYFX, NV-DONUT-SAMPLES.

Prompt:

- List launcher workflows that are product-owned.
- Mark diagnostic cockpit features for removal.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Build, cook, run, clean, package-if-owned are the only first-class launcher workflows.
- [x] Diagnostic/status/quality/debug-only launcher pages are listed.
- [x] Package UI ownership is decided.
- [x] No launcher feature is added.

Result:

- Product-owned first-class workflow families are:
  - Build: `workspace.generate-build-files`, `workspace.build-all`, `launcher.build.self`, `project.build.editor`, `project.build.runtime`, `cook.tools.prepare`.
  - Cook: `cook.project`, `cook.shaders`, `cook.textures`, `cook.assets`.
  - Run: `project.open.editor`, `project.open.runtime`, `project.run`; `workspace.open-ide` is build/navigation support, not a separate product family.
  - Clean: `workspace.clean`.
  - Package: `package.release`, because it maps to `BuildWorkspaceOperationKind::AssembleRelease` and writes reviewable runtime/symbol package layouts under `dist/releases/<version>` without publishing.
- Preserved but not first-class:
  - `workspace.sync-source-tiers` stays as required preparation for dependency and optional level/content sync, but Stage 14 should fold it into a Build/Prepare surface rather than keep a standalone top-level workflow family.
  - Level and optional content sync remain product capability; they should stay catalog-driven and must not hardcode sample names in launcher code.
- Diagnostic, status, quality, debug, or cockpit surfaces marked for Stage 14 removal or demotion:
  - `toolchain.check` as a standalone visible workflow. Keep host-readiness checks as blocking readiness data owned by Build/Cook/Run pages.
  - `quality.format` as a launcher workflow. Prefer CLI/dev-tool ownership or keep it outside the first-class launcher surface.
  - `LauncherMainWindowStatusPages.cpp` action-dependency panels named `Action Dependencies - ...`; keep only compact blocking readiness on the owning workflow.
  - `LauncherMainWindowSyncPages.cpp` raw third-party dependency cache inventory. Keep enabled dependency sync state, remove raw package cockpit details from the normal flow.
  - `LauncherOutputWidgets.*` and `LauncherMainWindowActivity.cpp` expanded Activity/log cockpit. Keep last-run result and failure surface only if directly actionable.
  - `LauncherRecoveryUiModel.*` and workflow recovery panels that duplicate action dependencies. Keep one recovery path at the owner boundary.
  - `LauncherMainWindowPagePrimitives.cpp` workflow visual banners if they do not carry direct command state.
  - Advanced shader diagnostics in `LauncherMainWindowOptionPages.cpp`: debug artifact bundles, debug info, optimization/debug stripping switches should leave the default GUI or become an explicit developer-only path.
  - Package contents/status breakdown in `LauncherMainWindowOptionPages.cpp` should be reduced to direct package assembly inputs until Stage 15 defines owned package outputs.
- Package UI ownership decision:
  - Keep `package.release` as an owned workflow only for assembly of already-built product artifacts.
  - Do not expand package UI into release validation, publishing, checklist, signing, or report generation.
  - Stage 15 still owns the detailed package output list: runtime, editor if shipped, launcher, dev tools if shipped, symbols archive, optional content packs.

Net code pressure:

- No source code was added.
- No source code was removed in this inventory-only stage.
- Stage 14 must pay the debt by deleting or demoting the listed diagnostic cockpit surfaces without replacing them with new panels.

Search evidence:

- Operation definitions were inventoried from `BuildWorkspacePlanner.cpp`, `CookOperations.cpp`, `LaunchOperations.cpp`, `MaintenanceOperations.cpp`, and `LauncherWorkflowCatalog.cpp`.
- UI cockpit surfaces were inventoried from `LauncherMainWindowStatusPages.cpp`, `LauncherMainWindowSyncPages.cpp`, `LauncherOutputWidgets.*`, `LauncherMainWindowActivity.cpp`, `LauncherRecoveryUiModel.*`, and `LauncherMainWindowOptionPages.cpp`.

### Stage 14: Launcher Workflow Slimming

References: AMD-FIDELITYFX, NV-DONUT-SAMPLES.

Prompt:

- Remove launcher UI/actions that do not support current workflows.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Launcher source line count decreases.
- [x] Shader debug/stat toggles leave default GUI.
- [x] Diagnostic-only pages/actions are removed.
- [x] No new panel replaces deleted panels.

Result:

- The default launcher workflow catalog now exposes product workflows only: build/prepare, cook, launch, package assembly, and clean. `workspace.sync-source-tiers` remains available, but it is folded into the Build preparation surface rather than a standalone top-level sync family.
- Shader debug/stat GUI controls were removed from the default shader cook page. Runtime shader packages and reflection remain owned by the cook path; debug artifacts are no longer a default GUI workflow.
- The package page was reduced to direct package assembly status instead of package cockpit/status breakdowns. Stage 15 still owns the exact package output contract.
- Removed workflow visual banners, workflow recovery UI/model, raw dependency inventory rows, per-dependency clean/regenerate actions, and diagnostic/status cockpit affordances that were not product workflows.
- Removed the launcher `quality.format` action across GUI, shell, maintenance planning, maintenance execution, and toolchain detection. Maintenance is now clean-only.
- Removed the standalone host-environment diagnostic action. Toolchain readiness remains available as build/cook/launch readiness data, but missing prerequisites now block at the owning workflow boundary instead of launching a diagnostic fallback workflow.
- No new panel, fallback chain, content hardcode, project hardcode, level hardcode, asset hardcode, or content-pack hardcode was added.

Net code pressure:

- Launcher code is net negative in this batch: `git diff --numstat -- Tools/Launcher/SparkleLauncher` totals 42 added lines and 878 deleted lines, for a net reduction of 836 lines.

Verification:

- Searches for removed workflow surfaces returned no matches in launcher C++/header source: `quality.format`, `format-mode`, `RunClangFormat`, `FormatMode`, `toolchain.check`, `CheckToolchain`, `Verify Host`, `Sync Diagnostics`, workflow recovery UI, workflow visual banners, advanced dependency inventory, tracked dependency actions, and shader debug/stat GUI labels.
- `cmake --build build --target SparkleLauncher --config DevelopmentEditor` succeeded. The deploy step still prints the existing warning `Cannot find Visual Studio installation directory, VCINSTALLDIR is not set.`
- `git diff --check` reported no whitespace errors; it only printed existing LF-to-CRLF working-copy warnings.

### Stage 15: Package Ownership Decision

References: NV-STREAMLINE, AMD-FIDELITYFX.

Prompt:

- Decide which packages are real: runtime, editor, launcher, dev tools, symbols, optional content.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Owned package list is written in existing docs or package config.
- [x] Unowned package outputs are marked for removal.
- [x] Optional content is not part of runtime package by default.
- [x] Checksums/manifests are kept only if consumed.

Package ownership decision:

| Package surface | Decision | Current owner | Stage 40 implementation rule |
| --- | --- | --- | --- |
| Runtime review package | Owned now. | `sparkle_release_assembly` creates `sparkle-runtime-<version>-<channel>-<platform>`. | Keep one runtime package rooted at `dist/releases/<version>/<runtime-package>/`; it contains runnable apps, cooked default content, runtime support, licenses, release notes, and the minimum package manifest needed for package-root discovery. |
| Launcher | Owned as a runtime package component, not as a separate package. | `CMake/SparkleReleaseAssembly.cmake` stages launcher artifacts at the runtime package root. | Keep `SparkleLauncher.exe` at package root because package-root launch detection depends on it. Do not create a separate `sparkle-launcher` package unless a consumer appears. |
| Runtime app | Owned as a runtime package component. | `CMake/SparkleReleaseAssembly.cmake` stages project runtime artifacts under `Apps/<RuntimeApp>/`. | Keep the runtime app in the runtime package. Avoid project-name hardcoding in the implementation pass; resolve package app identity from project/package metadata. |
| Editor app | Owned for review packages only, as a runtime package component. | `CMake/SparkleReleaseAssembly.cmake` stages project editor artifacts under `Apps/<EditorApp>/`. | Keep editor staging only while the review package is explicitly engine-evaluation oriented. Do not create a separate `sparkle-editor` package until there is a consumer. |
| Symbols | Owned now as a separate package. | `sparkle_release_assembly` creates `sparkle-symbols-<version>-<channel>-<platform>` and a zip when possible. | Keep symbols separate from runtime. Symbols may include PDB/debug artifacts from built products, but must not be copied into the runtime package. |
| Dev tools | Not owned as a package now. | Development tools remain build/cook prerequisites under `artifacts/dev/tools`. | Do not stage `ShaderCompiler`, cook tools, importers, headers, import libraries, static libraries, or source-facing diagnostics into runtime packages. Remove any future `sparkle-dev-tools` assembly until a real consumer requires it. |
| Dependency pack | Not owned as a package now. | Current release assembly writes a dependency-pack manifest name only. | Remove dependency-pack naming/manifests unless Stage 40 implements a real dependency package consumer. Source dependency sync remains a launcher/build workflow, not a distributable package. |
| Optional content pack | Owned as a future separate package, not part of default runtime. | Optional level/content availability is represented by catalog metadata such as `Projects/Showcase/Levels.catalog`. | Do not include optional synced content in the runtime package by default. Runtime package assembly must stage only the curated default level set and shared runtime content; optional content packs must be explicit separate outputs. |

Unowned outputs marked for removal:

- Remove or replace broad cooked-root staging in `CMake/SparkleReleaseAssembly.cmake` before treating package assembly as final. Copying all of `artifacts/dev/projects/<Project>/cooked` risks bundling optional levels and externally synced content into the default runtime package.
- Remove package IDs/config paths for separate launcher, editor, dev tools, and dependencies packages unless a real package consumer is added.
- Remove dependency-pack manifest emission from release assembly unless a dependency package is actually built and consumed.
- Remove package file manifests and `SHA256SUMS.txt` outputs unless the launcher, runtime, or final release validation command consumes them.

Manifest/checksum decision:

- Keep `manifests/sparkle-package-manifest.json` because package-root discovery consumes it through `Engine/Core/Private/FileSystemUtils.cpp`.
- Keep the bundled-component manifest only if Stage 40 uses it to drive package UI or package validation. Otherwise remove it with the package cockpit remnants.
- Treat `sparkle-release-manifest.json`, `sparkle-build-manifest.json`, `sparkle-dependency-manifest.json`, `sparkle-package-files.json`, and `SHA256SUMS.txt` as unowned review artifacts until a consuming workflow exists.

Result:

- Stage 15 is a decision-only stage; no package assembly code was added.
- Existing package capability search covered `CMake/SparkleReleaseAssembly.cmake`, `CMake/SparkleArtifactContract.cmake`, launcher package operation definitions, package-root discovery, and package manifest/checksum references.
- The owned package list is now explicit in this roadmap: runtime review package, package-root launcher component, runtime app component, editor app component for review packages, separate symbols package, and future optional content packs.
- Dev tools, separate launcher/editor packages, dependency pack outputs, broad cooked-root staging, unconsumed package manifests, and checksums are marked for removal or replacement in Stage 40.
- Optional content is explicitly excluded from the default runtime package. The implementation rule is catalog-driven default content only, with optional content packs as separate outputs.

Net code pressure:

- No source or package code was added in this stage. The only change is the roadmap ownership decision.
- The next package implementation stage must be net negative by deleting or replacing unowned package outputs rather than layering a package manager on top of the current script.

Verification:

- `rg -n "SparkleReleaseAssembly|sparkle_release_assembly|ReleaseAssembly|dist/releases|checksums|manifest|symbols" CMake Tools/Launcher` located the existing package assembly surface.
- `rg -n "sparkle-package-files|SHA256SUMS|sparkle-release-manifest|sparkle-build-manifest|sparkle-dependency-manifest|sparkle-package-manifest|sparkle-bundled-runtime-components|manifests" Tools Engine Projects CMake Docs` confirmed that package-root discovery consumes `sparkle-package-manifest.json`, while the detailed file manifests/checksums have no engine or launcher consumer today.

### Stage 16: Public Renderer Observation Inventory

References: UE-SOURCE, NV-DONUT, AMD-CAULDRON.

Prompt:

- Inventory public Renderer diagnostics and observation APIs.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Renderer public diagnostic headers are listed.
- [x] Consumers are listed.
- [x] Editor-only consumers are separated from runtime behavior.
- [x] Product-owned observation surfaces are explicitly justified.

Public Renderer Observation Inventory:

| Public surface | Header/API | Current consumers | Classification | Decision |
| --- | --- | --- | --- | --- |
| Mesh diagnostics snapshot | `Engine/Renderer/Public/Meshes/MeshDiagnostics.h`; `Renderer::CaptureMeshDiagnostics()` | `Engine/Application/Private/EditorApplication.cpp`, `Engine/Editor/Public/UI.h`, `Engine/Editor/Public/Panels/UsedMeshesPanel.h`, `Engine/Editor/Private/Panels/UsedMeshesPanel.cpp`; producer is `RendererSystemRoot` through `MeshDiagnosticsCollector` | Editor-only observation | Keep for now because the editor asset/scene inspection workflow consumes it. Later slimming should move the snapshot provider behind an editor-facing adapter if runtime no longer needs the header. |
| Texture diagnostics snapshot | `Engine/Renderer/Public/Resources/Textures/TextureDiagnostics.h`; `Renderer::CaptureTextureDiagnostics()` | `EditorApplication`, `UI`, `UsedTexturesPanel`; producer is `TextureManager::CaptureDiagnosticsSnapshot()` | Editor-only observation | Keep for now. It is an editor inspection surface, not runtime behavior. Later slimming should avoid exposing texture diagnostics through broad renderer API if an editor adapter can own it. |
| Renderer memory diagnostics snapshot | `Engine/Renderer/Public/Diagnostics/RendererMemoryDiagnostics.h`; `Renderer::CaptureMemoryDiagnostics()` | `EditorApplication`, `UI::CaptureMemoryDiagnostics`, editor diagnostics providers; producer is `RendererMemoryMonitor` | Editor/tool observation with product value | Keep. Memory pressure and residency data are useful for productization and feature work, but it should remain a snapshot, not a growing report system. |
| Viewport products and presentation | `Engine/Renderer/Public/Viewport/ViewportContracts.h`; `Renderer::SubmitViewportRenderRequest`, `GetViewportRenderProducts`, `BeginViewportPresentation`, `EndViewportPresentation` | `RuntimeApplication`, `EditorApplication`, `ViewportPanel`, `UI` | Product-owned runtime/editor contract | Keep. This is not diagnostic clutter; it is the high-level viewport contract between runtime/editor shell and renderer. |
| Screenshot/BMP capture | `ViewportCaptureRequest`, `ViewportCaptureResult`; `Renderer::CaptureViewportProductToBmp()` | Product capture path through `FramePipelineViewportProducts.cpp`; no smoke/ad hoc caller after Stage 09-10 | Product-owned tool/editor capability | Keep. It preserves screenshot capability while hiding RHI capture details behind the renderer-owned viewport product contract. |
| Debug view modes | `Engine/Renderer/Public/Debug/RenderViewMode.h`, `RendererCVars.h`, `CVarRenderViewMode` | `ViewportTopPanel`, `FramePipeline`, render pass utilities | Editor-facing rendering mode with runtime pass impact | Keep for now because editor viewport modes are real renderer features. Later stages should decide whether `RendererCVars.h` belongs in public or should become an editor/runtime settings bridge. |
| Renderer diagnostic CVars and markers | `RendererCVars.h` exposes `CVarRendererDiagnosticMarkerVerbosity`, `CVarRendererDiagnosticGpuTiming`, mesh batching, and ray tracing/PTLAS tuning CVars | Renderer private frame graph, pass diagnostics, ray tracing planner/builders, editor viewport top panel for view mode | Mixed: some product tuning, some diagnostic/profiling | Mark for Stage 16/17 follow-up pressure. Keep feature/tuning CVars that affect real rendering behavior; defer profiling/marker CVars to late performance stages or private renderer implementation unless a product UI consumes them. |
| Shader package reload observation | `Engine/Renderer/Public/Shaders/CookedShaderReloadResult.h`; `Renderer::ReloadCookedShaders`, `GetShaderPackageGeneration` | `ShaderRecookCoordinator`, `EditorApplication`, editor UI provider | Product-owned editor/tool workflow | Keep. It preserves offline cooked shader package workflow and reflection-era reload behavior without adding debug bundles. |
| Low-level renderer escape hatches | `Renderer::GetRenderHardwareInterface`, `GetCommandSubmissionService`, `GetImGuiRenderer` | `EditorApplication`, `RuntimeConsoleHost`, `ShaderRecookCoordinator` | Boundary pressure, not observation | Mark for later slimming. These are currently real integration points, but they expose RHI details through `Renderer.h`. Future work should replace only the external needs with thin renderer/editor services instead of broad RHI access. |

Editor-only consumers:

- `Engine/Editor/Public/UI.h` owns editor diagnostics provider slots for mesh, texture, memory, and shader package generation.
- `Engine/Editor/Public/Panels/UsedMeshesPanel.h` and `Engine/Editor/Private/Panels/UsedMeshesPanel.cpp` consume `MeshDiagnosticsSnapshot`.
- `Engine/Editor/Public/Panels/UsedTexturesPanel.h` and `Engine/Editor/Private/Panels/UsedTexturesPanel.cpp` consume `TextureDiagnosticsSnapshot`.
- `Engine/Editor/Public/Panels/ViewportTopPanel.h` and `Engine/Editor/Private/Panels/ViewportTopPanel.cpp` consume `RenderViewMode` and `CVarRenderViewMode`.
- `Engine/Application/Private/EditorApplication.cpp` wires renderer diagnostics providers into editor UI and uses viewport presentation products for editor rendering.

Runtime/product consumers:

- `Engine/Application/Private/RuntimeApplication.cpp` uses `ViewportRenderRequest` and `ViewportRenderProducts` for runtime/editor render request flow.
- `Engine/Application/Private/RuntimeConsole/RuntimeConsoleHost.cpp` currently uses `Renderer::GetImGuiRenderer` and `Renderer::GetRenderHardwareInterface`; this is an integration escape hatch to narrow later, not a diagnostic feature to grow.
- `Engine/Application/Private/ShaderRecook/ShaderRecookCoordinator.cpp` uses `Renderer::GetCommandSubmissionService`, `ReloadCookedShaders`, and `GetShaderPackageGeneration` to preserve safe cooked shader reload behavior.
- `Engine/Renderer/Private/FramePipeline/FramePipelineViewportProducts.cpp` owns renderer-to-RHI presentation and capture bridging.

Product-owned observation justification:

- Keep viewport products/presentation/capture because they are the renderer-facing API for editor/runtime view composition and screenshot/BMP capture.
- Keep shader reload result/generation because offline cooked shader packages with reflection data are preserved capabilities.
- Keep memory snapshots while feature work is still active because memory pressure connects directly to renderer productization, but do not expand it into reports or default diagnostics.
- Keep mesh/texture snapshots only as editor inspection data. They should not become runtime reporting systems.

Marked for later removal or narrowing:

- Move mesh and texture diagnostics behind an editor-owned adapter if runtime stops requiring public headers.
- Split `RendererCVars.h` into product rendering controls versus private diagnostics/profiling controls. Diagnostic marker verbosity and GPU timing should move late with performance/profiling stages.
- Replace `Renderer::GetRenderHardwareInterface`, `GetCommandSubmissionService`, and `GetImGuiRenderer` external usage with smaller services only where doing so deletes more public coupling than it adds.

Result:

- Stage 16 is an inventory-only stage; no renderer source code was added.
- Public diagnostic headers and public observation entrypoints are listed with concrete consumers.
- Editor-only observation surfaces are separated from product runtime behavior.
- Product-owned surfaces are justified by viewport rendering, screenshot capture, cooked shader reload, and memory/productization needs.

Net code pressure:

- No source code was added. The only change is this roadmap inventory.
- Follow-up stages must reduce public renderer coupling before adding any new renderer observation API.

Verification:

- `rg --files Engine/Renderer/Public` listed the renderer public header set.
- `rg -n "Diagnostic|Diagnostics|Stats|Metrics|Profiler|Profile|Capture|Screenshot|Snapshot|Debug|Report|Viewport|Observer|Observation|Inspector" Engine/Renderer/Public Engine/Renderer/Private Engine/Editor Tools Projects` found the candidate observation surface.
- `rg -n "RendererMemoryDiagnostics|CaptureMemoryDiagnostics|MeshDiagnostics|CaptureMeshDiagnostics|TextureDiagnostics|CaptureTextureDiagnostics|ViewportCapture|CaptureViewportProductToBmp|BeginViewportPresentation|ViewportRenderProducts|SubmitViewportRenderRequest|GetViewportRenderProducts|RenderViewMode|RendererCVars|CookedShaderReloadResult|ReloadCookedShaders|GetShaderPackageGeneration" Engine Tools Projects` identified the consumers.
- `rg -n "GetRenderHardwareInterface\\(|GetCommandSubmissionService\\(|GetImGuiRenderer\\(" Engine Tools Projects` identified low-level renderer escape-hatch consumers.

### Stage 17: Public RHI Observation Inventory

References: NV-NRI, NV-NVRHI, UE-SOURCE.

Prompt:

- Inventory RHI public diagnostics, descriptor snapshots, memory snapshots, capture surfaces.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] RHI public observation APIs are listed.
- [ ] Runtime-critical facts are separated from report/detail dumps.
- [ ] Capture is preserved and separated from broad diagnostics.
- [ ] D3D12/Vulkan parity risk is recorded.

Stage 17 inventory result:

Universal acceptance status:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

Specific acceptance status:

- [x] RHI public observation APIs are listed.
- [x] Runtime-critical facts are separated from report/detail dumps.
- [x] Capture is preserved and separated from broad diagnostics.
- [x] D3D12/Vulkan parity risk is recorded.

Public RHI observation APIs:

| Surface | Public owner | Current consumers | Classification | Stage 18/19 direction |
| --- | --- | --- | --- | --- |
| `RenderHardwareInterface::GetCapabilities`, `GetBackendApi`, `GetRequiredShaderBinaryFormat`, `GetCurrentFrameIndex` | `Engine/RHI/Public/Device/RenderHardwareInterface.h` | Renderer, application/runtime glue, shader package selection | Runtime-critical backend facts | Keep compact; do not expand into reports. |
| `RenderHardwareInterface::GetDiagnostics` and `Engine/RHI/Public/Diagnostics/RhiDiagnostics.h` | RHI public diagnostics service | Renderer diagnostics, backend failure paths, debug naming | Mixed surface: object names and capability flags are useful; messages, timing, live-object, crash, and memory report access are broad observation | Split by ownership later; product code should consume facts through narrow owners, not a general diagnostics cockpit. |
| `RenderObjectDiagnostics` | `RhiDiagnostics.h` | D3D12 and Vulkan debug names | Product/dev support for GPU object labels | Keep only as backend-owned debug-name capability. Do not expose as a general inspection API. |
| `RenderTimingDiagnostics` | `RhiDiagnostics.h` | `Engine/Renderer/Private/Diagnostics/FrameExecutionDiagnostics.cpp` and `ScopedGpuDiagnostics` | Performance observation, not first-priority feature work | Preserve behind capability checks, but defer slimming until after renderer/RHI/frame-graph feature work. |
| `RenderMessageDiagnostics` | `RhiDiagnostics.h` | D3D12 debug layer and Vulkan validation-message plumbing | Debug/validation observation | Keep backend-private or opt-in; do not surface as default product workflow. |
| `RenderFailureDiagnostics` | `RhiDiagnostics.h` | D3D12 resource/pipeline failure collection and live-object reporting | Failure-only backend support | Keep on-failure capability; remove any default report-style consumer. Vulkan correctly reports no support through capabilities. |
| `RenderMemoryDiagnostics` and `Engine/RHI/Public/Memory/RhiMemoryDiagnostics.h` | RHI public memory diagnostics service | `RendererMemoryMonitor`, `RendererMemoryDiagnosticsSnapshot` | Mixed surface: compact memory pressure facts are useful; allocator JSON dump and allocation-detail lists are report/detail dumps | Keep compact budget/category snapshot. Move/remove `WriteAllocatorJsonDump`, allocation-detail exposure, and environment-triggered dumps unless a product package consumes them. |
| `RhiDescriptorService::CaptureDescriptorUsageSnapshot` and `RhiDescriptorUsageSnapshot` | `Engine/RHI/Public/Descriptors/RhiDescriptorService.h` | No product consumer found outside backend implementations | Report/detail dump | Candidate for removal or private backend-only move. If kept, justify as runtime descriptor pressure fact in Stage 19. |
| `RenderHardwareInterface::GetCaptureService` and `Engine/RHI/Public/Capture/RhiCaptureService.h` | RHI capture service | `FramePipelineViewportProducts.cpp` through renderer-owned viewport capture | Product capability | Preserve. Keep separated from diagnostics. Users should enter through renderer/editor/tool capture, not broad RHI diagnostics. |
| `RhiPresentationService` and `RhiInteropService` | RHI public service interfaces | Renderer/application presentation and native interop needs | Runtime/product integration surfaces, not diagnostics | Keep scoped to presentation and explicit interop. Do not treat as observation/report APIs. |

Runtime-critical facts versus report/detail dumps:

- Keep runtime-critical facts: backend API, shader binary format, current frame index, capability flags, compact memory budget/category pressure, delayed-destruction pressure when consumed, capture request/result status, and explicit D3D12/Vulkan support flags.
- Treat as report/detail dumps: descriptor heap snapshots, allocator JSON dumps, full allocation maps, debug-message queues, timing query result reporting, live object reports, and crash diagnostic bundles. These are not first-class default workflows.
- Capture is not a dump. Screenshot/BMP capture is a preserved product/tool capability and must remain after diagnostics cleanup.

Consumer map:

- Memory pressure consumer: `Engine/Renderer/Private/Diagnostics/RendererMemoryMonitor.cpp` reads `RenderMemoryDiagnostics::GetLatestMemorySnapshot`.
- Timing consumer: `Engine/Renderer/Private/Diagnostics/FrameExecutionDiagnostics.cpp` uses timestamp query allocation/write/resolve through `RenderTimingDiagnostics`.
- Capture consumer: `Engine/Renderer/Private/FramePipeline/FramePipelineViewportProducts.cpp` calls `RhiCaptureService::CaptureTextureToBmp`.
- Descriptor snapshot consumer: none found outside D3D12/Vulkan descriptor service implementations.
- Failure diagnostics consumers: D3D12 backend failure paths call crash/live-object support internally; Vulkan exposes no failure diagnostics through its capability object.

D3D12/Vulkan parity risk:

- Capture parity exists and must be preserved: D3D12 has `D3D12CaptureService`, Vulkan has `VulkanCaptureService`, and both implement `CaptureTextureToBmp`.
- Descriptor snapshot parity exists but appears unconsumed: D3D12 and Vulkan both implement `CaptureDescriptorUsageSnapshot`; remove/private both together if Stage 19 does not keep descriptor pressure as a runtime fact.
- Memory diagnostics parity exists with backend-specific support flags: D3D12 and Vulkan both expose memory snapshots and JSON dump support through allocator-backed diagnostics. Capability flags must remain the authority; product code must not branch on backend names.
- Diagnostics parity is intentionally asymmetric: D3D12 exposes timestamp queries, debug-layer messages, live object reports, and crash diagnostics; Vulkan currently exposes object names, validation messages, memory diagnostics, and no timing/failure diagnostics. Any retained public surface must make unsupported backend behavior explicit through capabilities.

Net code pressure:

- No source code was added in this stage. The only change is this roadmap inventory.
- Stage 18 and Stage 19 must shrink public observation surfaces before adding any new RHI/renderer observation API.

Verification:

- `rg --files Engine/RHI/Public Engine/RHI/Private Engine/Renderer Engine/Application Engine/Editor Tools Projects | rg "(Diagnostic|Diagnostics|Stats|Metrics|Profile|Profiler|Capture|Memory|Descriptor|Snapshot|Report|Telemetry|Debug|Observation|Present|Presentation|Service|RenderHardwareInterface|Rhi)"` listed the candidate RHI observation files.
- `rg -n "CaptureDescriptorUsageSnapshot|RhiDescriptorUsageSnapshot|RhiDescriptorAllocatorUsage|GetLatestMemorySnapshot|WriteAllocatorJsonDump|SupportsJsonDump|RenderMemoryDiagnostics|GetMemoryDiagnostics|GetDiagnostics\\(\\)|GetCaptureService|CaptureTextureToBmp|RhiCaptureService|ReportLiveObjects|CollectCrashDiagnostics|TryPopMessage|AllocateTimestampQuery|WriteTimestamp|TryResolveTimestamp" Engine/RHI Engine/Renderer Engine/Application Engine/Editor Tools Projects -g "*.h" -g "*.cpp"` identified consumers and backend implementations.
- `Engine/RHI/Public/Diagnostics/RhiDiagnostics.h`, `Engine/RHI/Public/Memory/RhiMemoryDiagnostics.h`, `Engine/RHI/Public/Capture/RhiCaptureService.h`, `Engine/RHI/Public/Descriptors/RhiDescriptorService.h`, and `Engine/RHI/Public/Device/RenderHardwareInterface.h` were reviewed as public surfaces.
- `Engine/RHI/Private/D3D12/Diagnostics/D3D12RenderDiagnostics.cpp` and `Engine/RHI/Private/Vulkan/Diagnostics/VulkanRenderDiagnostics.cpp` were reviewed for backend parity and capability asymmetry.

### Stage 18: Preserve Editor Inspection And Narrow Diagnostics Ownership

References: UE-SOURCE, NV-DONUT.

Prompt:

- Preserve shader, mesh, and texture editor inspection windows as product-owned editor capabilities.
- Move editor-only diagnostics behind editor-private ownership where practical.
- Remove only unowned diagnostics panels; do not remove inspection windows that help understand renderer assets, residency, shader packages, or editor/runtime state.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Public Renderer/RHI diagnostics exposure is reduced, or remaining exposure is explicitly classified as editor inspection surface.
- [ ] Shader, mesh, and texture inspection windows remain available.
- [ ] No diagnostics facade is added.
- [ ] Runtime pressure facts remain only if consumed.

Stage 18 decision correction:

Universal acceptance status:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

Specific acceptance status:

- [x] Public Renderer/RHI diagnostics exposure is reduced, or remaining exposure is explicitly classified as editor inspection surface.
- [x] Shader, mesh, and texture inspection windows remain available.
- [x] No diagnostics facade is added.
- [x] Runtime pressure facts remain only if consumed.

Preserved editor inspection capabilities:

- `UsedShadersPanel` remains product-owned because it supports shader reload and recook workflows.
- `UsedMeshesPanel` remains product-owned because it lets the editor inspect mesh residency, draw/instance shape, material linkage, bounds, CPU/GPU byte estimates, and mesh batching behavior.
- `UsedTexturesPanel` remains product-owned because it lets the editor inspect texture residency, dimensions, format intent, mip count, estimated memory, default-vs-scene ownership, and SRV identity.
- `Windows > Shaders`, `Windows > Meshes`, and `Windows > Textures` remain valid editor workflows.

Corrected ownership rule:

- Editor inspection windows are capabilities, not scaffolding. They should be preserved.
- The cleanup target is not the existence of the panels. The cleanup target is excessive public renderer/RHI exposure, duplicated DTO ownership, fallback chains, and report-style detail surfaces that are not consumed by editor inspection.
- Mesh and texture inspection may keep renderer data extraction, but the API must be treated as an editor inspection surface rather than general runtime diagnostics.
- Future refactors should prefer narrowing names, ownership, and include paths over deleting the capability.

Future cleanup requirements for this area:

- Keep shader/mesh/texture inspection windows available after every Stage 18 follow-up batch.
- Do not add a generic diagnostics facade to hide the panels; ownership should stay direct and obvious.
- If a renderer public DTO exists only for editor inspection, either classify it under an editor/inspection-named renderer API or move the smallest possible data extraction boundary without making Renderer depend on Editor.
- Do not hardcode sample asset names, default level names, content-pack ids, or project names in inspection code. Inspection must reflect whatever the current level/content catalog loads.
- Avoid fallback chains in inspection panels. Missing data should show as unavailable/empty at the owning boundary rather than trying multiple hidden paths.
- Any cleanup in this area must be net code-negative or must remove equivalent duplicated panel/DTO/helper code in the same batch.

Stage 18 implementation result:

- Moved `UsedShadersPanel.h`, `UsedMeshesPanel.h`, and `UsedTexturesPanel.h` from `Engine/Editor/Public/Panels` to `Engine/Editor/Private/Panels`.
- Kept `UsedShadersPanel.cpp`, `UsedMeshesPanel.cpp`, and `UsedTexturesPanel.cpp` unchanged in behavior; the windows remain available from the editor `Windows` menu.
- Kept `UI.h` as the public editor API boundary with forward declarations only. External editor API users no longer see the concrete inspection panel headers.
- Did not add a diagnostics facade, wrapper, fallback chain, or replacement panel.
- Did not change renderer capture or renderer diagnostic DTO behavior in this batch. Those remain classified as editor inspection surfaces until a later net-negative narrowing pass can move or rename the smallest useful boundary.

Code ownership result:

- Shader, mesh, and texture inspection are now more clearly editor-private UI implementation details.
- The preserved public editor contract remains the high-level `UI` entrypoint and `EditorDiagnosticsProviders`.
- Renderer public diagnostic DTOs are still present only because the editor inspection windows consume them; Stage 19/20 may further narrow those names or ownership, but must not remove the windows.

Verification:

- `rg -n "UsedMeshesPanel|UsedTexturesPanel|CaptureMeshDiagnostics|CaptureTextureDiagnostics|MeshDiagnostics|TextureDiagnostics" Engine -g "*.h" -g "*.cpp"` confirms the shader/mesh/texture inspection path is present.
- `rg -n "Panels/UsedShadersPanel.h|Panels/UsedMeshesPanel.h|Panels/UsedTexturesPanel.h|class UsedShadersPanel|class UsedMeshesPanel|class UsedTexturesPanel" Engine -g "*.h" -g "*.cpp"` confirms private implementation includes and public `UI.h` forward declarations.
- `Test-Path` confirms the three inspection panel headers now live under `Engine/Editor/Private/Panels` and no longer live under `Engine/Editor/Public/Panels`.
- Source deletion from the previous Stage 18 attempt was reverted because it removed a desired editor capability.
- No source-code replacement layer was added.

### Stage 19: Compact Runtime Pressure Facts

References: NV-NVRHI, AMD-D3D12MA, AMD-VMA.

Prompt:

- Keep compact memory/descriptor pressure facts only when they drive runtime policy.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Memory budget facts are backed by D3D12MA/VMA paths.
- [ ] Descriptor pressure facts are consumed or removed.
- [ ] Public detail dumps are removed from default API.
- [ ] No report format is added.

### Stage 20: RHI Service Boundary Audit

References: NV-NRI, NV-NVRHI, UE-SOURCE.

Prompt:

- Audit `RenderHardwareInterface` services and ownership.
- Keep RHI explicit and thin.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Resource, descriptor, pipeline, upload, ray tracing, interop, capture, diagnostics, presentation services are still owned by RHI.
- [ ] Backend-native details remain private or provider-bridged.
- [ ] No new RHI wrapper layer is added.
- [ ] Public service surface is smaller or justified.

### Stage 21: D3D12/Vulkan Parity Matrix

References: AMD-CAULDRON, NV-NRI, UE-SOURCE.

Prompt:

- Create a working parity matrix in existing docs or issue notes for key RHI/Renderer features.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] D3D12/Vulkan parity is named for resources, descriptors, pipelines, uploads, presentation, ray tracing, PTLAS, capture.
- [ ] Backend-specific gaps are listed.
- [ ] No parity gap is hidden behind generic abstraction.
- [ ] Any extension opportunity is recorded.

### Stage 22: RHI Resource/Descriptor/Pipeline Cleanup

References: NV-NRI, NV-NVRHI, AMD-CAULDRON.

Prompt:

- Remove duplicate helper paths while preserving explicit API behavior.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Resource creation path remains backend-owned.
- [ ] Descriptor binding model remains understandable.
- [ ] Pipeline/layout creation remains explicit.
- [ ] Any removed helper had no unique product behavior.

### Stage 23: Native Interop Boundary

References: NV-NVRHI, NV-STREAMLINE, UE-SOURCE.

Prompt:

- Keep native interop only for explicit provider bridges.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Native handle access has explicit consumer ownership.
- [ ] Provider resource contracts remain narrow.
- [ ] No broad native escape hatch is exposed casually.
- [ ] Streamline/upscaling/ray reconstruction paths remain functional where supported.

### Stage 24: Shader Compiler/Cook ABI Audit

References: NV-SHADERMAKE, AMD-CAULDRON, UE-SOURCE.

Prompt:

- Audit shader source-to-cooked-package-to-runtime flow.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Shader source includes are tracked.
- [ ] DXC/Slang target outputs are known.
- [ ] Reflection data survives cook.
- [ ] Runtime package cache loads cooked packages.
- [ ] Debug artifact behavior is opt-in or removed.

### Stage 25: Shader Registration And Binding Duplication Cleanup

References: NV-SHADERMAKE, NV-NVRHI, UE-SOURCE.

Prompt:

- Reduce duplicated C++ registration/HLSL binding declarations only when net code decreases.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Duplicate declarations are listed.
- [ ] Any simplification deletes more code than it adds.
- [ ] No code generator is added unless net code decreases materially.
- [ ] Layout verification remains intact.

### Stage 26: Classic TLAS Flow Audit

References: NV-NRI, UE-SOURCE, NV-RTXPT.

Prompt:

- Trace classic BLAS/TLAS build/update/trace ownership.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] BLAS lifecycle owner is clear.
- [ ] Classic TLAS lifecycle owner is clear.
- [ ] Trace pipeline usage is clear.
- [ ] D3D12/Vulkan paths are both represented where supported.

### Stage 27: PTLAS Minimal Reference Flow

References: NV-NRI, UE-SOURCE, NV-RTXPT.

Prompt:

- Reduce PTLAS to capability check, compact descriptor input, backend build/update, resource lifetime, trace use.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] PTLAS planner metrics not required for product behavior are removed.
- [ ] CPU validation readbacks not required for product behavior are removed.
- [ ] Future GPU-pack placeholders are removed.
- [ ] PTLAS still renders where supported.

### Stage 28: TLAS/PTLAS User Selection

References: NV-NRI, UE-SOURCE.

Prompt:

- Preserve user-facing selection between classic TLAS and PTLAS.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Selection is available through one clear policy point.
- [ ] Unsupported backend/feature state fails gracefully or falls back explicitly.
- [ ] No debug panel is added for selection.
- [ ] Selection code is smaller or clearer than before.

### Stage 29: PTLAS D3D12/Vulkan Backend Parity

References: AMD-CAULDRON, NV-NRI, UE-SOURCE.

Prompt:

- Preserve or extend PTLAS capability across D3D12 and Vulkan.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] D3D12 PTLAS support path is listed and functional where supported.
- [ ] Vulkan PTLAS support path is listed and functional where supported.
- [ ] Capability checks are explicit per backend.
- [ ] PTLAS is not made D3D12-only if Vulkan support exists.

### Stage 30: Reference Path Tracing Role

References: NV-RTXPT, AMD-CAULDRON.

Prompt:

- Choose debug reference as the first clear role.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Reference path has one sentence role.
- [ ] Provider handoff hooks unsupported by data are removed.
- [ ] Guide outputs without consumers are removed.
- [ ] Material/light policy aligns with realtime path.

### Stage 31: Reservoir Direct Lighting Cleanup

References: NV-RTXDI, NV-RTXDI-LIBRARY if used, NV-SHARC.

Prompt:

- Keep native reservoir-based direct lighting honest and owned by Sparkle.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Direct lighting is not described as SDK-equivalent unless SDK is integrated.
- [ ] Light buffers, GBuffer addressing, TLAS, material model, and shader scheduling remain Sparkle-owned.
- [ ] Unused debug views or report outputs are removed.
- [ ] Shader/resource ownership is obvious.

### Stage 32: Post-Processing Pass Ownership

References: AMD-CAULDRON, NV-DONUT.

Prompt:

- Make post-processing pass inputs/outputs explicit and reduce duplicate pass plumbing.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Each post-process pass declares inputs/outputs clearly.
- [ ] Unused settings/CVars are removed.
- [ ] No new post-processing framework is added.
- [ ] Existing output quality path remains buildable.

### Stage 33: Denoising Feature Boundary

References: NV-NRD, NV-NRD-SAMPLE, AMD-FIDELITYFX.

Prompt:

- Treat denoising as a renderer feature slice with explicit resources.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Denoiser inputs/outputs are named.
- [ ] History/resource ownership is explicit.
- [ ] Provider boundary is narrow if external denoiser integration exists.
- [ ] No diagnostic panel is added.

### Stage 34: Upscaling And Ray Reconstruction Boundary

References: NV-STREAMLINE, AMD-FIDELITYFX.

Prompt:

- Preserve upscaling/ray reconstruction provider capability while trimming diagnostics and fallback scaffolding.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Provider required resources are narrow and named.
- [ ] Depth, motion vectors, exposure, history, jitter, frame index, and camera state ownership is clear where used.
- [ ] Unused fallback/provider objects are removed.
- [ ] Streamline bridge remains narrow.

### Stage 35: Frame Graph Pass/Resource Ownership

References: UE-SOURCE, NV-DONUT, NV-NVRHI.

Prompt:

- Reduce frame graph/pass duplication without replacing the graph.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Frame graph remains the only render scheduling abstraction.
- [ ] Pass inputs/outputs/history dependencies are clearer.
- [ ] Transient/persistent resource ownership is easier to trace.
- [ ] No replacement render graph is added.

### Stage 36: Core Public API Cleanup

References: UE-SOURCE, NV-DONUT.

Prompt:

- Reduce public Core convenience APIs that are not stable contracts.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Public Core line count decreases.
- [ ] Private helpers move private.
- [ ] No replacement aggregate header appears.
- [ ] Platform/Renderer/GameFramework includes remain clean.

### Stage 37: GameFramework Public API Cleanup

References: UE-SOURCE, NV-DONUT.

Prompt:

- Keep GameFramework high-level user concepts while hiding implementation details.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Level, scene, component, asset concepts remain.
- [ ] Asset loader/manifest implementation details are private where possible.
- [ ] Multi-level support remains intact.
- [ ] Renderer still consumes GameFramework privately.

### Stage 38: Import/Cooker Public Surface Cleanup

References: AMD-CAULDRON, AMD-FIDELITYFX.

Prompt:

- Keep tool public headers only when another executable consumes them as stable API.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Import/cooker public headers are audited.
- [ ] Unused public bridge headers are removed.
- [ ] Cooked asset outputs remain.
- [ ] Default reports do not return.

### Stage 39: Neural Rendering Readiness Without ML Bloat

References: NV-RTXNS, NV-RTX-KIT, NV-NRD, NV-STREAMLINE.

Prompt:

- Prepare for inference-like shader features without adding runtime ML frameworks.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Slang/HLSL flexibility is preserved.
- [ ] Tensor/operator concepts remain design-level unless a renderer feature needs them.
- [ ] No PyTorch/TensorFlow/ONNX Runtime dependency is added.
- [ ] Denoising/upscaling/ray reconstruction paths remain the practical readiness surface.

### Stage 40: Package Contract Implementation

References: NV-STREAMLINE, AMD-FIDELITYFX.

Prompt:

- Make package outputs real and intentional.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Runtime/editor/launcher/dev tools/symbols/optional content package ownership is decided.
- [ ] Unowned package assembly code is removed.
- [ ] Optional content is separate from runtime package.
- [ ] Manifest/checksum fields are consumed or removed.

### Stage 41: Late Measurement Setup

References: AMD-CAULDRON, NV-NRI, UE-SOURCE.

Prompt:

- Use existing markers/timestamps/debuggers before adding any measurement code.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Feature cleanup is complete for the measured path.
- [ ] Existing PIX/RenderDoc/Nsight hooks are used first.
- [ ] Any new measurement code replaces old diagnostics.
- [ ] No new report panel or benchmark format appears by default.

### Stage 42: Final Stabilization And Persona Evidence

References: all reference repos above.

Prompt:

- Run final stabilization after the staged cleanup sequence.
- Confirm the repo demonstrates the persona.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Build relevant editor/runtime targets.
- [ ] Cook curated default level set.
- [ ] Run default level set.
- [ ] Run D3D12 path where supported.
- [ ] Run Vulkan path where supported.
- [ ] Verify shader packages cook and load with reflection data.
- [ ] Verify screenshot/BMP capture.
- [ ] Verify classic TLAS selection.
- [ ] Verify PTLAS selection where supported.
- [ ] Verify multiple levels remain selectable.
- [ ] Confirm public APIs are smaller.
- [ ] Confirm repo/depot weight is smaller.
- [ ] Confirm no new docs/logs/validation/report systems/wrappers/thick abstractions replaced old ones.

## Phase Note 0: Baseline And Guardrails

Goal:

- establish what must be preserved before changing code
- stop accidental deletion of important capabilities

Use:

- `01_KEEP_PreservedCapabilities.md`
- `H_AdvancedGraphicsEngineerPersona.md`
- `G_AdvancedGraphicsEngineExecutiveSummary.md`

Implementation prompt:

1. Read `01_KEEP_PreservedCapabilities.md`.
2. For the subsystem you want to touch, list preserved capabilities from the KEEP doc.
3. Run discovery searches before editing.
4. Save the final build/cook/run commands you will use in Stage 42.

Baseline searches:

```powershell
rg -n "D3D12|Vulkan|PTLAS|PartitionedTlas|CaptureTextureToBmp|RhiCaptureService|debug-artifacts|asset-cooker-summary|asset-cooker-plan|SmokeDiagnostics|SPARKLE_SMOKE" Engine Tools Projects
rg -n "RendererMemoryDiagnostics|MeshDiagnostics|TextureDiagnostics|RhiDiagnostics|DescriptorUsage|ShaderStats|ComputeClear|HelloWorld" Engine Tools
```

Must preserve:

- D3D12 backend
- Vulkan backend
- D3D12/Vulkan parity preserved or extended
- frame graph
- offline cooked shader packages with reflection data
- classic TLAS
- PTLAS
- screenshot/BMP capture
- multi-level support
- PIX/RenderDoc/Nsight markers
- final build/cook/run workflows

Done criteria:

- [ ] You know the verification command for the target subsystem.
- [ ] You know the final build/cook/run commands for Stage 42.
- [ ] You know which KEEP items the batch must preserve.
- [ ] You have identified whether the batch is MODIFY, ADD, REMOVE, or mixed.

## Phase Note 1: Repository And Documentation Hygiene

Goal:

- make the docs navigable
- keep `01-Implementation` as the active execution spine
- keep `00-Review` as source material
- remove stale review references

Use:

- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 10
- `Docs/README.md`

Implementation prompt:

1. Keep `Docs/README.md` pointing to this roadmap first.
2. Keep the four implementation docs as the active prompt set.
3. Keep review docs as source material only.
4. Remove stale references to deleted docs.
5. Do not create another planning layer.

Search patterns:

```powershell
rg -n "B_RendererReleaseReadinessMap|C_ValidationDiagnosticsCleanupMap|stale review doc|deleted review doc|source trail" Docs
```

Done criteria:

- [ ] README points to `00_ORDERED_ImplementationRoadmap.md` first.
- [ ] No stale links to deleted review docs remain.
- [ ] No sensitive source trail remains.
- [ ] No new planning document is created after this roadmap.

## Phase Note 2: Content Catalog And Optional Heavy Packs

Goal:

- preserve multiple levels
- reduce depot weight
- stop heavy content from polluting default build/cook/run

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 1
- `03_ADD_MinimalMissingCapabilities.md`, Allowed Addition 1
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 1
- `01_KEEP_PreservedCapabilities.md`, multi-level support guardrails

Implementation prompt:

1. Inspect current Showcase project and level discovery.
2. Identify the curated default level set.
3. Add the smallest catalog/manifest needed to describe levels and optional packs.
4. Teach launcher/cooker/runtime to select available levels through that metadata.
5. Mark heavyweight content as optional.
6. Move/delete heavy content only after default levels still run.

Target paths:

- `Projects/Showcase`
- `Projects/Showcase/Assets/Meshes/Bistro`
- large DDS/TGA/HDR assets
- launcher project selection
- cooker project/level discovery

Search patterns:

```powershell
rg -n "Showcase|Level|Scene|Bistro|\\.hdr|\\.tga|\\.dds|Cook" Projects Tools Engine
```

Allowed additions:

- minimal level/content catalog
- optional pack id/root/availability metadata
- resolver logic only if it enables content removal

Forbidden additions:

- asset database rewrite
- content browser rewrite
- package manager
- network downloader unless already product-owned

Done criteria:

- [ ] Multiple levels remain selectable.
- [ ] Default level set cooks and launches without optional packs.
- [ ] Missing optional packs fail gracefully.
- [ ] Heavy content can be removed from the core repo.
- [ ] Depot bytes decrease materially.

## Phase Note 3: Screenshot/BMP Capture Hardening

Goal:

- preserve screenshot/BMP capture
- remove smoke/ad hoc ownership
- narrow API cost

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 2
- `03_ADD_MinimalMissingCapabilities.md`, Allowed Addition 3
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 2
- `01_KEEP_PreservedCapabilities.md`, capture guardrails

Implementation prompt:

1. Find all capture call sites.
2. Identify which are smoke/ad hoc paths.
3. Preserve or create one product-owned editor/tool capture entrypoint.
4. Keep RHI/BMP writer implementation only behind the hardened path.
5. Remove smoke-owned capture.

Target paths:

- `Engine/RHI/Public/Capture/RhiCaptureService.h`
- `Engine/RHI/Private/Capture/RhiBmpWriter.*`
- smoke/ad hoc capture call sites
- editor/tool capture command paths

Search patterns:

```powershell
rg -n "CaptureTextureToBmp|RhiCaptureService|RhiBmpWriter|Screenshot|BMP|Smoke" Engine Tools
```

Done criteria:

- [ ] Capture still writes BMP through intended path.
- [ ] Smoke/ad hoc capture ownership is gone.
- [ ] Public capture surface is smaller or clearly product-owned.
- [ ] D3D12/Vulkan readback behavior remains intact where supported.

## Phase Note 4: Smoke, Validation, Cook Report, And Debug Artifact Cleanup

Goal:

- remove default-path validation/report/debug artifact behavior
- preserve fatal guardrails and real product outputs

Use:

- `04_REMOVE_DeletionsAndCleanup.md`, Remove Categories 2 and 3
- `02_MODIFY_RefactorExistingSystems.md`, sections 2 and 7

Implementation prompt:

1. Remove application-level smoke validation if still present.
2. Remove launcher smoke orchestration if still present.
3. Remove renderer smoke snapshots.
4. Remove AssetCooker plan/timing/summary artifacts from default cook.
5. Remove shader debug artifacts from default launcher/cook.
6. Keep fatal errors and product outputs.

Target paths:

- `Engine/Application/Private/Validation/RhiSmoke*`
- `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/*`
- `Engine/Renderer/Public/Diagnostics/*Smoke*`
- `Tools/Cooking/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp`
- `Tools/Cooking/AssetCooker/Private/Discovery/AssetCookerDiscovery.*`
- shader debug artifact writers

Search patterns:

```powershell
rg -n "RhiSmoke|SmokeDiagnostics|SPARKLE_SMOKE|asset-cooker-plan-v1|asset-cooker-summary-v1|debug-artifacts|timing summary|dry-run" Engine Tools
```

Done criteria:

- [ ] Default cook writes assets, not reports.
- [ ] Shader cook writes runtime packages, not debug bundles.
- [ ] Runtime/editor launch still works.
- [ ] PIX/RenderDoc/Nsight markers remain.
- [ ] Deleted files/lines greatly exceed added lines.

## Phase Note 5: Launcher Workflow Shell

Goal:

- reduce launcher to product workflows
- stop it being a diagnostic cockpit

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 3
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 7

Implementation prompt:

1. Identify launcher workflows actually owned: build, cook, run, clean, package if real.
2. Remove diagnostic-only pages/actions.
3. Remove status pages not tied to product workflow.
4. Remove shader debug/stat toggles from default GUI.
5. Remove package UI for unowned packages.
6. Keep command paths for real workflows.

Target paths:

- `Tools/Launcher/SparkleLauncher/Private/Shell/LauncherShell.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Gui/*`
- option pages
- operation/status pages
- package workflow files
- shader/debug option code

Search patterns:

```powershell
rg -n "Status|Diagnostic|DebugArtifact|ShaderStats|Package|Operation|DryRun|Quality|Smoke" Tools/Launcher/SparkleLauncher
```

Done criteria:

- [ ] Launcher source decreases.
- [ ] Build/cook/run/clean still work.
- [ ] Package remains only if product-owned.
- [ ] No new panel replaces removed panels.

## Phase Note 6: Public Renderer/RHI Observation API Narrowing

Goal:

- shrink public APIs around behavior instead of observation
- keep compact facts only if they drive runtime policy

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 4
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 4
- `01_KEEP_PreservedCapabilities.md`, RHI/Renderer guardrails

Implementation prompt:

1. List public diagnostic/observation headers.
2. Identify real consumers.
3. Move editor-only observation private or delete unowned panels.
4. Collapse memory/descriptor snapshots into compact status only if consumed by policy.
5. Preserve debugger markers, timestamps, object names, fatal diagnostics, and capture.

Target paths:

- `Engine/Renderer/Public/Renderer.h`
- `Engine/Renderer/Public/Diagnostics/*`
- `Engine/Renderer/Public/Meshes/MeshDiagnostics.h`
- `Engine/Renderer/Public/Resources/Textures/TextureDiagnostics.h`
- `Engine/RHI/Public/Diagnostics/*`
- `Engine/RHI/Public/Descriptors/RhiDescriptorService.h`
- editor panels consuming diagnostics

Search patterns:

```powershell
rg -n "RendererMemoryDiagnostics|MeshDiagnostics|TextureDiagnostics|RhiDiagnostics|DescriptorUsage|Snapshot|Dump|Json|Panel" Engine
```

Done criteria:

- [ ] Public renderer/RHI header count or line count decreases.
- [ ] Editor still has only product-owned panels.
- [ ] No new diagnostics facade appears.
- [ ] Runtime pressure facts remain if used by policy.

## Phase Note 7: Classic TLAS And PTLAS Refactor

Goal:

- preserve both classic TLAS and PTLAS
- minimize PTLAS toward original reference-style flow
- preserve D3D12 and Vulkan support where available

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 5
- `03_ADD_MinimalMissingCapabilities.md`, Allowed Addition 2
- `03_ADD_MinimalMissingCapabilities.md`, Allowed Addition 4 if capability facts are needed
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 5
- `01_KEEP_PreservedCapabilities.md`, ray tracing guardrails

Implementation prompt:

1. Trace current classic TLAS and PTLAS build/update/trace flows.
2. Identify strategy selection points.
3. Preserve selection between classic TLAS and PTLAS.
4. Remove future GPU-pack placeholders.
5. Remove PTLAS planner metrics/diagnostics not needed for build/update/trace.
6. Remove CPU validation readbacks not required for product behavior.
7. Collapse PTLAS toward capability check, descriptor input, backend call, lifetime, trace use.

Target paths:

- `Engine/Renderer/Private/RayTracing/Acceleration/RayTracingPartitionedTlasStrategy.*`
- `Engine/Renderer/Private/RayTracing/Acceleration/RayTracingPtlas*`
- `Engine/Renderer/Private/RayTracing/Diagnostics/RayTracingPtlas*`
- `Engine/RHI/Public/RayTracing/RhiPartitionedTlasDesc.h`
- D3D12 PTLAS services
- Vulkan PTLAS services

Search patterns:

```powershell
rg -n "PTLAS|Ptlas|PartitionedTlas|future GPU pack|placeholder|AllowCpuValidationReadback|TLAS|BLAS" Engine
```

Done criteria:

- [ ] Classic TLAS renders.
- [ ] PTLAS renders where supported.
- [ ] User selection can choose classic TLAS or PTLAS.
- [ ] D3D12 and Vulkan capability checks remain honest.
- [ ] PTLAS code is smaller and more direct.
- [ ] No future GPU-pack scaffolding remains in default path.

## Phase Note 8: Reference Path Tracing Role Cleanup

Goal:

- make reference path tracing honest and scoped
- avoid a second ambiguous realtime renderer

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 6
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 8
- `G_AdvancedGraphicsEngineExecutiveSummary.md`, Ray Tracing/GI/Path Tracing section

Implementation prompt:

1. Choose debug reference as the first role.
2. Remove unsupported provider handoff hooks.
3. Remove guide outputs not consumed by product features.
4. Remove settings that imply unsupported realtime product quality.
5. Share BRDF/material/light policy with realtime path.

Target areas:

- reference frame path
- reference passes
- reference shaders
- provider handoff code
- reference settings/CVars

Search patterns:

```powershell
rg -n "Reference|PathTrace|PathTracing|Guide|RayReconstruction|MotionVector|Accumulation|Progressive" Engine/Renderer Engine/Assets/Shaders
```

Done criteria:

- [ ] Reference mode has one sentence role.
- [ ] Dead provider handoff is gone.
- [ ] Reference buffers/settings are fewer or explicitly consumed.
- [ ] Material/light policy aligns with realtime path.

## Phase Note 9: Shader Debug And Duplication Cleanup

Goal:

- preserve shader compiler/cook/runtime ABI
- remove debug/demo bloat from default workflows

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 7
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 6
- `01_KEEP_PreservedCapabilities.md`, Shader Pipeline section

Implementation prompt:

1. Identify default shader debug artifacts.
2. Remove debug artifact generation from default cook.
3. Remove cooked stats CSV from default workflow unless actively consumed.
4. Remove validation-only shader registrations from product runtime.
5. Preserve reflection, layout safety, package cooking, runtime package cache.

Target paths:

- `Tools/Shaders/ShaderCompiler`
- `Tools/Shaders/ShaderContracts`
- `Engine/Renderer/ShaderRegistrations`
- launcher shader options
- shader debug artifact writers

Search patterns:

```powershell
rg -n "debug-artifacts|ShaderStats|stats CSV|HelloWorld|ComputeClear|artifact|reflection|package" Engine Tools
```

Done criteria:

- [ ] Default shader cook emits runtime packages only.
- [ ] Debug bundles are opt-in or removed.
- [ ] Shader packages still load at runtime.
- [ ] Renderer shaders still compile.

## Phase Note 10: Core And GameFramework Public Surface Cleanup

Goal:

- reduce public engine surface
- keep stable runtime contracts

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 8
- `D_WholeRepositoryArchitectureMap.md`, Public Versus Private API Shape

Implementation prompt:

1. Identify public Core and GameFramework headers that are implementation details.
2. Move private-only helpers private.
3. Remove unused utility APIs.
4. Narrow asset loader and scene manifest exposure.
5. Keep level/scene/component/asset types projects actually use.

Target paths:

- `Engine/Core/Public`
- `Engine/GameFramework/Public`
- `Engine/GameFramework/Private`
- cooked scene loaders
- asset loader/scene manifest types

Search patterns:

```powershell
rg -n "Manifest|Loader|Scene|Level|Component|Asset|Utils|Helper" Engine/Core Engine/GameFramework
```

Done criteria:

- [ ] Public Core/GameFramework lines decrease.
- [ ] Cooked scene still loads.
- [ ] No replacement aggregate header appears.
- [ ] Renderer still consumes GameFramework privately.

## Phase Note 11: Renderer Feature Hardening Before Measurement

Goal:

- make the feature surface worth profiling
- harden renderer features without adding measurement systems

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 9
- `03_ADD_MinimalMissingCapabilities.md`, Allowed Addition 5
- `H_AdvancedGraphicsEngineerPersona.md`, Renderer Feature Depth and Shader/Kernel Craft

Implementation prompt:

1. Pick one feature area: RT, GI/path tracing, post-processing, denoising, upscaling, frame graph, shaders, or passes.
2. Identify old ambiguity or duplicate ownership.
3. Implement only a vertical slice that replaces or simplifies old code.
4. Keep pass inputs/outputs obvious.
5. Preserve existing debugger/profiler hooks.
6. Do not add benchmarking/reporting systems.

Target areas:

- `Engine/Renderer/Private/Passes`
- `Engine/Renderer/Private/FrameGraph`
- `Engine/Renderer/Private/Frame`
- `Engine/Renderer/Private/Upscaling`
- `Engine/Renderer/Private/RayReconstruction`
- `Engine/Renderer/Private/RayTracing`
- `Engine/Assets/Shaders/Passes`

Search patterns:

```powershell
rg -n "Denois|Upscal|RayReconstruction|Post|Composite|FrameGraph|Pass|Resource|History|Provider|Fallback" Engine/Renderer Engine/Assets/Shaders
```

Done criteria:

- [ ] Feature-facing renderer/RHI code is smaller or simpler.
- [ ] No new docs/logs/validation/report systems appear.
- [ ] Affected feature remains buildable.
- [ ] Existing markers/timestamps/capture remain.

## Phase Note 12: Package Contract Cleanup

Goal:

- keep only real package outputs
- separate optional content from runtime package

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 10
- `03_ADD_MinimalMissingCapabilities.md`, Allowed Addition 6
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 9

Implementation prompt:

1. Decide which packages are real now: runtime, editor, launcher, dev tools, symbols, optional content.
2. Remove package assembly code for unowned outputs.
3. Remove future manifest fields not consumed.
4. Remove release note generation not consumed.
5. Keep checksums/manifests only if workflow consumes them.
6. Keep optional content out of runtime package.

Target paths:

- `CMake/SparkleReleaseAssembly.cmake`
- launcher package workflow files
- artifact contract CMake files

Search patterns:

```powershell
rg -n "Package|Manifest|Release|Checksum|Artifact|Symbols|ContentPack|Assembly" CMake Tools
```

Done criteria:

- [ ] One package command produces only owned outputs.
- [ ] Package/build code decreases or package output bytes decrease.
- [ ] Optional content is separate from runtime package.

## Phase Note 13: Late Profiling And Measurement

Goal:

- profile only after feature cleanup
- use professional tools and existing hooks first

Use:

- `03_ADD_MinimalMissingCapabilities.md`, Late Additions Only
- `G_AdvancedGraphicsEngineExecutiveSummary.md`, Late CPU/GPU Performance Evidence
- `D_WholeRepositoryArchitectureMap.md`, CPU/GPU Performance Maps

Implementation prompt:

1. Verify stages 2 through 12 are substantially complete for the target area.
2. Use PIX/RenderDoc/Nsight and existing markers first.
3. Use existing timestamps before adding measurement systems.
4. Add measurement code only if it replaces multiple old diagnostics.
5. Keep net code non-positive unless explicitly approved.

Measurement targets:

- CPU frame setup/compile
- scene snapshot/build cost
- texture loading
- shader package load
- renderer submit
- GPU pass timings
- ray tracing build/update timings
- memory budget/transient pressure
- descriptor allocator occupancy
- pipeline/shader package count

Done criteria:

- [ ] Before/after evidence exists.
- [ ] No new log/report panel appears.
- [ ] Existing markers/timestamps remain.
- [ ] Measurement code replaces old diagnostics or remains very small.

## Phase Note 14: Final Readiness Pass

Goal:

- confirm the full scope is covered
- confirm persona and engine direction are real in the repo

Use:

- all four implementation docs
- `H_AdvancedGraphicsEngineerPersona.md`
- `G_AdvancedGraphicsEngineExecutiveSummary.md`
- `D_WholeRepositoryArchitectureMap.md`

Final checks:

- [ ] Core repo size is much smaller.
- [ ] Multi-level project support remains intact.
- [ ] Public Renderer/RHI/Core/GameFramework API is smaller.
- [ ] Launcher is workflow-focused.
- [ ] Default cook writes assets, not diagnostics.
- [ ] Default shader cook writes runtime packages, not debug bundles.
- [ ] Classic TLAS and PTLAS are both product-owned and selectable where supported.
- [ ] PTLAS is smaller and closer to reference-style flow.
- [ ] Reference path has a clear role.
- [ ] Frame graph remains the one render scheduling abstraction.
- [ ] RHI remains explicit.
- [ ] D3D12 and Vulkan remain first-class, with parity preserved or extended.
- [ ] Screenshot/BMP capture remains preserved and hardened.
- [ ] PIX/RenderDoc/Nsight support remains strong.
- [ ] Heavy optional content is out of the default footprint.
- [ ] No new docs/logs/validation/report systems/wrappers/scaffolding have replaced old ones.
- [ ] The next feature can delete or replace something old.

Final stabilization checks:

- [ ] Build the relevant editor/runtime targets.
- [ ] Cook the curated default level set.
- [ ] Run the default level set.
- [ ] Run both D3D12 and Vulkan paths where the local machine supports them.
- [ ] Verify shader packages cook and load with reflection data.
- [ ] Verify screenshot/BMP capture through the hardened path.
- [ ] Verify classic TLAS and PTLAS selection where supported.
- [ ] Verify multiple levels remain selectable.

## Weekly Working Pattern

A sustainable cadence:

1. Monday: choose one roadmap stage and fill the batch prompt.
2. Tuesday-Wednesday: inspect code and implement the smallest meaningful batch.
3. Thursday: run targeted checks for the edited subsystem and record any final stabilization risk.
4. Friday: remove leftover dead code, update only stale existing docs, and record the next batch.

Keep each batch small enough to review. The plan can take weeks; that is fine. The important thing is that every completed batch leaves Sparkle smaller, clearer, and closer to the advanced graphics engineer persona.

## What To Open First

For a normal implementation session, open in this order:

1. `00_ORDERED_ImplementationRoadmap.md`
2. `01_KEEP_PreservedCapabilities.md`
3. the exact section in `02_MODIFY_RefactorExistingSystems.md`, `03_ADD_MinimalMissingCapabilities.md`, or `04_REMOVE_DeletionsAndCleanup.md`
4. the source review doc only if you need background:
   - current state: `D_WholeRepositoryArchitectureMap.md`
   - external comparison: `E_ExternalRendererRepositoryComparison.md`
   - persona: `H_AdvancedGraphicsEngineerPersona.md`
   - executive priority: `G_AdvancedGraphicsEngineExecutiveSummary.md`

Do not open every doc every time. Use this roadmap as the index, then jump to the exact implementation prompt.
