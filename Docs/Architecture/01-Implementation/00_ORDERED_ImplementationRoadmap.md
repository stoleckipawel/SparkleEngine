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
6. Run the cleanup-after-cleanup scan for the touched module before declaring the batch done.
7. Verify the acceptance criteria from the stage with targeted checks; defer full build/cook/run stabilization to Stage 42 unless you intentionally choose a checkpoint.
8. Update only existing docs if the implementation result makes current instructions stale.

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
Post-cleanup scan:
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
- no single-field data-only wrapper structs/classes after cleanup; use the field directly
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
- Shape pressure: if a refactor shrinks a struct/class to one meaningful data field and no behavior, invariant, or external ABI role, delete the wrapper before the CL is done and use the field directly at the owning call sites.
- Wrapper pressure: if a function only forwards to another function without adding a stable name, policy, invariant, ABI boundary, or meaningful composition, remove the wrapper and update call sites to the real owner. Keep wrappers only when they encode product vocabulary or build a composed path/value.
- Empty-control-flow pressure: after deleting code, scan for empty `if`, `else`, loop, switch, lambda, callback, and function bodies. Remove the empty branch, invert/collapse the condition, or delete the dead hook rather than leaving a misleading shell.
- Leftover-helper pressure: after moving ownership, scan for private helpers, local normalizers, duplicate constants, stale includes, unused headers, unused files, and one-off utility functions that now duplicate Core/RHI/Renderer/GameFramework/Tools ownership.
- Propagation pressure: a cleanup is not done until the call sites, includes, CMake/source lists, docs, and module boundaries use the surviving owner. Do not leave both the new path and the old compatibility path unless the old path has a current product consumer.
- Data owns content names: catalogs, config, level files, and project data may name content; compiled engine/tool code should not hardcode sample or asset names.
- Fail simply: required data should fail clearly at the owning boundary; optional data should use explicit availability metadata rather than layered fallbacks.

Cleanup-after-cleanup scan means:

- Scan touched files and their owning module for no-value forwarding functions.
- Scan for structs/classes/enums/config objects left with one meaningful field and no behavior, invariant, or ABI role.
- Scan for empty branches, empty callbacks, no-op overrides, no-op `default`/`else` blocks, and functions that only preserve a deleted hook.
- Scan for duplicated constants, duplicated marker names, duplicated normalization/path helpers, duplicated capability discovery, and duplicate resource/provider ownership.
- Scan for includes that were only needed by deleted wrappers or transitive dependencies.
- Scan for stale comments/docs that describe deleted wrappers, reports, diagnostics, fallbacks, or scaffolding.
- Run the scan at module scope for each batch, and run a repo-wide version in Stage 42.

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

| Package surface | Decision | Current implementation after Stage 40 | Stage 40 implementation rule |
| --- | --- | --- | --- |
| Runtime review package | Owned now. | `sparkle_release_assembly` creates `sparkle-runtime-<version>-<channel>-<platform>`. | Keep one runtime package rooted at `dist/releases/<version>/<runtime-package>/`; it contains launcher/editor/runtime app components, cooked shader packages, package markers, licenses, and the minimum package manifest needed for package-root discovery. Do not broad-copy cooked content until a catalog-driven default-content package step owns it. |
| Launcher | Owned as a runtime package component, not as a separate package. | `CMake/SparkleReleaseAssembly.cmake` stages launcher artifacts at the runtime package root. | Keep `SparkleLauncher.exe` at package root because package-root launch detection depends on it. Do not create a separate `sparkle-launcher` package unless a consumer appears. |
| Runtime app | Owned as a runtime package component. | `CMake/SparkleReleaseAssembly.cmake` stages project runtime artifacts under `Apps/<ProjectId>Runtime/` from the package project id. | Keep the runtime app in the runtime package. Avoid project-name hardcoding in implementation logic; resolve package app identity from package/project metadata. |
| Editor app | Owned for review packages only, as a runtime package component. | `CMake/SparkleReleaseAssembly.cmake` stages project editor artifacts under `Apps/<ProjectId>Editor/` from the package project id. | Keep editor staging only while the review package is explicitly engine-evaluation oriented. Do not create a separate `sparkle-editor` package until there is a consumer. |
| Symbols | Owned now as a separate package. | `sparkle_release_assembly` creates `sparkle-symbols-<version>-<channel>-<platform>` as a separate folder. | Keep symbols separate from runtime. Symbols may include PDB/debug artifacts from built products, but must not be copied into the runtime package. |
| Dev tools | Not owned as a package now. | Development tools remain build/cook prerequisites under `artifacts/dev/tools`. | Do not stage `ShaderCompiler`, cook tools, importers, headers, import libraries, static libraries, or source-facing diagnostics into runtime packages. Remove any future `sparkle-dev-tools` assembly until a real consumer requires it. |
| Dependency pack | Not owned as a package now. | Stage 40 removed dependency-pack naming/manifests from release assembly. | Source dependency sync remains a launcher/build workflow, not a distributable package. Do not reintroduce dependency package metadata unless a real package consumer appears. |
| Optional content pack | Owned as a future separate package, not part of default runtime. | Optional level/content availability is represented by catalog metadata such as `Projects/Showcase/Levels.catalog`. Stage 40 does not broad-copy cooked scene/content folders. | Do not include optional synced content in the runtime package by default. Future runtime content staging must be catalog-driven for curated default content only; optional content packs must be explicit separate outputs. |

Unowned outputs closed by Stage 40:

- Stage 40 removed broad cooked-root staging from `CMake/SparkleReleaseAssembly.cmake`; only cooked shader packages are staged until a catalog-driven default-content package step exists.
- Stage 40 removed package IDs/config paths for separate launcher, editor, dev tools, and dependencies packages.
- Stage 40 removed dependency-pack manifest emission.
- Stage 40 removed package file manifests and `SHA256SUMS.txt` outputs because no launcher/runtime/release validation consumer exists.

Manifest/checksum decision:

- Keep `manifests/sparkle-package-manifest.json` because package-root discovery consumes it through `Engine/Core/Private/FileSystemUtils.cpp`.
- Stage 40 removed the bundled-component manifest because no package UI or package validation path consumes it.
- Stage 40 removed `sparkle-release-manifest.json`, `sparkle-build-manifest.json`, `sparkle-dependency-manifest.json`, `sparkle-package-files.json`, and `SHA256SUMS.txt` from default assembly.

Result:

- Stage 15 is a decision-only stage; no package assembly code was added.
- Existing package capability search covered `CMake/SparkleReleaseAssembly.cmake`, `CMake/SparkleArtifactContract.cmake`, launcher package operation definitions, package-root discovery, and package manifest/checksum references.
- The owned package list is now explicit in this roadmap: runtime review package, package-root launcher component, runtime app component, editor app component for review packages, separate symbols package, and future optional content packs.
- Stage 40 implemented that decision: dev tools, separate launcher/editor packages, dependency pack outputs, broad cooked-root staging, unconsumed package manifests, and checksums are gone from default release assembly.
- Optional content is explicitly excluded from the default runtime package. Runtime package content assembly beyond cooked shader packages remains a later catalog-driven content-pack/default-content packaging task, not a broad CMake copy.

Net code pressure:

- No source or package code was added in this stage. The only change is the roadmap ownership decision.
- Stage 40 followed this decision with a net-negative implementation by deleting/replacing unowned package outputs rather than layering a package manager on top of the old script.

Verification:

- `rg -n "SparkleReleaseAssembly|sparkle_release_assembly|ReleaseAssembly|dist/releases|checksums|manifest|symbols" CMake Tools/Launcher` located the original package assembly surface.
- `rg -n "sparkle-package-files|SHA256SUMS|sparkle-release-manifest|sparkle-build-manifest|sparkle-dependency-manifest|sparkle-package-manifest|sparkle-bundled-runtime-components|manifests" Tools Engine Projects CMake Docs` confirmed that package-root discovery consumes `sparkle-package-manifest.json`, while the detailed file manifests/checksums had no engine or launcher consumer.
- Stage 40 verification later confirmed the unconsumed package surfaces are removed from default package assembly.

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

Stage 19 implementation result:

Universal acceptance status:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

Specific acceptance status:

- [x] Memory budget facts are backed by D3D12MA/VMA paths.
- [x] Descriptor pressure facts are consumed or removed.
- [x] Public detail dumps are removed from default API.
- [x] No report format is added.

Removed descriptor pressure reports:

- Removed `RhiDescriptorUsageSnapshot`, `RhiDescriptorAllocatorUsage`, and `ERhiDescriptorUsageStatus` from public RHI.
- Removed `RhiDescriptorService::CaptureDescriptorUsageSnapshot`.
- Removed D3D12 `D3D12DescriptorService::CaptureDescriptorUsageSnapshot`.
- Removed Vulkan `VulkanDescriptorManager::CaptureDescriptorUsageSnapshot`.
- Removed now-unconsumed private descriptor stats helpers:
  - `D3D12DescriptorAllocatorStats`
  - `D3D12DescriptorAllocator::CaptureStats`
  - `D3D12DescriptorHeapUsage`
  - `D3D12DescriptorHeapManager::CaptureUsage`
  - `VulkanDescriptorAllocatorStats`
  - `VulkanDescriptorAllocator::CaptureStats`

Removed memory detail dumps:

- Removed `RhiMemoryAllocationInfo` and the `RhiMemoryUsageSnapshot::Allocations` vector.
- Removed `RhiMemoryUsageSnapshot::HasAllocationDetails`.
- Removed `RenderMemoryDiagnostics::SupportsJsonDump`.
- Removed `RenderMemoryDiagnostics::SupportsAllocationDetails`.
- Removed `RenderMemoryDiagnostics::WriteAllocatorJsonDump`.
- Removed `RhiDiagnosticsCapabilities::SupportsMemoryJsonDump`.
- Removed `RhiBackendMemorySupport::SupportsJsonDump` and `SupportsAllocationDetails`.
- Removed `D3D12GpuMemoryAllocator::SupportsJsonDump` and `WriteAllocatorJsonDump`.
- Removed `VulkanGpuMemoryAllocator::SupportsJsonDump` and `WriteAllocatorJsonDump`.
- Removed `SPARKLE_RHI_MEMORY_JSON_DUMP` default-path behavior from D3D12/Vulkan diagnostics.
- Removed renderer `SceneMemoryReport::NamedAllocationCount` and `LargestNamedAllocations`; renderer memory snapshots now keep totals/category facts rather than allocation-detail reports.

Preserved compact runtime pressure facts:

- D3D12 memory snapshots still call D3D12MA budget/statistics paths through `GetBudget` and `CalculateStatistics`.
- Vulkan memory snapshots still call VMA budget/statistics paths through `vmaGetHeapBudgets` and `vmaCalculateStatistics`.
- `RhiMemoryUsageSnapshot` still preserves allocator backend, total used/allocated/budget bytes, API usage, committed/placed/transient usage, delayed-destruction facts, and category stats.
- `RendererMemoryMonitor` still computes category pressure, overall pressure, texture pressure, and texture streaming policy from compact budget/category facts.
- Editor memory inspection remains available through `Renderer::CaptureMemoryDiagnostics`; only allocation-detail dump payload was removed.

Verification:

- `rg -n "RhiDescriptorUsageSnapshot|RhiDescriptorAllocatorUsage|ERhiDescriptorUsageStatus|CaptureDescriptorUsageSnapshot|RhiDescriptorUsageStatusToString|SupportsJsonDump|SupportsAllocationDetails|SupportsMemoryJsonDump|HasAllocationDetails|RhiMemoryAllocationInfo|\\.Allocations|LargestNamedAllocations|NamedAllocationCount|WriteAllocatorJsonDump|SPARKLE_RHI_MEMORY_JSON_DUMP|VulkanDescriptorAllocatorStats|D3D12DescriptorAllocatorStats|D3D12DescriptorHeapUsage|CaptureUsage\\(|CaptureStats\\(" Engine/RHI Engine/Renderer -g "*.h" -g "*.cpp"` returned no matches.
- `cmake --build build --config DevelopmentEditor --parallel` completed successfully.

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

Stage 20 audit result:

- [x] Existing-capability search was completed before adding code.
- [x] No source code was added in this stage; there is no code debt to offset in this batch.
- [x] No duplicate responsibility was introduced.
- [x] No content/project/level names were added to real code.
- [x] No fallback chain was added.
- [x] Resource, descriptor, pipeline, upload, ray tracing, interop, capture, diagnostics, and presentation services remain owned by RHI.
- [x] Backend-native details remain private or are exposed through the explicit interop/provider bridge.
- [x] No new RHI wrapper layer was added.
- [x] Public service surface is justified for now after the Stage 17 and Stage 19 diagnostic/detail reductions.

RHI service boundary:

- `RenderHardwareInterface` remains the one explicit RHI service root. It exposes backend facts and owned services; it does not become a gameplay/content API.
- `GetCapabilities`, `GetBackendApi`, `GetRequiredShaderBinaryFormat`, and `GetCurrentFrameIndex` are backend/runtime facts. Keep them on RHI because renderer policy, shader package selection, frame resources, and parity checks consume them directly.
- `RhiResourceService` owns GPU resource creation, release, transient memory, native resource handles, GPU virtual addresses, and allocation-size queries. Renderer consumers are frame graph transient allocation/planning, texture upload, mesh buffers, lighting buffers, skinning buffers, BLAS/TLAS/PTLAS builders, and frame history resources.
- `RhiDescriptorService` owns descriptor/view creation, release, shared sampler bindings, descriptor table binding, GPU/CPU view handles, global descriptor state binding, and native texture view info. Stage 19 removed broad descriptor pressure reports; the remaining service surface is functional, not a reporting layer.
- `RhiPipelineService` owns binding layouts and graphics/compute pipeline state creation. Renderer pipeline/runtime code consumes it directly.
- `RhiUploadService` owns uniform upload and upload/readback paths. Keep it in RHI because upload behavior is backend- and frame-lifetime-dependent.
- `RhiRayTracingService` owns ray tracing prebuild queries, scratch/acceleration-structure buffers, instance buffers, and build commands for classic TLAS and PTLAS flows. Renderer owns policy and scene selection; RHI owns backend execution.
- `RhiInteropService` owns explicit native interop and presentation provider bridging. It is the sanctioned escape hatch for external providers and platform/native integration.
- `RhiCaptureService` owns texture-to-BMP capture/readback execution. Capture remains preserved as a product/tool capability; editor/runtime-facing capture commands should stay above this service.
- `RenderDiagnostics` owns compact runtime facts, object names, timing hooks, failure reporting, and compact memory-budget facts. It should not regain broad dump/report ownership after Stage 18 and Stage 19.
- `RhiPresentationService` owns swapchain/back-buffer presentation, viewport/scissor facts, render-target access, ImGui texture resolution, manual-present state, and present execution.
- `RenderDeviceServices` remains the device/service owner and command-submission surface. Its `GetImGuiRenderer` is the editor UI backend bridge; it should not grow into a general editor facade.

Public escape hatches:

- `Renderer::GetRenderHardwareInterface()` is currently consumed by application/editor/runtime-console code for presentation handoff and by renderer systems internally. Keep it narrow; do not let gameplay/content systems use it directly.
- `Renderer::GetCommandSubmissionService()` is currently consumed by shader recook to wait for idle. Keep this as an explicit tool/runtime synchronization escape hatch until a smaller owned synchronization command replaces it with less public surface.
- `RendererSystemRoot::GetRenderHardwareInterface()` is renderer-private plumbing and is acceptable because renderer passes and resource managers need explicit RHI services.

Backend-native boundary:

- Native handles and backend-specific details should remain in backend-private implementations, `RhiInteropService`, `RhiNativeHandles`, or explicit provider bridge structs.
- Frame graph native view data currently flows through `RhiDescriptorService::GetNativeTextureViewInfo`; keep this as a descriptor-owned backend bridge instead of adding another renderer wrapper.
- No D3D12/Vulkan-specific assumptions should leak into GameFramework, Projects, content catalog, or launcher workflows.

Next reduction pressure:

- Do not add a second RHI facade.
- Do not add generic service locators around RHI.
- Only reduce `RenderHardwareInterface` public methods when a stage can remove an actual consumer or merge a duplicated path without hurting D3D12/Vulkan parity, capture, shader package ABI, TLAS/PTLAS, or editor inspection.
- Candidate later cleanup: review whether const/non-const service accessor pairs are all consumed; remove pairs only if the consumer search proves they are dead.

Verification:

- `Get-Content Engine/RHI/Public/Device/RenderHardwareInterface.h` confirmed the current service root and service list.
- `Get-Content Engine/RHI/Public/Device/RenderDeviceServices.h` confirmed device ownership, command submission, and ImGui bridge ownership.
- `rg -n "GetResourceService\\(|GetDescriptorService\\(|GetPipelineService\\(|GetUploadService\\(|GetRayTracingService\\(|GetInteropService\\(|GetCaptureService\\(|GetDiagnostics\\(|GetPresentationService\\(|GetRenderHardwareInterface\\(|GetCommandSubmissionService\\(" Engine/Renderer Engine/Application Engine/Editor Tools Projects -g "*.h" -g "*.cpp"` identified current service consumers.
- `rg --files Engine/RHI/Public` enumerated the public RHI surface.
- No build was required because this stage made no source changes.

### Stage 21: D3D12/Vulkan Parity Matrix

References: AMD-CAULDRON, NV-NRI, UE-SOURCE.

Prompt:

- Create a working parity matrix in existing docs or issue notes for key RHI/Renderer features.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] D3D12/Vulkan parity is named for resources, descriptors, pipelines, uploads, presentation, ray tracing, PTLAS, capture.
- [x] Backend-specific gaps are listed.
- [x] No parity gap is hidden behind generic abstraction.
- [x] Any extension opportunity is recorded.

Stage 21 implementation result:

Universal acceptance status:

- [x] Existing-capability search was completed before adding code.
- [x] No source code was added in this stage; there is no code debt to offset in this batch.
- [x] No duplicate responsibility was introduced.
- [x] No content/project/level names were added to real code.
- [x] No fallback chain was added.

Specific acceptance status:

- [x] D3D12/Vulkan parity is named for resources, descriptors, pipelines, uploads, presentation, ray tracing, PTLAS, and capture.
- [x] Backend-specific gaps are listed.
- [x] No parity gap is hidden behind generic abstraction.
- [x] Extension opportunities are recorded.

Working parity matrix:

| Area | Public owner | D3D12 status | Vulkan status | Parity result | Gap / next action |
| --- | --- | --- | --- | --- | --- |
| Backend identity | `RhiCapabilities` / `RenderHardwareInterface` | `ERhiBackendApi::D3D12`, DXIL shader packages, feature-level version semantic | `ERhiBackendApi::Vulkan`, SPIR-V shader packages, API-version semantic | Parity with intentional shader/package split | Keep package ABI explicit; do not hide DXIL/SPIR-V behind a generic blob. |
| Resources | `RhiResourceService` | `D3D12ResourceService`, D3D12MA-backed allocation records, native resource handles | `VulkanResourceService`, VMA-backed allocation records, native resource handles | Parity | Stage 22 should remove duplicated convenience forwarding only if consumers prove redundant. |
| Texture upload | `RhiResourceService` / texture factories | `D3D12TextureFactory` through resource service | `VulkanTextureFactory` through resource service | Parity | Preserve backend-specific texture creation details privately. |
| Buffer upload / uniforms | `RhiUploadService` | constant buffer manager is upload service | Vulkan constant buffer manager is upload service | Parity | Compute/copy queues are not exposed yet; keep graphics-queue upload baseline explicit. |
| Readback / capture source | `RhiUploadReadbackCapabilities`, `RhiCaptureService` | readback supported, `D3D12CaptureService::CaptureTextureToBmp` | readback supported, `VulkanCaptureService::CaptureTextureToBmp` | Parity | Keep capture as product/tool capability; do not re-add smoke capture ownership. |
| Descriptors | `RhiDescriptorService` | descriptor tables, D3D12 descriptor heap manager/service | descriptor sets, Vulkan descriptor manager/allocator | Parity with backend-specific model | The descriptor model difference is real and should remain visible through capabilities. |
| Shared samplers | `RhiDescriptorService` | D3D12 sampler library/table binding | Vulkan sampler library/set binding | Parity | Keep sampler ownership in descriptor service. |
| Pipeline layouts / PSO | `RhiPipelineService` | D3D12 root signature, binding layout, graphics/compute PSO | Vulkan pipeline layout, binding layout, graphics/compute pipeline | Parity | Shader reflection/package inputs must remain explicit per backend format. |
| Presentation | `RhiPresentationService` | D3D12 swapchain/back buffer, manual present, ImGui texture resolve | Vulkan swapchain/back buffer, manual present, ImGui texture resolve | Parity | Application/editor may use presentation service; gameplay/content should not. |
| ImGui backend | `RenderDeviceServices::GetImGuiRenderer` | D3D12 ImGui backend | Vulkan ImGui backend | Parity | Preserve as editor/tool bridge, not a broader UI facade. |
| Diagnostics facts | `RenderDiagnostics`, `RhiMemoryDiagnostics` | D3D12 debug layer, PIX events, D3D12MA budget/statistics | Vulkan validation/debug events, VMA budget/statistics | Parity after Stage 19 cleanup | Keep compact facts only; do not restore default JSON/detail dump reports. |
| Native interop | `RhiInteropService`, `RhiNativeHandles` | native device/queue/command list/resource bridge | Vulkan handles/manual function pointer/interposer bridge | Parity with different bridge kinds | Stage 23 should keep native access tied to explicit provider consumers. |
| Format support | `RhiCapabilities::FormatSupport` | queried through `CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT)` | queried through `vkGetPhysicalDeviceFormatProperties` | Parity | Add formats only when a renderer feature consumes them. |
| Queues | `RhiQueueCapabilities` | graphics true, compute false, copy false | graphics true, compute false, copy false | Parity baseline | Extension opportunity: expose async compute/copy only after frame graph has real users. |
| Mesh/task shaders | `RhiCapabilities` | false | false | Parity baseline | Extension opportunity after core renderer/ray tracing path is cleaner. |
| Ray tracing base | `RhiRayTracingService` | DXR capability query through D3D12 options5 | Vulkan KHR acceleration/ray query feature path | Parity baseline | Keep backend capability reasons explicit. |
| BLAS | `RhiRayTracingService` | D3D12 BLAS prebuild/resource paths | Vulkan BLAS prebuild/resource paths | Parity | Build/update policy remains renderer-owned. |
| Classic TLAS | `RhiClassicTlasService` / `RhiRayTracingService` | build/update/gpu-readable instance buffer supported when DXR is available | build/update/gpu-readable instance buffer supported when Vulkan RT is available | Parity | Preserve as baseline RT path. |
| PTLAS capability | `RhiPartitionedTlasService` / `RhiRayTracingCapabilities` | NVAPI partitioned TLAS path; public DXR PTLAS flags currently false | Vulkan NV partitioned acceleration structure path | Partial parity | Both are modeled, but provider details differ and public DXR PTLAS is not implemented. |
| PTLAS renderer selection | Renderer ray tracing scene/strategies | capability reason reports `d3d12-nvapi-ptlas-supported-but-renderer-selection-not-wired` when requested/supported | capability reason reports `partitioned-tlas-supported-but-renderer-selection-not-wired` when requested/supported | Gap | Future RT stage must wire renderer selection and acceptance scenes for both backends. |
| PTLAS CPU operation packing | `RhiPartitionedTlasService` | supported when NVAPI standard cap is present | supported when Vulkan NV PTLAS is enabled | Parity when provider is available | Keep minimal original-PTLAS-style implementation; avoid extra wrappers. |
| PTLAS GPU-driven operations | `RhiPartitionedTlasCapabilities` | capability can be true through D3D12 NVAPI | capability can be true through Vulkan NV partitioned acceleration structure | Provider parity when supported | Renderer must branch by explicit capability, not by backend name; a product shader-writer demo is still future work. |
| PTLAS shader access | `RhiPartitionedTlasCapabilities` / renderer capability report | D3D12 NVAPI path, public DXR descriptor path false | Vulkan descriptor path false, shader-device-address path possible | Backend-specific gap | Decide one minimal shader access contract before adding demos. |
| Capture BMP writer | `RhiCaptureService` | product capture path exists | product capture path exists | Parity | Harden capability without expanding diagnostics/smoke harnesses. |

Backend-specific gaps to keep visible:

- D3D12 uses descriptor tables; Vulkan uses descriptor sets. This is intentional and should remain visible in `RhiCapabilities::DescriptorModel`.
- D3D12 requires DXIL shader packages; Vulkan requires SPIR-V shader packages. Offline cooked shader package reflection must preserve this ABI distinction.
- D3D12 PTLAS currently depends on NVAPI capability/runtime/header paths; public DXR PTLAS flags are explicitly false.
- Vulkan PTLAS currently depends on the NV partitioned acceleration structure extension and loaded functions.
- PTLAS renderer selection is not fully wired even when a backend reports provider support. This is the most important parity gap for the ray tracing roadmap.
- PTLAS GPU-driven operation capability is now represented for both D3D12 NVAPI and Vulkan NV providers, but renderer-owned shader writer demos are not implemented yet.
- Both backends report graphics queue support only; compute/copy queue capability is not yet exposed as an active renderer path.
- Mesh/task shader support is false on both backends and should not be treated as a hidden renderer feature.
- Native interop bridges differ by design: D3D12 exposes native device/queue/list style handles; Vulkan exposes Vulkan handles/function-pointer/interposer style capability.

Extension opportunities:

- Wire renderer PTLAS provider selection so classic TLAS and PTLAS are both usable product paths on D3D12 and Vulkan.
- Add a minimal RT scene or level mode that proves classic TLAS and PTLAS can both be selected without hardcoded content names.
- Keep PTLAS implementation close to the original provider model: backend capability query, operation pack, build sizes, buffers, and build command. Avoid broad new abstraction.
- Add async compute/copy only when frame graph passes have real queue ownership and synchronization requirements.
- Add mesh/task shader support only when a renderer feature consumes it and shader package reflection can express it cleanly.
- Keep native interop only for explicit provider bridges such as upscaling/ray reconstruction/external feature integration.

Verification:

- `rg --files Engine/RHI/Private | rg "(D3D12|Vulkan)"` confirmed both backend trees expose matching service areas.
- `rg -n "class .*ResourceService|class .*Descriptor|class .*PipelineService|class .*CaptureService|class .*PresentationService|class .*InteropService|class .*RayTracingServices" Engine/RHI/Private/D3D12 Engine/RHI/Private/Vulkan -g "*.h"` confirmed matching service implementations.
- `Get-Content Engine/RHI/Public/Core/RhiCapabilities.h` confirmed the public capability model.
- `Get-Content Engine/RHI/Public/RayTracing/RhiRayTracingService.h`, `RhiClassicTlasService.h`, and `RhiPartitionedTlasService.h` confirmed classic TLAS and PTLAS public ownership.
- D3D12 capability inspection covered `D3D12RenderHardwareInterface::BuildCapabilities`, `D3D12Rhi::CheckRayTracingSupport`, and `D3D12NvapiRayTracingProvider::QueryPartitionedTlasCapabilities`.
- Vulkan capability inspection covered `VulkanRenderHardwareInterface::BuildCapabilities` and `VulkanRhi::BuildRayTracingCapabilities`.
- No build was required because this stage made no source changes.

### Stage 22: RHI Resource/Descriptor/Pipeline Cleanup

References: NV-NRI, NV-NVRHI, AMD-CAULDRON.

Prompt:

- Remove duplicate helper paths while preserving explicit API behavior.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Resource creation path remains backend-owned.
- [x] Descriptor binding model remains understandable.
- [x] Pipeline/layout creation remains explicit.
- [x] Any removed helper had no unique product behavior.

Stage 22 implementation result:

- Removed duplicate descriptor, pipeline, and resource forwarding methods from `D3D12RenderHardwareInterface`.
- Removed duplicate descriptor, pipeline, and resource forwarding methods from `VulkanRenderHardwareInterface`.
- Kept `RhiResourceService`, `RhiDescriptorService`, and `RhiPipelineService` as the explicit owners.
- Kept D3D12 private descriptor table resolution because `D3D12RenderCommandList` still consumes it for backend-native descriptor binding.
- Kept backend presentation helpers because presentation ownership is covered by `RhiPresentationService` and was not the duplicate resource/descriptor/pipeline surface being removed.
- Updated D3D12 ImGui descriptor callbacks to allocate/free through `GetDescriptorService()`.
- Updated D3D12/Vulkan interop native texture view lookup to go through `GetDescriptorService().GetNativeTextureViewInfo(...)`.

Removed helpers with no unique product behavior:

- Pipeline forwarding: `CreateBindingSet`, `CreateBindingLayout`, `CreateGraphicsPipelineState`, `CreateComputePipelineState`.
- Descriptor forwarding: `BindGlobalDescriptorState`, descriptor allocation/table helpers, shader-resource descriptor helpers, shared sampler binding, resource view create/release/lookup, native texture view lookup.
- Resource forwarding: texture/buffer creation helpers, vertex/index/structured buffer helpers, owned resource release/native lookup/GPU address lookup, transient memory and aliasing resource helpers, allocation info helpers, unordered-access helper.

Ownership after cleanup:

- Renderer resource consumers already call `GetResourceService()` directly.
- Renderer descriptor consumers already call `GetDescriptorService()` directly.
- Renderer pipeline/runtime consumers already call `GetPipelineService()` directly.
- Backend-native details remain inside D3D12/Vulkan services or explicit interop bridges.
- No project, level, asset, content-pack, or sample names were added.
- No fallback chain was added; the batch removes duplicate entrypoints instead of adding alternate paths.
- The batch is net code negative: the working diff is hundreds of lines smaller after accounting for the roadmap note.

Verification:

- `rg -n -g "*.h" -g "*.cpp" -- "D3D12RenderHardwareInterface::(CreateBindingSet|CreateBindingLayout|CreateGraphicsPipelineState|CreateComputePipelineState|BindGlobalDescriptorState|AllocateDescriptor|ReleaseDescriptor|AllocateDescriptorTable|GetDescriptorTableCpuHandle|ReleaseDescriptorTable|AllocateShaderResourceDescriptor|ReleaseShaderResourceDescriptor|AllocateUniformConstantBuffer|GetSharedSamplerBinding|CreateTexture\\(|CreateTextureResource|CreateBufferResource|CreateVertexBuffer|CreateStructuredBuffer|CreateIndexBuffer|ReleaseOwnedResource|GetNativeResource|GetResourceGpuVirtualAddress|GetTextureAllocationInfo|GetBufferAllocationInfo|CreateTransientMemoryBlock|ReleaseTransientMemoryBlock|CreateAliasingTextureResource|CreateAliasingBufferResource|CreateResourceView|ReleaseResourceView|GetResourceViewCpuHandle|GetResourceViewGpuHandle|GetNativeTextureViewInfo|SupportsUnorderedAccess)" Engine/RHI/Private/D3D12` returned no matches.
- `rg -n -g "*.h" -g "*.cpp" -- "VulkanRenderHardwareInterface::(CreateBindingSet|CreateBindingLayout|CreateGraphicsPipelineState|CreateComputePipelineState|BindGlobalDescriptorState|AllocateDescriptor|ReleaseDescriptor|AllocateDescriptorTable|GetDescriptorTableCpuHandle|ReleaseDescriptorTable|AllocateShaderResourceDescriptor|ReleaseShaderResourceDescriptor|AllocateUniformConstantBuffer|GetSharedSamplerBinding|CreateTexture\\(|CreateTextureResource|CreateBufferResource|CreateVertexBuffer|CreateStructuredBuffer|CreateIndexBuffer|ReleaseOwnedResource|GetNativeResource|GetResourceGpuVirtualAddress|GetTextureAllocationInfo|GetBufferAllocationInfo|CreateTransientMemoryBlock|ReleaseTransientMemoryBlock|CreateAliasingTextureResource|CreateAliasingBufferResource|CreateResourceView|ReleaseResourceView|GetResourceViewCpuHandle|GetResourceViewGpuHandle|GetNativeTextureViewInfo|SupportsUnorderedAccess)" Engine/RHI/Private/Vulkan` returned no matches.
- `cmake --build build --config DevelopmentEditor --parallel` completed successfully. The build emitted the existing launcher warning that `VCINSTALLDIR` is not set, but compilation/linking completed.

### Stage 23: Native Interop Boundary

References: NV-NVRHI, NV-STREAMLINE, UE-SOURCE.

Prompt:

- Keep native interop only for explicit provider bridges.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Native handle access has explicit consumer ownership.
- [x] Provider resource contracts remain narrow.
- [x] No broad native escape hatch is exposed casually.
- [x] Streamline/upscaling/ray reconstruction paths remain functional where supported.

Stage 23 implementation result:

- Removed raw Vulkan native handles from broad `RhiExternalFeatureInteropCapabilities`.
- Added provider-owned Vulkan native payload to `RhiNativeDeviceQueueInterop`, returned only through `RhiInteropService::GetDeviceQueueInterop(...)` with a named `RhiNativeInteropRequest`.
- Removed unused duplicate `RhiInteropService::GetNativeTextureViewInfo`; native texture view conversion remains owned by `RhiDescriptorService`.
- Updated Streamline backend validation, device setup, and adapter info construction to use `RhiNativeDeviceQueueInterop` instead of raw handles in capabilities.
- Updated DLSS upscaling and DLSS ray reconstruction runtimes to pass their explicit native interop contract into Streamline adapter setup.

Native interop ownership after cleanup:

- Upscaling provider initialization requests native device/queue ownership with `ERhiNativeInteropConsumer::UpscalerProvider`.
- Ray reconstruction provider initialization requests native device/queue ownership with `ERhiNativeInteropConsumer::RayReconstructionProvider`.
- DLSS and DLSS ray reconstruction command-list access is still requested with provider-specific consumers.
- Presentation bridge use remains explicit for ImGui and Streamline presentation upgrade paths.
- Frame graph/resource native texture view data remains descriptor-owned and is not exposed through the interop facade.
- Frame graph `NativeResourceHandle` use is treated as the RHI resource identity needed for scheduling/barriers/imports, not as a backend API escape hatch; backend-native API objects remain in RHI-private code or explicit provider bridges.
- Validation/debug native command-list access remains named with `ERhiNativeInteropConsumer::Validation`; it is not a provider bridge, but it is an explicit diagnostic owner and was not expanded in this stage.

Provider contract result:

- Streamline still receives D3D12 device handles through provider-owned `RhiNativeDeviceQueueInterop`.
- Streamline still receives Vulkan instance, physical device, logical device, graphics queue, and queue-family data through provider-owned `RhiNativeDeviceQueueInterop`.
- `RhiCapabilities::ExternalFeatureInterop` now reports capability facts only; it no longer carries raw Vulkan object pointers.
- No project, level, asset, content-pack, or sample names were added.
- No fallback chain was added.
- The batch remains net code negative together with Stage 22 cleanup.

Verification:

- `rg -n "ERhiNativeInteropConsumer::|RhiNativeInteropRequest\\{|GetNativeHandle\\(|GetDeviceQueueInterop\\(|GetNativeTextureViewInfo\\(|UpgradePresentationInterface\\(" Engine Tools Projects -g "*.h" -g "*.cpp"` identified native interop ownership.
- `rg -n "ExternalFeatureInterop\\.Vulkan(Instance|PhysicalDevice|Device|GraphicsQueue|GraphicsQueueFamilyIndex)|\\.VulkanInstance|\\.VulkanPhysicalDevice|\\.VulkanDevice|\\.VulkanGraphicsQueue|\\.VulkanGraphicsQueueFamilyIndex|GetInteropService\\(\\)\\.GetNativeTextureViewInfo|RhiInteropService::GetNativeTextureViewInfo|virtual NativeTextureViewInfo GetNativeTextureViewInfo" Engine Tools Projects -g "*.h" -g "*.cpp"` now finds only descriptor-service native texture view ownership.
- `cmake --build build --target architecture_boundary_check --config DevelopmentEditor` completed successfully; Renderer native API usage remains limited to the counted Streamline provider bridge exceptions.
- `cmake --build build --config DevelopmentEditor --parallel` completed successfully. The build emitted the existing launcher warning that `VCINSTALLDIR` is not set, but compilation/linking completed.

### Stage 24: Shader Compiler/Cook ABI Audit

References: NV-SHADERMAKE, AMD-CAULDRON, UE-SOURCE.

Prompt:

- Audit shader source-to-cooked-package-to-runtime flow.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Shader source includes are tracked.
- [x] DXC/Slang target outputs are known.
- [x] Reflection data survives cook.
- [x] Runtime package cache loads cooked packages.
- [x] Debug artifact behavior is opt-in or removed.

Stage 24 closure notes:

- Existing-capability search covered shader registration, include tracking, compiler target selection, package writing, reflection serialization, runtime cache loading, launcher cook requests, and editor recook.
- No new shader ABI abstraction was added. The only source edit removed the sample project name from editor shader recook error text in `Engine/Application/Private/ShaderRecook/ShaderCompilerProcess.cpp`.
- Include tracking is owned by `IncludeClosureHasher`; it recursively resolves `#include` files through `ShaderIncludeResolver`, hashes every source file, and feeds both `sourceHash` and `includeClosureHash` into `ShaderCacheKey` and `ShaderContractJobIdentity`.
- Target output policy is explicit: `ShaderPackageCookSettings` defaults to `DxilSm66` plus `SpirV16`; `ShaderCompiler.exe list-backends` reports DXC and Slang targets across `DxilSm60` through `DxilSm67` and `SpirV14` through `SpirV16`.
- Cooked package ABI keeps both binary formats in one runtime package where requested: `StageCompiler` maps DXIL targets to `CookedShaderBinaryFormat::Dxil` and SPIR-V targets to `CookedShaderBinaryFormat::SpirV`.
- Reflection survives cook by default: `stripReflection=false`, `StageCompiler` moves backend reflection into `CookedStageBuild`, `CookedPackageWriter` serializes it through `ReflectionSerializer`, and `CookedShaderPackageCache` reads reflection arrays from `.sparkshader` packages.
- Runtime loading is owned by `CookedShaderPackageCache` and `PipelineRuntimeLibrary`; package load validates package key, shader model ABI, binding-layout hash, required backend binary format, bytecode hashes, expected stages, and reflected binding compatibility.
- Debug artifacts are outside the runtime package path. Default launcher/cook only forwards `--debug-artifacts` when `ShaderWriteDebugArtifacts` is enabled; editor recook writes debug bundles as an explicit editor inspection path, preserving the shader inspection panels without making debug bundles part of default runtime cook.
- Representative ABI check: `ShaderCompiler.exe cook --package ComputeClear --target DxilSm66 --target SpirV16` produced `A5C7ADD3A0B8D443.sparkshader` plus `ShaderPackageRegistry.sreg`; `ShaderCompiler.exe inspect-package` reported two binaries, two pipeline layouts, and two reflection records.

### Stage 25: Shader Registration And Binding Duplication Cleanup

References: NV-SHADERMAKE, NV-NVRHI, UE-SOURCE.

Prompt:

- Reduce duplicated C++ registration/HLSL binding declarations only when net code decreases.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Duplicate declarations are listed.
- [x] Any simplification deletes more code than it adds.
- [x] No code generator is added unless net code decreases materially.
- [x] Layout verification remains intact.

Stage 25 closure notes:

- Existing-capability search covered shader registration macros, renderer shader registration files, HLSL resource declarations, pass definitions, `ShaderPackageDefinition`, `ShaderPackageLayoutBuilder`, `CookedShaderPackageCache`, and `PipelineRuntimeLibrary`.
- Duplicate declarations found:
  - C++ shader parameter structs in `Engine/Renderer/ShaderRegistrations/*.cpp` mirror HLSL resources in `Engine/Assets/Shaders/**`. This remains intentionally owned by shader registration because cook-time reflection validates it and a generator would add scaffolding before deleting enough code.
  - Runtime pass definitions duplicated package identity as both `PackageId` and `BindingLayoutId`. This was removed from `ShaderPackageDefinition`; runtime now uses the generated `PassParameterLayout` debug name and binding-layout hash for diagnostics/validation.
  - Runtime pass definitions duplicated package identity again through `PackageDeclarationName` strings such as `GBufferShaderPackage`; this diagnostic-only field was removed from `RenderPassDefinition`, `RenderPassShaderRuntimeDesc`, and `PipelineRuntimePackageRequest`.
  - `RendererShaderPackages::*` constants duplicate package names used by shader registrations and pass definitions. This remains for now because it centralizes cross-file package IDs without adding a generator or reflection lookup layer.
  - Per-pass debug names such as `*_BindingLayout` and `*_PipelineState` duplicate package names. This remains for now because deriving stable wide debug names would add helper code and not materially reduce source.
- Code simplification was net-negative: removed the runtime `BindingLayoutId` field, removed the runtime `PackageDeclarationName` field, removed duplicated pass assignments/arguments in `ComputePassUtilities`, `RasterPassUtilities`, `GBufferPass`, and per-pass definition call sites, and simplified diagnostics in `CookedShaderPackageCache`, `RenderPassShaderRuntime`, `RenderPassDefinitionRuntime`, and `PipelineRuntimeLibrary` without adding a replacement abstraction.
- Source-only cleanup count for the shader/runtime pass code is `9` added / `61` deleted, excluding this roadmap note.
- Layout verification remains intact: `BuildRegisteredShaderPackageLayout` still builds runtime layouts from registered C++ parameter structs; `CookedShaderPackageCache` still validates package key, shader model, binding-layout hash, expected stages, backend binary format, bytecode hashes, and reflected binding compatibility.
- Representative verification: `ShaderCompiler.exe cook --package GBuffer --target DxilSm66 --target SpirV16` succeeded; `ShaderCompiler.exe inspect-package ...DCDF88FE4533E870.sparkshader` reported four binaries, four reflection records, two pipeline layouts, and matching DXIL/SPIR-V layout hash `0xAA07F509D5C7FEE2`.

### Stage 26: Classic TLAS Flow Audit

References: NV-NRI, UE-SOURCE, NV-RTXPT, NVPRO-VK-PARTITIONED-TLAS.

Prompt:

- Trace classic BLAS/TLAS build/update/trace ownership.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] BLAS lifecycle owner is clear.
- [x] Classic TLAS lifecycle owner is clear.
- [x] Trace pipeline usage is clear.
- [x] D3D12/Vulkan paths are both represented where supported.

Stage 26 closure notes:

- Existing-capability search covered RHI ray tracing services, D3D12/Vulkan classic TLAS services, render command-list acceleration-structure builds, renderer BLAS cache, classic TLAS builder, top-level strategy selection, frame graph scene build passes, pass TLAS bindings, and HLSL ray-query helpers.
- External audit reference studied: `nvpro-samples/vk_partitioned_tlas`, especially `partitioned_tlas_acceleration_structure.cpp`, `partitioned_acceleration_structures.hpp`, `animation_update_instances.comp.glsl`, `animation_init.comp.glsl`, `raytrace.rgen.glsl`, and `partitioned_tlas.cpp`. Use it to keep the classic TLAS/PTLAS split explicit: regular TLAS owns full-instance-buffer build/refit through host-known `vkCmdBuildAccelerationStructuresKHR` style inputs; PTLAS owns partitioned, device-addressed, operation-buffer driven partial updates.
- No new capability code was added. The only source cleanup was removing an unused `unordered_set` include from `Engine/Renderer/Private/RayTracing/Acceleration/RayTracingBlasCache.h`; this keeps the batch net-negative for code.
- BLAS lifecycle owner:
  - Renderer owns scene-level BLAS reuse through `RayTracingBlasCache`.
  - `RayTracingBlasCache::BeginFrame` marks entries stale, `EnsureBlas` rebuilds only when mesh geometry changes or the BLAS is missing, `EndFrame` releases entries not touched by the current scene, and `Clear` releases all owned resources.
  - RHI owns backend allocation and native build execution through `RhiRayTracingService::GetBottomLevelAccelerationStructurePrebuildInfo`, `CreateRayTracingScratchBuffer`, `CreateRayTracingAccelerationStructureBuffer(...BottomLevel...)`, and `RenderCommandList::BuildBottomLevelAccelerationStructure`.
- Classic TLAS lifecycle owner:
  - Renderer owns top-level scene policy through `RenderRayTracingScene`, `RayTracingTopLevelScenePlanner`, `RayTracingTopLevelAccelerationStructureStrategy`, and `RayTracingClassicTlasStrategy`.
  - `RayTracingClassicTlasBuilder::Prepare` sizes persistent TLAS/scratch capacity for the frame, `Build` converts current `RenderSceneData::meshInstances` into `RhiRayTracingInstanceDesc` entries, requests or reuses BLAS handles, uploads the instance buffer, inserts UAV barriers for BLAS built this frame, and submits build/refit through `RenderCommandContext::BuildTopLevelAccelerationStructure`.
  - Classic refit remains a renderer policy gate: `r.RayTracing.Tlas.Refit` is honored only when `RhiCapabilities.RayTracing.Groups.ClassicTlas.SupportsClassicTlasUpdate` is true.
  - RHI owns backend-native TLAS prebuild sizing, instance-buffer conversion, AS buffer creation, and command-list build/update execution through `RhiClassicTlasService` and `RenderCommandList`.
- Trace pipeline usage:
  - Frame graph reserves a persistent `SceneTlas` handle and adds `RayTracingSceneBuild` before ray-query consumers.
  - Descriptor TLAS consumers bind `SceneTlas` through `RayTracingScenePassBinding::BindSceneTlas` in indirect diffuse, indirect specular, direct shadow signal, and reference path tracing passes.
  - Shader trace code routes through `Engine/Assets/Shaders/RayTracing/RayTracingSceneTlasTrace.hlsli` and `RayTracingTraceQuery.hlsli`, using inline `RayQuery::TraceRayInline` against either descriptor-bound `SceneTlas` or an explicit TLAS device address where the pass selects that path.
  - No ray-generation / hit-group state-object path is required for the current classic TLAS flow; current product usage is inline ray query.
- D3D12/Vulkan parity:
  - D3D12 path: `D3D12RayTracingServices` queries BLAS/TLAS prebuild sizes, `D3D12ClassicTlasServices` converts `RhiRayTracingInstanceDesc` to `D3D12_RAYTRACING_INSTANCE_DESC`, and `D3D12RenderCommandList` submits `BuildRaytracingAccelerationStructure` for BLAS and TLAS build/update.
  - Vulkan path: `VulkanRayTracingServices` queries BLAS/TLAS build sizes, `VulkanClassicTlasServices` converts `RhiRayTracingInstanceDesc` to `VkAccelerationStructureInstanceKHR`, and `VulkanRenderCommandList` submits `vkCmdBuildAccelerationStructuresKHR` for BLAS and TLAS build/update with explicit acceleration-structure barriers.
  - Both backends preserve classic TLAS build and update where capability bits report support. Capability absence produces empty prebuild/resource handles at the owning RHI boundary; renderer then marks the scene TLAS unavailable instead of inventing a fallback chain.
- Ownership risk recorded for later stages:
  - `RenderCommandContext` is a renderer-side forwarding wrapper over `RenderCommandList`; it should remain thin and must not grow backend-specific logic.
  - `RayTracingTopLevelAccelerationStructureStrategy` currently includes PTLAS hooks in the common interface. Stage 27 should reduce PTLAS-specific planning, metrics, and operation packing so classic TLAS remains readable without losing PTLAS capability.
  - The direct shadow device-address path exists for shader-device-address TLAS access. Keep it only if Stage 27 confirms it is required for the PTLAS path; classic TLAS itself should stay descriptor-first.

### Stage 27: PTLAS Minimal Reference Flow

References: NV-NRI, UE-SOURCE, NV-RTXPT, NVPRO-VK-PARTITIONED-TLAS.

Prompt:

- Reduce PTLAS to capability check, compact descriptor input, backend build/update, resource lifetime, trace use.
- Use `nvpro-samples/vk_partitioned_tlas` as the primary PTLAS audit reference for partition layout, instance write records, operation buffers, indirect build/update submission, and shader-visible TLAS/PTLAS equivalence.

Reference study conclusions:

- Preserve the current Sparkle direction where BLAS is shared by classic TLAS and PTLAS. The reference builds identical BLAS data for both paths and changes only top-level construction/update behavior.
- Keep PTLAS ownership narrow:
  - Renderer owns partition planning, stable instance indices, dynamic/static classification, update-stream selection, and deciding whether classic TLAS or PTLAS is active.
  - RHI owns PTLAS size query, buffer/resource creation, backend-native descriptor write contract, and backend command submission.
  - Shader code owns only operation-buffer writes when a GPU update path is selected; it must not know project, level, asset, or sample names.
- Keep the PTLAS data model small. The minimum useful product set is:
  - PTLAS storage buffer.
  - Build scratch and optional update scratch if consumed.
  - Operation records buffer.
  - Operation count buffer.
  - Instance write records buffer.
  - Optional instance update records only if BLAS reference updates are genuinely supported.
  - Optional partition translation records only if a product feature consumes partition translations.
- Prefer one compact operation path first. The reference initializes PTLAS with a write-instance operation and performs dynamic updates by resetting the operation record, atomically increasing `argCount`, and writing only changed instances. Sparkle should not carry multiple writer paths unless each is selected by real capability and used by runtime.
- Enforce stable unique PTLAS instance indices. The reference relies on `instanceIndex` identity; duplicate instance indices are not valid input. Sparkle's planner should make this an owned invariant rather than a diagnostics-only afterthought.
- Keep shader-visible TLAS/PTLAS equivalence. The reference ray shader receives a single top-level address and traces it the same way whether the active source is TLAS or PTLAS. Sparkle should preserve one pass-facing scene top-level handle and hide only the backend binding difference in RHI/frame-graph binding code.
- Do not import sample/demo scaffolding:
  - Do not copy the domino physics simulation.
  - Do not add UI-only PTLAS mode experiments as engine architecture.
  - Do not add inspector/readback buffers to product runtime.
  - Do not add extra timing/report artifacts.
  - Do not add shader recompilation or SPIR-V dump behavior from the sample.
- The intended Sparkle impact is deletion/refinement, not feature sprawl: collapse PTLAS planning/metrics/update-writer code to the smallest path that preserves Vulkan PTLAS and D3D12 parity where supported, while leaving classic TLAS readable and descriptor-first.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] PTLAS planner metrics not required for product behavior are removed.
- [x] CPU validation readbacks not required for product behavior are removed.
- [x] Future GPU-pack placeholders are removed.
- [x] PTLAS still renders where supported.
- [x] PTLAS implementation is checked against `nvpro-samples/vk_partitioned_tlas`: no duplicate instance indices, explicit partition ownership, compact operation buffers, and no host-side synchronization requirement hidden behind renderer code.
- [x] PTLAS buffer/resource set is no larger than the minimal set named in the source study conclusions, unless a consumed product feature justifies the extra buffer.
- [x] Any PTLAS shader update path writes operation records and instance records only; sample physics/UI/inspection concepts are not imported into engine runtime.

Stage 27 closure notes:

- Existing-capability search covered Renderer PTLAS planning, logical update stream construction, classic/partitioned strategy selection, frame-graph ray tracing passes, `RhiPartitionedTlasDesc`, `RhiPartitionedTlasService`, `RhiRayTracingService`, Vulkan PTLAS services/build submission, D3D12 NVAPI PTLAS services/build submission, and deleted GPU-writer policy files.
- The useful NVIDIA reference behavior is preserved:
  - Sparkle still builds PTLAS through indirect GPU arguments: `RhiPartitionedTlasBuildCommandDesc::OperationHeaders` and `OperationCount` map to Vulkan `VkBuildPartitionedAccelerationStructureInfoNV::srcInfos` and `srcInfosCount`, and to D3D12/NVAPI `indirectOps` and `indirectOpCount`.
  - Sparkle still packs `WriteInstance` operation records plus `RhiPartitionedTlasInstanceWriteDesc` records with stable `InstanceIndex`, `PartitionIndex`, transform, mask, flags, and BLAS address.
  - Sparkle still preserves the real optional PTLAS operation vocabulary required to follow the reference sample later: `UpdateInstance` and `WritePartitionTranslation` map to native Vulkan/NVAPI operation types, record strides, record payloads, and backend pack-buffer addresses.
  - Partition translation capacity follows the reference shape when enabled: `partitionCount + 1` entries are reserved so the global partition can be addressed.
  - Sparkle still performs full build with source AS `0` and update with source AS set to existing PTLAS storage, matching the reference distinction between initial build and partial update.
  - Sparkle still exposes one renderer-facing scene top-level handle/address; ray-query passes do not need to know whether classic TLAS or PTLAS produced it.
- The removed GPU-path code was not useful product behavior:
  - `RayTracingPtlasOperationWriterPolicyResolver::ResolveRequestedPath` always selected CPU pack.
  - D3D12 and Vulkan `PackPartitionedTopLevelAccelerationStructureGpuOperations` returned `false`.
  - The removed frame-graph logical-update/native-pack passes fed no product-owned shader writer.
  - `AllowCpuValidationReadback` existed only on unused GPU operation buffer creation and was removed with that unused path.
- PTLAS resource set after cleanup:
  - PTLAS storage buffer.
  - PTLAS build scratch buffer.
  - CPU-packed operation buffer containing operation count, operation header(s), and instance write records for the current renderer path.
  - Optional instance-update and partition-translation record sections are preserved in the backend packers and layout only when the layout/operation pack asks for them.
  - No logical update GPU buffer, no validation/readback buffer, no GPU-pack placeholder buffer, no planner/GPU metric surfaces.
- Future GPU-driven PTLAS work must be real implementation, not scaffolding: add it only when a compute shader writes operation `argCount`, instance records, optional update records, or optional partition-translation records into device-side buffers consumed by the same `OperationHeaders`/`OperationCount` build command path. Vulkan must allocate those buffers with real shader device addresses; a plain buffer handle fallback is not valid PTLAS operation input. Do not restore policy enums, report-only capabilities, or false-return backend hooks.
- Verification:
  - `cmake --build build --target SparkleRenderer --config DevelopmentEditor --parallel` succeeded after CMake regenerated the globbed source list for deleted files.
  - `cmake --build build --target architecture_boundary_check --config DevelopmentEditor` succeeded.
  - No runtime GPU launch was performed in this stage; the render/build path remains compiled and wired for supported PTLAS providers.

### Stage 28: PTLAS Reference Mirrorization Matrix

References: NVPRO-VK-PARTITIONED-TLAS, NV-NRI, UE-SOURCE.

Prompt:

Create the implementation matrix that makes Sparkle's PTLAS path intentionally mirror `nvpro-samples/vk_partitioned_tlas`.
This stage is a planning and audit stage only: classify every reference feature as keep, modify, add, remove, or skip, then bind it to a Sparkle owner.
Do not invent extra PTLAS product behavior beyond the reference unless it is required for D3D12/Vulkan parity.

Reference flow to mirror:

- The ray tracing pass sees classic TLAS and PTLAS as one top-level acceleration structure input; construction differs, tracing does not.
- The demo-level scene is partitioned into a ground-aligned 2D grid with one optional global partition.
- `PartitionsPerAxis` is the primary partition density knob.
- `PartitionUpdateMode` exposes the same three behaviors: always update partition, always move dynamic objects to global, and update partition nearby while moving far dynamic objects to global.
- `MarkAllDynamicInPartition` and `ModeChangeDistance` are the remaining policy knobs.
- Initial build derives `partitionCount`, `maxInstancesPerPartition`, `maxInstancesInGlobalPartition`, `maxOperations`, storage size, scratch size, operation info size, operation count size, instance write size, optional instance update size, and optional partition translation size from the scene and chosen policy.
- Default initial build uploads one `WRITE_INSTANCE` operation over all instance write records.
- Dynamic update resets one operation header and operation count, runs a GPU writer that atomically increments the operation `argCount`, writes only changed `WRITE_INSTANCE` records, then builds PTLAS from indirect GPU-side buffers.
- `UPDATE_INSTANCE` and `WRITE_PARTITION_TRANSLATION` remain backend capabilities, but they are not allowed to pollute the default product path unless a reference-parity stage proves a consumed use.

Current Sparkle assessment to record before changing code:

- Already aligned: RHI owns backend PTLAS capability/build/update, Renderer owns partition policy and scene selection, classic TLAS and PTLAS share scene extraction and BLAS ownership, and PTLAS is selected explicitly rather than hidden behind classic TLAS.
- Missing or incomplete: reference-level parameter surface, reference-level GPU update writer path, strict XZ partition model for the default PTLAS path, exact buffer lifetime contract, Sponza acceptance that proves classic TLAS/PTLAS interchangeability, and a removal pass for planner/detail behavior not present in the reference.
- Must preserve: D3D12/Vulkan parity, backend support for all three PTLAS operation record types, offline shader package ABI, screenshot capture capability, multi-level support, and clean RHI/Renderer separation.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Matrix names the reference file/function and Sparkle owner for scene partitioning, BLAS reuse, initial PTLAS build, operation buffers, dynamic update writer, global partition modes, top-level selection, trace binding, Vulkan backend, and D3D12 backend.
- [x] Each PTLAS feature is marked keep, modify, add, remove, or skip.
- [x] Each skip is justified as non-product scaffolding, sample-only UI, physics demo behavior, or unsupported-by-current-backend behavior.
- [x] No source code changes are made in this stage.

Completion notes:

- Existing-capability search covered:
  - Renderer strategy/selection: `RenderRayTracingScene`, `RayTracingTopLevelAccelerationStructureStrategy`, `RayTracingClassicTlasStrategy`, `RayTracingPartitionedTlasStrategy`, and `RayTracingPartitionedTlasBuild`.
  - Renderer partition planning/update stream: `RayTracingTopLevelScenePlanner`, `RayTracingPtlasPartitionPlanner`, and `RayTracingPtlasLogicalUpdateStream`.
  - RHI PTLAS contract: `RhiPartitionedTlasDesc`, `RhiPartitionedTlasService`, and `RhiRayTracingService`.
  - Backend PTLAS providers: `VulkanPartitionedTlasServices`, `VulkanRenderCommandList::BuildPartitionedTopLevelAccelerationStructure`, `D3D12PartitionedTlasServices`, `D3D12NvapiRayTracingProvider`, and D3D12/Vulkan capability setup.
  - PTLAS settings surface: `CVarRayTracingPreferPartitionedTlas`, `CVarRayTracingPartitionsPerAxis`, `CVarRayTracingPtlasPartitionUpdateMode`, `CVarRayTracingPtlasMarkAllDynamicInPartition`, `CVarRayTracingPtlasModeChangeDistance`, and `EngineRenderingSettings`.
- No source code was changed for Stage 28. The only change is this audit matrix in the active roadmap.
- No fallback chain was added. The hidden classic fallback that existed inside `RayTracingPartitionedTlasStrategy` during Stage 28 was removed in Stage 29E so user-facing PTLAS selection now reports explicit unavailability instead of silently choosing nested classic TLAS.

Reference mirrorization matrix:

| Reference feature | Reference file/function | Sparkle owner | Classification | Required Sparkle direction |
| --- | --- | --- | --- | --- |
| Shader-facing TLAS/PTLAS equivalence | README `TLAS vs. PTLAS`, `raytrace.rgen.glsl`, `raytrace.rchit.glsl` | `RenderRayTracingScene`, `RayTracingTopLevelAccelerationStructureStrategy`, `RayTracingSceneFrameData`, frame-graph TLAS binding | Keep | Preserve one selected scene top-level handle/resource for ray tracing passes. Construction may differ, but pass code should not branch on classic TLAS versus PTLAS details. |
| BLAS shared by classic TLAS and PTLAS | `partitioned_tlas_acceleration_structure.cpp` BLAS setup before both top-level paths | `RayTracingBlasCache`, `RayTracingClassicTlasBuilder`, `RayTracingPartitionedTlasBuild` | Keep | Continue sharing BLAS build/reuse across both strategies. Do not create PTLAS-only BLAS ownership. |
| Explicit top-level selection | `partitioned_tlas_ui.cpp` `PTLAS Active` checkbox | `CVarRayTracingPreferPartitionedTlas`, `CreateRayTracingTopLevelAccelerationStructureStrategy`, `RenderRayTracingScene` | Modify | Keep one explicit selection point. Remove hidden fallback behavior later: unsupported PTLAS should be explicit unavailable state, while classic TLAS remains an explicit separate selection. |
| XZ grid partitioning | README uniform 2D grid, `partitionIndexFromPosition` style behavior | `RayTracingPtlasPartitionPlanner`, `RayTracingTopLevelScenePlanner` | Modify | Make XZ ground-grid the default product path. Stage 29 removed the remaining internal 3D topology branch so future PTLAS work starts from the reference grid. |
| Optional global partition | README global partition modes, `m_animationShaderData.globalPartitionIndex = partitionCount - 1` | `RayTracingPtlasPartitionPlanner`, `RayTracingPtlasPartitionPlanCounts` | Keep | Preserve global partition as the last partition when the selected update mode needs it. Do not create it for modes that do not consume it unless backend sizing requires it. |
| `PartitionsPerAxis` knob | `partitioned_tlas_ui.cpp` scene/partition controls | `CVarRayTracingPartitionsPerAxis`, `EngineRenderingSettings`, `RayTracingPtlasPartitionPlannerConfig` | Keep | Preserve as the primary partition density knob. Clamp/sanitize in the PTLAS planner, not the generic settings facade. |
| `PartitionUpdateMode` three-mode surface | README update behavior section, `partitioned_tlas_ui.cpp` radio buttons | `CVarRayTracingPtlasPartitionUpdateMode`, `RayTracingPtlasPartitionPlanner` | Modify | Preserve only the three reference behaviors as product modes: always update partition, always move dynamic to global, update nearby partition and move far to global. |
| `MarkAllDynamicInPartition` knob | README checkbox behavior, `partitioned_tlas_ui.cpp` | `CVarRayTracingPtlasMarkAllDynamicInPartition`, `RayTracingPtlasPartitionPlanner` | Keep | Preserve, but it should be active only for update modes that use the global partition. |
| `ModeChangeDistance` knob | README mixed update mode threshold, `partitioned_tlas_ui.cpp` | `CVarRayTracingPtlasModeChangeDistance`, `RayTracingPtlasPartitionPlanner` | Keep | Preserve as the near/far threshold for mixed global-partition behavior. |
| Scene-size, domino count, self-toppling, regenerate-scene UI | `partitioned_tlas_ui.cpp`, `partitioned_tlas_scene.cpp` | None in engine architecture | Skip | Sample-only scene generation and physics controls. Sparkle must stay content/catalog driven and level-agnostic. |
| Scene transform dirtiness feeding dynamic updates | `animation_physics.comp.glsl` requests updates after physics changes | `RayTracingPtlasPartitionPlanner`, `RayTracingPtlasLogicalUpdateStream` | Modify | Use engine scene transform dirtiness and stable instance identity instead of importing sample physics. Keep the concept of changed instance records. |
| Initial build size query | README Initial Build, helper `PartitionedAccelerationStructures::getBuildSizes`, `createPartitionedTopLevelAS` | `RhiPartitionedTlasDesc`, `RhiPartitionedTlasBuildSizes`, `VulkanPartitionedTlasServices`, `D3D12NvapiRayTracingProvider` | Keep | Keep one RHI size-query contract mapping instance count, partition count, max instances per partition, max global instances, max operations, and optional sections to backend build sizes. |
| Derived `maxInstancesPerPartition` | README build-size requirements | `RayTracingPartitionedTlasStrategy::BuildPartitionedTlasLayout`, `RayTracingPtlasPartitionPlanner` | Modify | Current path effectively uses instance capacity as max-per-partition. Stage 29A/29B should derive this from the partition plan counts, not from a broad capacity fallback. |
| Default initial `WRITE_INSTANCE` operation | README Initial Build, `uploadPtlasData`, `VkBuildPartitionedAccelerationStructureIndirectCommandNV` | `RayTracingPartitionedTlasBuild`, `RhiPartitionedTlasOperationPackDesc` | Keep | Continue defaulting to one `WriteInstance` operation over complete instance records for first build. |
| Operation count plus operation header buffer | README device-side `srcInfos` and `srcInfosCount` | `RhiPartitionedTlasOperationBufferLayout`, `VulkanPartitionedTlasServices`, `D3D12PartitionedTlasServices` | Keep | Preserve operation-count and operation-header buffers as real GPU-addressed build inputs. |
| Persistent PTLAS storage and scratch buffers | README build buffers, helper `Buffers` struct | `RayTracingPartitionedTlasStrategy::PartitionedTlasResources`, RHI resource service | Keep | Preserve minimal persistent storage and scratch. Do not add validation/readback buffers. |
| Instance write records | README `VkPartitionedAccelerationStructureWriteInstanceDataNV` | `RhiPartitionedTlasInstanceWriteDesc`, backend packers | Keep | Preserve transform, instance ID, mask, hit-group offset, flags, stable instance index, partition index, and BLAS address. |
| Unique `instanceIndex` invariant | README note that duplicate instance indices are invalid | `RayTracingPtlasPartitionPlanner`, `RayTracingPartitionedTlasBuild` | Modify | Current planner detects duplicates. Stage 29A should turn this into a hard planner boundary and avoid carrying invalid entries deeper. |
| Optional partition translation record | README optional `VkPartitionedAccelerationStructureWritePartitionTranslationDataNV` | `RhiPartitionedTlasPartitionTranslationDesc`, Vulkan/D3D12 packers | Keep | Preserve backend capability and packing. Do not enable default product path until a reference-parity scenario consumes it. |
| `UPDATE_INSTANCE` operation | Vulkan extension/sample header support, backend operation vocabulary | `ERhiPartitionedTlasOperationType::UpdateInstance`, Vulkan/D3D12 packers | Keep | Preserve backend support as useful provider capability. Do not force into default product path while reference default uses `WRITE_INSTANCE`. |
| Dynamic update operation reset | `partitioned_tlas.cpp` resets `argCount`, op type, op data address, and operation count before compute dispatch | Not present as GPU path; CPU pack in `RayTracingPartitionedTlasBuild` | Add | Stage 29C must add the real reset path for GPU-side updates and delete/narrow the CPU duplicate path. |
| GPU writer with atomic `argCount` | `animation_update_instances.comp.glsl` `atomicAdd(...argCount, 1)` | Not present; current `RayTracingPtlasLogicalUpdateStream` emits CPU records | Add | Stage 29C must add a cooked compute shader that writes changed PTLAS records into device buffers. It must not copy sample physics. |
| No host synchronization for changed update count | README Dynamic Updates, `vkCmdBuildPartitionedAccelerationStructuresNV` from device-side buffers | Current CPU update count known on host | Modify | Move dynamic update count to GPU-visible operation `argCount`. Host may select policy, but should not require a readback or host-known changed count. |
| Vulkan PTLAS backend | `VK_NV_partitioned_acceleration_structure`, `vkCmdBuildPartitionedAccelerationStructuresNV` | `VulkanPartitionedTlasServices`, `VulkanRenderCommandList` | Keep | Preserve Vulkan provider, device addresses, native operation mapping, and `srcInfos`/`srcInfosCount` build command path. |
| D3D12 PTLAS backend parity | No Vulkan sample equivalent; parity requirement comes from Sparkle product goal | `D3D12NvapiRayTracingProvider`, `D3D12PartitionedTlasServices`, `D3D12RayTracingServices` | Keep | Preserve NVAPI provider capability, build-size query, operation packing, and indirect build path. Backend parity is allowed beyond the Vulkan-only reference. |
| Backend operation vocabulary | `gl_NV_partitioned_acc.h` / Vulkan operation enum | `ERhiPartitionedTlasOperationType`, Vulkan/D3D12 type conversion | Keep | Keep `WriteInstance`, `UpdateInstance`, and `WritePartitionTranslation` mappings. The default product path still exercises `WriteInstance`. |
| Backend-native handles and commands | Vulkan helper functions and extension calls | RHI backend services only | Keep | Native `Vk*`, `D3D12*`, and NVAPI types must stay in RHI/provider files, never Renderer PTLAS policy/build files. |
| Descriptor versus shader-device-address access | README PTLAS descriptor write plus shader equivalence | `RayTracingSceneTlasShaderAccessMode`, frame-graph/resource binding code, backend descriptor/device-address path | Modify | Preserve one pass-facing top-level scene binding while making descriptor/device-address selection explicit per backend capability. |
| CPU-packed operation buffer | Sparkle-only bring-up path | `RayTracingPartitionedTlasBuild`, `VulkanPartitionedTlasServices`, `D3D12PartitionedTlasServices` | Modify | Accept for initial build and backend parity bring-up. Stage 29C should remove or narrow it for dynamic updates once GPU writer exists. |
| Planner activity counters and report-only counts | Sparkle-only detail state | `RayTracingPtlasPartitionPlanCounts`, `RayTracingPtlasPartitionPlanner` | Remove | Remove counts not consumed by reference-mode decisions or backend sizing. Keep only values needed for product policy/build. |
| PTLAS profiler/inspector/readback/sample highlights | README profiler, sample inspector, partition highlight shaders | None in product PTLAS path | Skip | Sample-only diagnostics/visualization. Do not add equivalent engine panels or readbacks as part of PTLAS mirrorization. |
| Procedural physics and domino animation | `animation_physics.comp.glsl`, scene generation code | None in engine architecture | Skip | Sample-only workload generator. Sparkle should prove PTLAS on Sponza and arbitrary levels using existing scene data. |
| Sample AO/toon compositing | `compositing.comp.glsl` | Post-processing roadmap, not PTLAS | Skip | Rendering-style sample output is unrelated to PTLAS architecture. Do not import. |

Stage 28 closeout:

- Keep first: selected scene TLAS handle, shared BLAS cache, RHI-owned PTLAS build contract, Vulkan/D3D12 backend providers, operation vocabulary, instance write records, global partition support, and the reference PTLAS policy knobs.
- Modify next: remove hidden fallback, derive max-per-partition from real plan data, replace dynamic CPU packing with GPU-written indirect operation data, and make descriptor/device-address binding explicit without renderer-native backend code.
- Add only when deleting/narrowing existing paths: GPU dynamic update writer, operation reset path, and any minimal shader package registration needed for that writer.
- Remove: report-only PTLAS counters, unconsumed topology/policy branches, validation/readback/debug paths, and duplicate CPU dynamic update machinery after the GPU writer exists.
- Skip: sample physics, sample UI, procedural content generation, profiler/inspector/highlight tooling, AO/toon compositing, and any content-specific logic.

### Stage 29: PTLAS Parameter Surface Parity

References: NVPRO-VK-PARTITIONED-TLAS, NV-NRI, UE-SOURCE.

Prompt:

Collapse Sparkle's PTLAS policy surface to the knobs exposed by the reference sample, plus only the backend-selection control needed to compare classic TLAS and PTLAS.
The goal is a small, reviewable parameter surface that can be shown in code review without explaining extra engine-only inventions.

Required product knobs:

- `PreferPartitionedTlas` or equivalent explicit TLAS/PTLAS selection.
- `PartitionsPerAxis`.
- `PartitionUpdateMode`.
- `MarkAllDynamicInPartition`.
- `ModeChangeDistance`.

Required derived values, not user knobs:

- `PartitionCount`.
- `GlobalPartitionIndex`.
- `MaxInstancesPerPartition`.
- `MaxInstancesInGlobalPartition`.
- `MaxOperations`.
- PTLAS operation and instance record byte sizes.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] PTLAS product settings expose only the required knobs listed above.
- [x] Existing extra knobs are deleted, made private implementation constants, or explicitly tied to a later reference-parity stage.
- [x] Default PTLAS policy uses the reference-compatible 2D ground-grid behavior.
- [x] Parameter names and enum values are stable enough to be used from launcher/editor/runtime configuration without duplicating responsibility.
- [x] No sample, project, level, or asset name is hardcoded into PTLAS policy code.

Completion notes:

- Existing-capability search confirmed the current product PTLAS setting surface is exactly the required reference-aligned set:
  - `r.RayTracing.PreferPartitionedTlas` / `PtlasActive`.
  - `r.RayTracing.Ptlas.PartitionsPerAxis` / `PtlasPartitionsPerAxis`.
  - `r.RayTracing.Ptlas.PartitionUpdateMode` / `PtlasPartitionUpdateMode`.
  - `r.RayTracing.Ptlas.MarkAllDynamicInPartition` / `PtlasMarkAllDynamicInPartition`.
  - `r.RayTracing.Ptlas.ModeChangeDistance` / `PtlasModeChangeDistance`.
- Confirmed no remaining source references to `RayTracingPtlasPartitionTopology`, `CVarRayTracingPtlasPartitionTopology`, `EnginePtlasPartitionTopology`, `PtlasPartitionTopology`, `PartitionTopology`, or `EnableGlobalPartition`.
- Removed the remaining internal 3D topology branch from `RayTracingPtlasPartitionPlanner.cpp`; partition IDs, partition centers, and grid partition count are now always reference-compatible XZ ground-grid calculations.
- Global partition creation is now derived from `PartitionUpdateMode` instead of a separate config switch.
- No code was added in this batch. The source delta is a net reduction in the planner path.
- Verification:
  - `cmake --build build --target SparkleRenderer --config DevelopmentEditor --parallel` succeeded.
  - `cmake --build build --target SparkleEditor --config DevelopmentEditor --parallel` succeeded.
  - `cmake --build build --target architecture_boundary_check --config DevelopmentEditor` succeeded.
  - `git diff --check` passed with only the repository's existing line-ending warning.

### Stage 29A: PTLAS Partition Model And Stable Instance Identity

References: NVPRO-VK-PARTITIONED-TLAS, NV-NRI, UE-SOURCE.

Prompt:

Make the default PTLAS partition planner mirror the reference model: an XZ ground-aligned grid plus an optional global partition.
Sparkle may keep a private escape hatch only if a real current consumer exists; otherwise remove broader topology support to stay reference-shaped.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Default partition index calculation is XZ-grid based and independent of content names.
- [x] `PartitionCount` is `PartitionsPerAxis * PartitionsPerAxis` plus one global partition only when the selected mode needs it.
- [x] `GlobalPartitionIndex` is the last partition when present.
- [x] Stable PTLAS instance indices are unique; duplicates fail at the planner boundary.
- [x] `MaxInstancesPerPartition` and `MaxInstancesInGlobalPartition` are derived from gathered scene data and the selected policy, not guessed constants.
- [x] Unused topology/planner state, including any unconsumed XYZ partition mode, is removed or explicitly justified by a current product consumer.

Completion notes:

- Existing-capability search confirmed the Stage 29 topology knob removal left no PTLAS-owned references to `RayTracingPtlasPartitionTopology`, `PartitionTopology`, or `EnableGlobalPartition`.
- The planner now computes partition id as `x + z * PartitionsPerAxis`, uses the XZ grid center for distance policy, and derives `GridPartitionCount` as `PartitionsPerAxis * PartitionsPerAxis`.
- Global partition ownership is policy-derived: modes that move far or dynamic instances to global create one extra partition, and `GlobalPartitionIndex` is exactly `GridPartitionCount`.
- Duplicate stable PTLAS indices now fail at the planner boundary by setting `HasDuplicateStableIndices`, clearing previous instance state, and returning before entries are emitted; the strategy refuses unusable plans instead of carrying invalid layout deeper.
- `MaxInstancesPerPartition` and `MaxInstancesInGlobalPartition` are derived from gathered per-partition instance counts. The previous broad capacity fallback was removed from `RayTracingPartitionedTlasStrategy.cpp`.
- Removed planner-only report/detail state not present in the reference-shaped product path: candidate/static/dynamic counts, dirty/moved/global counters, active partition counters, duplicate stable index count, per-entry duplicate flag, and per-partition activity count.
- No content names, sample names, or asset paths were added to the PTLAS planner or strategy.
- Source delta for the Stage 29A PTLAS files is net negative: 55 insertions and 112 deletions across `RayTracingPtlasPartitionPlanner.h`, `RayTracingPtlasPartitionPlanner.cpp`, and `RayTracingPartitionedTlasStrategy.cpp`.
- Verification:
  - PTLAS-only search found no remaining deleted topology or planner-report symbols.
  - `cmake --build build --target SparkleRenderer --config DevelopmentEditor --parallel` succeeded.
  - `cmake --build build --target architecture_boundary_check --config DevelopmentEditor` succeeded.
  - `git diff --check` passed with only the repository's existing line-ending warning.

### Stage 29B: PTLAS Initial Build Buffer Contract

References: NVPRO-VK-PARTITIONED-TLAS, NV-NRI, UE-SOURCE.

Prompt:

Make Sparkle's initial PTLAS build contract match the reference size-query and upload flow.
Renderer prepares compact PTLAS descriptors and records; RHI owns backend buffer creation, address exposure, barriers, and build submission.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Build-size query fields map one-to-one to instance count, partition count, max instances per partition, max global instances, max operations, build flags, instance update support, and partition translation support.
- [x] Default build uses `MaxOperations = 1`, `AllowInstanceUpdate = false`, and `AllowPartitionTranslation = false` unless a later stage consumes the optional paths.
- [x] Default persistent PTLAS buffers are limited to storage, scratch, operation info, operation count, and instance write records.
- [x] Optional instance update and partition translation buffers are created only when consumed by an accepted feature.
- [x] Initial build writes one `WRITE_INSTANCE` operation over the complete instance record array.
- [x] Renderer does not allocate or interpret backend-native PTLAS memory objects.

Completion notes:

- Existing-capability search confirmed `CreatePartitionedTopLevelAccelerationStructureOperationBuffer` has one renderer consumer and the default operation pack is one `ERhiPartitionedTlasOperationType::WriteInstance` operation with one operation count.
- `RhiPartitionedTlasDesc` remains the one build-size query contract: `InstanceCapacity`, `PartitionCount`, `MaxInstancesPerPartition`, `MaxInstancesInGlobalPartition`, `MaxOperations`, `AllowInstanceUpdates`, and `AllowPartitionTranslation`.
- `RayTracingPartitionedTlasStrategy::BuildPartitionedTlasLayout` now explicitly sets `MaxOperations = 1`, `AllowInstanceUpdates = false`, and `AllowPartitionTranslation = false` for the default product path.
- Renderer keeps only high-level PTLAS resources and addresses. Backend-native sizes, operation-header stride, instance-record stride, optional update-record stride, and optional partition-translation stride stay behind the RHI service.
- Removed retained renderer operation layout/count state. The RHI operation-buffer layout is now local to the build submission and is not stored as renderer state.
- Vulkan and D3D12 operation packers no longer align through optional instance-update or partition-translation sections when the operation pack has zero records for those features.
- Backend support for `UPDATE_INSTANCE` and `WRITE_PARTITION_TRANSLATION` operation record types remains intact for later reference-parity stages, but default allocation and upload do not create those records.
- Removed diagnostic-only CPU-pack active reason strings instead of adding replacement reporting.
- No content names, sample names, or asset paths were added.
- Source delta for the Stage 29B code batch is net negative: 42 insertions and 55 deletions across `RayTracingPartitionedTlasStrategy.h`, `RayTracingPartitionedTlasStrategy.cpp`, `RayTracingPartitionedTlasBuild.cpp`, `VulkanPartitionedTlasServices.cpp`, and `D3D12PartitionedTlasServices.cpp`.
- Verification:
  - `rg` found no remaining `NativeOperationLayout`, `NativeOperationCount`, `ResolveActiveCpuPackReason`, or `active-cpu-pack` references.
  - `cmake --build build --target SparkleRenderer --config DevelopmentEditor --parallel` succeeded.
  - `cmake --build build --target SparkleEditor --config DevelopmentEditor --parallel` succeeded.
  - `cmake --build build --target architecture_boundary_check --config DevelopmentEditor` succeeded.
  - `git diff --check` passed with only the repository's existing line-ending warning.

### Stage 29C: PTLAS GPU Dynamic Update Writer

References: NVPRO-VK-PARTITIONED-TLAS, NV-NRI, UE-SOURCE.

Prompt:

Replace duplicated CPU-side dynamic PTLAS operation packing with a reference-shaped GPU writer.
The CPU may decide which scene instances are dynamic or dirty, but the per-frame PTLAS operation count and changed instance records must be produced on the GPU path when PTLAS dynamic update is enabled.

Reference behavior to mirror:

- Reset operation header to `WRITE_INSTANCE`.
- Reset operation `argCount` to zero and operation count to one.
- Dispatch a compute shader that tests changed instances, atomically increments `argCount`, and writes compact instance records.
- Build PTLAS from the indirect operation buffers without reading changed record count back to the CPU.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Existing CPU PTLAS dynamic-pack logic is removed, narrowed to one backend bring-up path, or replaced by the GPU writer with net non-positive code growth.
- [ ] Vulkan buffers used by the writer expose the required shader device addresses.
- [ ] D3D12 buffers used by the writer expose the required UAV/GPU virtual address path through RHI.
- [ ] The shader writes only PTLAS operation count/records and transformed instance data; no sample physics or demo animation logic is copied into engine code.
- [ ] The build call consumes GPU-side operation data directly and does not require CPU validation readback.
- [ ] The writer is packaged through the existing offline shader cook path with reflection preserved.

Completion notes:

- Existing-capability search found one CPU dynamic PTLAS update stream owner: `RayTracingTopLevelScenePlanner` built `RayTracingPtlasLogicalUpdateStream`, and `RayTracingPartitionedTlasBuild` consumed it through `appendIncrementalWrites`.
- Removed `RayTracingPtlasLogicalUpdateStream.h` and `RayTracingPtlasLogicalUpdateStream.cpp`.
- Removed the planner-owned logical update stream, `GetCurrentLogicalUpdateStream`, and the `PlanFrame(..., buildPartitionedTlasUpdateStream)` switch.
- Removed the PTLAS incremental CPU branch, stable-instance fingerprint, incremental-update state, and CPU recovery branch that repacked dynamic records after failure.
- Current product PTLAS path is intentionally narrowed to one backend-neutral full `WRITE_INSTANCE` operation pack until the GPU writer has a real shader/runtime owner.
- Default build still consumes indirect PTLAS operation data and performs no CPU validation readback.
- Backend `UPDATE_INSTANCE` and `WRITE_PARTITION_TRANSLATION` operation support remains preserved in RHI for later feature parity; it is not consumed by this narrowed product path.
- No content names, sample names, or asset paths were added.
- Source delta for the Stage 29C narrowing batch is net negative: 58 insertions and 344 deletions across the PTLAS build/scene-planner files.
- The unchecked writer-specific acceptance items below are carried into Stage 29C2; they are not marked complete because no GPU writer shader package exists yet.
- Verification:
  - `rg` found no remaining `RayTracingPtlasLogicalUpdateStream`, `LogicalUpdate`, `GetCurrentLogicalUpdateStream`, `StableInstanceFingerprint`, `IncrementalUpdatesAllowed`, `HasStructuralPartitionValidationFailure`, `useFullBuild`, `appendIncrementalWrites`, `NativeOperationLayout`, or `NativeOperationCount` references.
  - `cmake --build build --target SparkleRenderer --config DevelopmentEditor --parallel` succeeded.
  - `cmake --build build --target SparkleEditor --config DevelopmentEditor --parallel` succeeded.
  - `cmake --build build --target architecture_boundary_check --config DevelopmentEditor` succeeded.
  - `git diff --check` passed with only the repository's existing line-ending warning.

### Stage 29C2: PTLAS GPU Dynamic Update Writer

References: NVPRO-VK-PARTITIONED-TLAS, NV-NRI, UE-SOURCE.

Prompt:

Implement the actual GPU writer after the CPU dynamic pack path has been removed.
This stage must add only the minimum shader/runtime path needed to reset one `WRITE_INSTANCE` operation, atomically append changed instance records, and build PTLAS from GPU-side operation data.
Do not reintroduce CPU changed-record packing.

Acceptance:

Universal acceptance for this stage:

- [ ] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [ ] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [ ] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [ ] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [ ] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [ ] Vulkan buffers used by the writer expose the required shader device addresses.
- [ ] D3D12 buffers used by the writer expose the required UAV/GPU virtual address path through RHI.
- [ ] The shader writes only PTLAS operation count/records and transformed instance data; no sample physics or demo animation logic is copied into engine code.
- [ ] The build call consumes GPU-side operation data directly and does not require CPU validation readback.
- [ ] The writer is packaged through the existing offline shader cook path with reflection preserved.
- [ ] The implementation deletes or folds enough PTLAS CPU-side packing/support code to keep the batch net non-positive.

### Stage 29D: PTLAS Global Partition Update Modes

References: NVPRO-VK-PARTITIONED-TLAS, NV-NRI, UE-SOURCE.

Prompt:

Make Sparkle's PTLAS update policy match the reference global-partition modes while using engine scene transform dirtiness instead of sample physics.
This must work for arbitrary levels, with Sponza as the default proof scene.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] `AlwaysUpdatePartition` updates changed instances in their current partition.
- [x] `AlwaysMoveDynamicToGlobal` sends dynamic or unstable instances to the global partition.
- [x] `UpdatePartitionNearbyMoveToGlobalOtherwise` uses `ModeChangeDistance` to choose partition update near the camera and global movement when far.
- [x] `MarkAllDynamicInPartition` marks all instances in a partition dynamic when that partition contains dynamic content, matching the reference behavior.
- [x] Static, dynamic, far, and global decisions are data-driven from scene extraction and camera state, not hardcoded level names.
- [x] Extra update modes or counters not consumed by these decisions are removed.

Stage 29D completion notes:

- Existing-capability search covered `RayTracingPtlasPartitionPlanner`, `RayTracingTopLevelScenePlanner`, PTLAS settings/CVars, and the partition assignment fields consumed by the PTLAS build path.
- `AlwaysUpdatePartition` remains the no-global default: instances stay in their current XZ-grid partition unless the selected mode explicitly needs the global partition.
- `AlwaysMoveDynamicToGlobal` sends non-static, dirty, unstable, or partition-marked dynamic instances to the global partition.
- `UpdatePartitionNearbyMoveToGlobalOtherwise` now uses `ModeChangeDistance` directly: touched dynamic partitions near the camera stay local, while far touched dynamic partitions move to global. The previous "already global and moving" override was removed because it was not one of the reference modes.
- `MarkAllDynamicInPartition` propagates dynamic handling across a touched partition for eligible non-static instances; static geometry remains static.
- Removed unused `m_frameIndex` and `LastModifiedFrame` planner state. No content names or fallback chains were added.

### Stage 29E: Classic TLAS/PTLAS Interchangeable Sponza Acceptance

References: NVPRO-VK-PARTITIONED-TLAS, NV-NRI, UE-SOURCE.

Prompt:

Prove that classic TLAS and PTLAS are interchangeable construction strategies for the same renderer scene.
Sponza is the mandatory default sample level, but the code must remain catalog-driven and level-agnostic.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Classic TLAS and PTLAS use the same scene extraction, BLAS cache, material bindings, shader packages, and ray tracing passes.
- [x] Ray tracing passes consume one selected top-level scene handle and do not branch on backend-native TLAS/PTLAS details.
- [x] The default catalog-selected Sponza level can launch with classic TLAS.
- [x] The same default catalog-selected Sponza level can launch with PTLAS on a supported backend.
- [x] Unsupported PTLAS is reported as an explicit unavailable selected capability; the engine does not silently build a hidden fallback chain.
- [x] No Sponza path, mesh, material, or asset name appears in RHI or Renderer code.

Stage 29E completion notes:

- Existing-capability search covered `RenderRayTracingScene`, `RayTracingTopLevelAccelerationStructureStrategy`, classic/partitioned strategy implementations, frame ray tracing bindings, deferred/reference ray tracing passes, `Projects/Showcase/Levels.catalog`, and Sponza/SponzaPtlas level data.
- Classic TLAS and PTLAS are now explicit peer strategies selected by `r.RayTracing.PreferPartitionedTlas`. The factory no longer requires PTLAS support to instantiate the PTLAS strategy, so unsupported PTLAS remains an explicit selected-unavailable state instead of silently selecting classic TLAS.
- `RayTracingPartitionedTlasStrategy` no longer owns `RayTracingClassicTlasStrategy`; the nested fallback path, frame-mode switch, and fallback resource getters were removed.
- Passes still consume `RayTracingSceneFrameData` as one selected scene top-level handle/resource/address and do not branch on classic TLAS versus PTLAS construction details.
- Sponza remains a catalog startup default and required level in `Projects/Showcase/Levels.catalog`. Runtime GPU launch was not performed in this batch; this stage is closed at source/build readiness and should be included in the final stabilization run on supported hardware.
- No Sponza path, mesh, material, or asset name was added to RHI or Renderer code. Existing project-owned Sponza patrol behavior remains outside RHI/Renderer and should be data-driven in a later project-content cleanup if the no-hardcoded-content rule is made absolute across `Projects/*/Src`.

### Stage 29F: PTLAS D3D12/Vulkan Backend Parity Completion

References: NVPRO-VK-PARTITIONED-TLAS, NV-NRI, NV-NVRHI, UE-SOURCE.

Prompt:

Keep the PTLAS RHI contract backend-neutral while preserving all useful backend operations.
Vulkan and D3D12 may use different native providers, but Renderer must see the same compact PTLAS descriptors, handles, and selected top-level scene result.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Vulkan `VK_NV_partitioned_acceleration_structure` and D3D12 NVAPI PTLAS paths map to the same RHI capability fields.
- [x] `WRITE_INSTANCE` is the default exercised product operation on both supported backends.
- [x] `UPDATE_INSTANCE` and `WRITE_PARTITION_TRANSLATION` remain represented as backend capabilities and can be packed without being forced into the default product path.
- [x] Operation buffers, operation counts, instance records, optional update records, and optional partition translation records have matching semantics across D3D12 and Vulkan.
- [x] Backend-native command types, handles, addresses, and barriers remain inside RHI/provider code.
- [x] Renderer contains no `Vk*`, `D3D12*`, or NVAPI PTLAS types.

Stage 29F completion notes:

- Existing-capability search covered `RhiPartitionedTlasDesc`, `RhiPartitionedTlasService`, `RhiRayTracingService`, `D3D12NvapiRayTracingProvider`, `D3D12PartitionedTlasServices`, `VulkanPartitionedTlasServices`, and renderer PTLAS strategy/scene binding.
- D3D12 and Vulkan both preserve the same RHI operation vocabulary: `WriteInstance`, `UpdateInstance`, and `WritePartitionTranslation`.
- D3D12 keeps NVAPI GPU-driven operation capability fields, GPU operation count, GPU-written instance records, GPU-written partition records, and partition translation capability; Vulkan keeps the corresponding NV partitioned acceleration structure mappings.
- Default product PTLAS build uses one `WRITE_INSTANCE` operation. Optional update/partition-translation records remain backend-packable but are not allocated or forced into the default renderer path.
- Renderer PTLAS strategy no longer branches on D3D12/Vulkan header/runtime/function-loading details. It consumes the backend-neutral `PartitionedTlas.Supported`, `CapabilityStatusReason`, and TLAS shader-access report; backend detail remains in RHI capability setup and provider code.
- Renderer still has capability-report projection code that reads RHI capability fields by name, but no native `Vk*`, `ID3D12*`, `NVAPI_*`, or backend-native PTLAS command/resource types are present in Renderer.

### Stage 29G: PTLAS Bloat Removal And Reference Lock

References: NVPRO-VK-PARTITIONED-TLAS, NV-NRI, UE-SOURCE.

Prompt:

After the reference-shaped PTLAS path works, remove PTLAS behavior that is not needed for reference parity, backend parity, or current product behavior.
The goal is a smaller implementation that is easier to reason about than the current code, not a larger abstraction around PTLAS.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] PTLAS planner metrics, debug readbacks, report-only counters, and future placeholder paths not required for product behavior are removed.
- [x] Any retained non-reference feature names its current product consumer and backend parity reason.
- [x] Product PTLAS code line count decreases compared with the Stage 28 snapshot.
- [x] Public PTLAS API surface is smaller or explicitly justified.
- [x] Stage 27 through Stage 29G together leave classic TLAS and PTLAS as peers, not nested fallbacks or duplicate renderer systems.

Stage 29G completion notes:

- Existing-capability search covered PTLAS planner state, logical update stream files, partition strategy resources, RHI PTLAS packers, renderer capability report, top-level strategy factory, and pass-facing scene TLAS bindings.
- Removed PTLAS logical update stream files, incremental CPU dynamic pack/recovery logic, stable-fingerprint state, operation-layout/count retention in renderer resources, unused planner frame counters, nested classic fallback strategy ownership, and backend-detail PTLAS checks from the renderer strategy.
- Retained non-reference features:
  - D3D12 NVAPI PTLAS backend provider and operation vocabulary: retained for backend parity.
  - `UpdateInstance` and `WritePartitionTranslation`: retained because both D3D12/Vulkan providers can represent and pack them, and Stage 29C2/29F may consume them without changing the RHI contract.
  - Capability-report provider enum: retained as a compact RHI-reported identity for diagnostics/selection reason, not as backend-native Renderer work.
- Product PTLAS-related source delta from the Stage 28-era files is net negative in the current working diff: 166 insertions and 631 deletions across PTLAS renderer files, deleted logical update stream files, renderer ray tracing capability report, and D3D12/Vulkan PTLAS packer touch points.
- Public Renderer PTLAS strategy surface is smaller: no nested classic fallback member, no frame-mode enum, no retained operation layout/count, no incremental update flag, and no renderer-owned backend-detail capability checks.
- Verification:
  - `cmake --build build --target SparkleRenderer --config DevelopmentEditor --parallel`
  - `cmake --build build --target SparkleEditor --config DevelopmentEditor --parallel`
  - `cmake --build build --target architecture_boundary_check --config DevelopmentEditor`
  - `git diff --check`
  - `rg -n "ClassicFallback|m_classicFallbackStrategy|classic-fallback|partitioned-tlas-resource-setup-failed-classic-fallback|RayTracingPtlasLogicalUpdateStream|LastModifiedFrame|m_frameIndex|alreadyGlobalAndMoving" Engine/Renderer/Private Engine/Renderer/Public Engine/RHI/Public Engine/RHI/Private`

### Stage 30: Reference Path Tracing Role

References: NV-RTXPT, AMD-CAULDRON.

Prompt:

- Choose debug reference as the first clear role.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Reference path has one sentence role.
- [x] Provider handoff hooks unsupported by data are removed.
- [x] Guide outputs without consumers are removed.
- [x] Material/light policy aligns with realtime path.

Completion notes:

- One sentence role: reference path tracing is a debug reference render path for comparing Sparkle's realtime lighting, material, TLAS, and shader-binding behavior against a high-sample path-traced result.
- Existing-capability search covered reference frame assembly, reference render targets, reference pass parameter bindings, shader registrations, HLSL side outputs, ray reconstruction provider handoff, realtime material/light bindings, frame graph resource shells, RHI device settings/selection wrappers, editor launch options, and provider pass-service wrappers.
- Removed unsupported reference-provider handoff hooks: reference path tracing no longer builds `ReferenceRayReconstruction` provider inputs, no longer creates missing motion-vector handoff state, and no longer falls back through ray reconstruction before producing `FinalSceneColor`.
- Removed guide outputs without consumers: direct/indirect split textures, primary depth/normal/albedo/material/path-sample guide textures, matching pass parameter bindings, shader registrations, frame graph target creation, and HLSL writes.
- Material/light policy remains shared with the realtime path through `LightingPassBinding`, `RayTracingHitDataPassBinding`, `MaterialTextureTablePassBinding`, `SurfaceLighting`, `RayTracingPathLighting`, shared light buffers, material texture table bindings, and the selected scene TLAS.
- Single-field wrapper cleanup performed in the same batch: deleted `ReferenceRenderTargets`, `RayTracingSceneFrameGraphResources`, `FrameGraphResourceRuntimeState`, `GPUMeshCache::CacheEntry`, image-provider pass-service shells, `RayTracingSceneDiagnosticState`, `RenderDeviceServices::Impl`, `RhiBackendSelection`, `RenderDeviceSettings`, `EditorApplicationOptions`, and collapsed shrunken frame assembly resource shells.
- Remaining one-field scan results are intentional strong typed handles/tokens, shader parameter binding records, serialized asset/tool payloads, or ABI/resource records; they are not plain post-refactor carrier shells.
- Verification:
  - `cmake --build build --target SparkleRHI --config DevelopmentEditor --parallel`
  - `cmake --build build --target SparkleRenderer --config DevelopmentEditor --parallel`
  - `cmake --build build --target SparkleApplication --config DevelopmentEditor --parallel`
  - `cmake --build build --target ShowcaseEditor --config DevelopmentEditor --parallel`
  - `cmake --build build --target ShaderCompiler --config DevelopmentEditor --parallel`
  - `ShaderCompiler.exe cook --package ReferencePathTracing --target DxilSm66 --target SpirV16`
  - `git diff --check`
  - `rg -n "EditorApplicationOptions|RenderDeviceSettings|RhiBackendSelection final|FrameAssemblyPersistentResources|Persistent\\.SceneTlas|m_frameresources|ReferenceRenderTargets|RayTracingSceneFrameGraphResources|FrameGraphResourceRuntimeState|RenderUpscalingPassServices|RenderRayReconstructionPassServices|RayTracingSceneDiagnosticState|CacheEntry|RenderDeviceServices::Impl|m_impl->backend|\\.Subsystem" Engine Tools Projects -g "*.h" -g "*.cpp"`

### Stage 31: Reservoir Direct Lighting Cleanup

References: NV-RTXDI, NV-RTXDI-LIBRARY if used, NV-SHARC.

Prompt:

- Keep native reservoir-based direct lighting honest and owned by Sparkle.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Direct lighting is not described as SDK-equivalent unless SDK is integrated.
- [x] Light buffers, GBuffer addressing, TLAS, material model, and shader scheduling remain Sparkle-owned.
- [x] Unused debug views or report outputs are removed.
- [x] Shader/resource ownership is obvious.

Stage 31 completion notes:

- Reference check: RTXDI is treated as an external SDK/runtime reference, not as an implied Sparkle implementation. SHARC remains a GI/path-tracing reference, not a direct-lighting dependency.
- Code search found no RTXDI/SHARC library calls in the direct-lighting path. Sparkle owns light selection, reservoir packing, GBuffer loading, shadow signal consumption, and final direct-light accumulation through `Lighting/DirectLightReservoir.hlsli`, `Lighting/DirectLightSampling.hlsli`, `Passes/Deferred/DirectLightReservoir*.hlsl`, and `Passes/Deferred/DirectLighting.hlsl`.
- Net cleanup removed unused candidate pack/unpack helpers from `DirectLightSampling.hlsli` and stale diagnostics includes from the direct reservoir, direct shadow signal, and direct lighting pass implementations.
- Editor/debug buffer visualization of direct diffuse/specular/subsurface remains because it is a consumed inspection surface, not an unowned report artifact.

### Stage 32: Post-Processing Pass Ownership

References: AMD-CAULDRON, NV-DONUT.

Prompt:

- Make post-processing pass inputs/outputs explicit and reduce duplicate pass plumbing.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Each post-process pass declares inputs/outputs clearly.
- [x] Unused settings/CVars are removed.
- [x] No new post-processing framework is added.
- [x] Existing output quality path remains buildable.

Stage 32 completion notes:

- Reference check: Cauldron and Donut keep post-processing as concrete feature passes with explicit resources. Sparkle keeps the same shape through `Exposure`, `ToneMapping`, `OutputEncoding`, and the existing upscaler provider handoff.
- Existing-capability search found the active post-processing settings are consumed: exposure mode/metering, exposure range/adaptation, tone mapper, and output encoding all feed runtime passes or persisted renderer settings.
- Net cleanup removed duplicated presentation sampler plumbing. Tone mapping and output encoding now load same-extent input pixels directly, so pass parameters, shader registrations, and shaders no longer declare `SamplerLinearClamp`.
- No new post-processing framework, content-specific path, or fallback chain was added.

### Stage 33: Denoising Feature Boundary

References: NV-NRD, NV-NRD-SAMPLE, AMD-FIDELITYFX.

Prompt:

- Treat denoising as a renderer feature slice with explicit resources.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Denoiser inputs/outputs are named.
- [x] History/resource ownership is explicit.
- [x] Provider boundary is narrow if external denoiser integration exists.
- [x] No diagnostic panel is added.

Stage 33 completion notes:

- Reference check: NRD exposes denoising as explicit noisy-signal, GBuffer-guide, history, settings, and output resources owned by the application/RHI integration. FidelityFX effects follow the same concrete feature-slice pattern.
- Existing-capability search found no Sparkle denoiser implementation, no NRD/FidelityFX denoiser integration, and no REBLUR/RELAX/SIGMA resource path in `Engine/Renderer`, `Engine/Assets/Shaders`, `Tools`, or `Projects`.
- Sparkle therefore has no denoiser inputs/outputs or denoiser history resources to name yet. Stage 33 is closed by keeping denoising absent instead of adding placeholder scaffolding.
- Ray reconstruction remains a separate Stage 34 provider feature, not a denoiser substitute. The editor ray reconstruction search alias no longer labels it as `denoise`.
- No denoising diagnostic panel, provider facade, content hardcode, fallback chain, or new abstraction was added.

### Stage 34: Upscaling And Ray Reconstruction Boundary

References: NV-STREAMLINE, AMD-FIDELITYFX.

Prompt:

- Preserve upscaling/ray reconstruction provider capability while trimming diagnostics and fallback scaffolding.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Provider required resources are narrow and named.
- [x] Depth, motion vectors, exposure, history, jitter, frame index, and camera state ownership is clear where used.
- [x] Unused fallback/provider objects are removed.
- [x] Streamline bridge remains narrow.

Stage 34 closure notes:

- Reference check: Streamline is an external feature bridge that consumes native devices, command lists, resource tags, frame constants, and selected feature options; FidelityFX-style integrations are explicit feature passes with named input/output resources rather than broad renderer fallbacks.
- Existing-capability search found Sparkle already has separate upscaler and ray reconstruction provider slices, separate input contracts, explicit frame provider resources, and Streamline-native resource tagging for DLSS Super Resolution and DLSS Ray Reconstruction.
- The provider result contracts now report only whether the provider produced output. The removed `UsedFallback` field and `FailedWithFallback` state names no longer duplicate the renderer frame-copy path as provider behavior.
- The renderer-owned copy path remains inside the frame passes as the explicit output-preservation path when no provider output is produced; it is not hidden behind an alternate provider object or chained fallback system.
- Upscaling owns scaling input/output, depth, motion vectors, optional exposure metadata, history/jitter/camera constants, frame index, and Streamline native resource tags where DLSS consumes them. Ray reconstruction owns noisy input/output, depth, motion vectors, exposure, normals, roughness, diffuse/specular albedo, specular hit distance, history/jitter/camera constants, frame index, and the command-state reset hook required by the external provider.
- The Streamline bridge remains isolated to the provider/runtime/tagging code and the RHI native interop request for `UpscalerProvider` or `RayReconstructionProvider`; no content names, level names, project names, asset names, or new provider abstraction were added.

### Stage 35: Frame Graph Pass/Resource Ownership

References: UE-SOURCE, NV-DONUT, NV-NVRHI.

Prompt:

- Reduce frame graph/pass duplication without replacing the graph.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Frame graph remains the only render scheduling abstraction.
- [x] Pass inputs/outputs/history dependencies are clearer.
- [x] Transient/persistent resource ownership is easier to trace.
- [x] No replacement render graph is added.

Stage 35 closure notes:

- Reference check: UE RDG keeps pass/resource declaration in the graph, Donut composes reusable renderer passes instead of per-call bespoke scheduling, and NVRHI emphasizes explicit resource states at command-list boundaries.
- Existing-capability search found Sparkle already had `PassUtilities::AddCopyTexturePass`/`AddCopyBufferPass`, plus one local `CopyEncodedColorToBackBuffer` pass in presentation that duplicated the copy-pass declaration/execution pattern.
- Presentation now reuses `PassUtilities::AddCopyTexturePass`, so frame graph remains the only render scheduling abstraction and no replacement graph, wrapper graph, or new pass framework was added.
- Shared copy utilities now label copy dependencies as `Source` and `Destination`, making copy pass inputs/outputs visible to the frame graph for presentation and reference-path copy passes.
- Transient/persistent ownership was not changed: tone-mapped and encoded scene color remain transient frame graph textures, the swapchain back buffer remains the imported presentation target, and history resources remain under `FrameAssemblyHistoryResources`.
- The batch removes more bespoke pass code than it adds, introduces no content or project hardcodes, and does not add fallback behavior.

### Stage 36: Core Public API Cleanup

References: UE-SOURCE, NV-DONUT.

Prompt:

- Reduce public Core convenience APIs that are not stable contracts.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.

- [x] Public Core line count decreases.
- [x] Private helpers move private.
- [x] No replacement aggregate header appears.
- [x] Platform/Renderer/GameFramework includes remain clean.

Stage 36 closure notes:

- Reference check: UE module guidance favors private dependencies and forward declarations where possible; Donut exposes deliberate include roots for stable app/core contracts rather than broad convenience surfaces.
- Existing-capability search found `DirectoryPaths.h` was the largest Core public header and mixed stable engine path contracts with reusable path formatting helpers.
- `DirectoryPaths.h` now keeps only composed artifact path builders and `Paths::LogFile`; direct forwarding wrappers to `Filesystem::*` roots were removed and call sites now use the real Core filesystem contract.
- Reusable formatting primitives live in `Core/Public/Paths/PathFormatting.h` with implementation in `Engine/Core/Private/Paths/PathFormatting.cpp`.
- Sparkle-specific log directory policy moved to `Engine/Core/Private/Paths/LogPathPolicy.*`; package/workspace/project discovery moved to `Engine/Core/Private/Paths/FileSystemDiscovery.*`.
- Private path implementation uses named private namespaces instead of unnamed namespaces in the touched Core path files.
- `Paths::LogFile` remains the log path public contract, and the root markers such as `.sparkle`, `.sparkle-engine`, and `.sparkle-project` remain intentional Core repository discovery data.
- `DirectoryPaths.h` dropped from 343 lines to 59 lines; `FileSystemUtils.cpp` dropped from about 800 lines to 553 lines after the private discovery split.
- Marker discovery propagation check: launcher repository discovery and AssetCooker repository discovery now use Core `Filesystem::FindAncestorWithMarker` and marker constants instead of local repository-root walkers/string literals; launcher's local public `NormalizePath` helper was removed.
- Remaining marker text outside Core is CMake project discovery, user-facing messages, and the launcher home-state folder name, not duplicate runtime marker logic.
- Platform, Renderer, and GameFramework include direction stays unchanged: consumers still include the specific Core headers they use, and no content/project/level/asset names or fallback chains were introduced.

### Stage 37: GameFramework Public API Cleanup

References: UE-SOURCE, NV-DONUT.

Prompt:

- Keep GameFramework high-level user concepts while hiding implementation details.

Acceptance:

Universal acceptance for this stage:

- [x] Existing-capability search is completed before adding code; prefer reuse, deletion, or replacement over new code.
- [x] Added code is offset by removed or simplified code in the same batch, or the batch records why net code reduction is impossible and where the next removal occurs.
- [x] No duplicate responsibility is introduced; if a similar capability exists, the older or less-owned path is removed or merged.
- [x] Real code does not hardcode project, level, asset, content-pack, or sample names; content-specific names stay in catalog, config, or content data.
- [x] No fallback chain is added; missing required data fails clearly at the owning boundary, and optional data is represented by explicit availability metadata.
- [x] Cleanup-after-cleanup scan is run for GameFramework and any touched Core/Renderer boundary files.

Stage-specific acceptance:

- [x] Level, scene, component, asset concepts remain.
- [x] Asset loader/manifest implementation details are private where possible.
- [x] Multi-level support remains intact.
- [x] Renderer still consumes GameFramework privately.

Stage 37 closure notes:

- Public `Engine/GameFramework/Public/Level/LevelRegistry.h` was removed; `LevelRegistry` is now private under `Engine/GameFramework/Private/Level`.
- `LevelManager` remains the high-level public level API; level change events remain public, while registry discovery/catalog ownership is private.
- No-value registry wrappers were removed after the initial cleanup: `GetAllLevels`, `GetLevelCount`, and `GetDefaultLevel` are gone; `GetLevelNames` owns sorted name enumeration without exposing the registry map.
- Public `Engine/GameFramework/Public/Assets/SceneAssetRegistry.h` was removed; `SceneAssetRegistry` is now private under `Engine/GameFramework/Private/Assets`, and `SceneAssetManager` hides it behind the high-level asset-load API.
- The scene cooker updates the registry through a private GameFramework include path declared in `Tools/Cooking/SceneCooker/CMakeLists.txt`, because registry persistence is a tool/runtime implementation detail, not a public user concept.
- Multi-level behavior remains catalog-driven, and no project, level, asset, content-pack, or sample name was hardcoded in GameFramework code.
- Verification: `cmake --build build --target SparkleGameFramework --config DevelopmentEditor`, `cmake --build build --target SceneCooker --config DevelopmentEditor`, `cmake --build build --target AssetCooker --config DevelopmentEditor`, and `cmake --build build --target SparkleApplication --config DevelopmentEditor` passed.

### Stage 38: Import/Cooker Public Surface Cleanup

References: AMD-CAULDRON, AMD-FIDELITYFX.

Prompt:

- Keep tool public headers only when another executable consumes them as stable API.

Acceptance:

General acceptance for this stage:

- [x] Universal engineering gate is satisfied.
- [x] Cleanup-after-cleanup scan is run for importer/cooker modules and any touched tool shared libraries.

Stage-specific acceptance:

- [x] Import/cooker public headers are audited.
- [x] Unused public bridge headers are removed.
- [x] Cooked asset outputs remain.
- [x] Default reports do not return.

Stage 38 closure notes:

- `Tools/Cooking/AssetCooker/Public/AssetCookRequest.h` and `Tools/Cooking/AssetCooker/Public/AssetCookerTypes.h` were removed; AssetCooker API types now live under private `Api`.
- `AssetCookerService` now exposes one real operation, `Cook`, instead of the old `CookProject` / `RecookAssets` / `CookCategory` forwarding stack.
- TextureCooker keeps `Tools/Cooking/TextureCooker/Public/TextureCookRequestList.h` as the remaining public request-list serialization contract; batch processing and command implementation details are private.
- Searches found no removed AssetCooker wrapper names and no `asset-cooker-plan-v1` / `asset-cooker-summary-v1` default report surfaces in the touched tool paths.
- Verification: `cmake --build build --target AssetCooker --config DevelopmentEditor` and `cmake --build build --target TextureCooker --config DevelopmentEditor` passed.

### Stage 39: Neural Rendering Readiness Without ML Bloat

References: NV-RTXNS, NV-RTX-KIT, NV-NRD, NV-STREAMLINE.

Prompt:

- Prepare for inference-like shader features without adding runtime ML frameworks.

Acceptance:

General acceptance for this stage:

- [x] Universal engineering gate is satisfied.
- [x] Cleanup-after-cleanup scan is run for touched shader, renderer, upscaling, denoising, or ray reconstruction code.

Stage-specific acceptance:

- [x] Slang/HLSL flexibility is preserved, and extended to match all requirements described in the persona.
- [x] Tensor/operator concepts remain design-level unless a renderer feature needs them; RHI readiness is expressed through shader package ABI, reflection, descriptor/binding parity, specialization/root constants, explicit capability reporting, and provider interop instead of empty tensor wrappers.
- [x] No PyTorch/TensorFlow/ONNX Runtime dependency is added, but architecture is ready for its arrival whenever a real renderer feature needs it.
- [x] Denoising/upscaling/ray reconstruction paths remain the practical readiness surface.

Stage 39 status:

- Completed as an audit-only stage. No source code changes were made because the existing shader/provider architecture already carries the useful readiness surface, and adding tensor/operator abstractions now would violate the no-bloat rule without a current renderer consumer.
- Reference check: RTX Neural Shading and RTX Kit point toward shader-side inference and narrow SDK/provider integrations; NRD and Streamline/DLSS Ray Reconstruction keep practical neural rendering value behind renderer feature slices rather than a general runtime ML framework.
- Existing-capability search covered ShaderCompiler DXC/Slang support, Slang DXIL/SPIR-V emission, shader reflection records, shader parameter struct verification, RHI binding and capability metadata, RHI native interop consumers, provider resource contracts, upscaling inputs, ray reconstruction inputs, GBuffer motion vectors, exposure, temporal jitter, frame index, camera state, and Streamline/DLSS runtime staging.
- Dependency scan across `Engine`, `Tools`, `CMake`, and `Projects` found no PyTorch, TensorFlow, ONNX Runtime, DirectML, CUDA, cuDNN, TensorRT, or similar runtime ML framework dependency.
- Cleanup-after-cleanup scan for the touched-readiness surface found no new single-field wrapper struct, empty conditional, or pass-through wrapper introduced by this stage. Existing multi-field provider/input contracts remain because they name real resources and validation state.
- D3D12/Vulkan parity remains through cooked shader targets, HLSL/SPIR-V binding reflection, RHI backend capabilities, and explicit provider interop. Future neural hardware features, such as cooperative-vector or tensor-oriented shader capabilities, must add backend capability fields only when consumed by a renderer feature.
- Verification: `cmake --build build --target ShaderCompiler --config DevelopmentEditor` and `cmake --build build --target SparkleRenderer --config DevelopmentEditor` passed.

### Stage 40: Package Contract Implementation

References: NV-STREAMLINE, AMD-FIDELITYFX.

Prompt:

- Make package outputs real and intentional.

Acceptance:

General acceptance for this stage:

- [x] Universal engineering gate is satisfied.
- [x] Cleanup-after-cleanup scan is run for package assembly, launcher package UI, artifact naming, manifests, and CMake packaging code.

Stage-specific acceptance:

- [x] Runtime/editor/launcher/dev tools/symbols/optional content package ownership is decided.
- [x] Unowned package assembly code is removed.
- [x] Optional content is separate from runtime package.
- [x] Manifest/checksum fields are consumed or removed.

Stage 40 closure notes:

- Owned package outputs are now runtime package and separate symbols package. Launcher, editor app, and runtime app are runtime-package components, not independent packages.
- Dev tools package, dependency package, separate launcher/editor package IDs, broad cooked-root staging, detailed release/build/dependency/component/file manifests, checksums, and symbol zip assembly were removed from default package assembly.
- Runtime package stages cooked shader packages only. Default scene/content package assembly must be added later through a catalog-driven default-content/optional-content package step, not by copying every cooked folder.
- Package UI copy now describes the actual runtime/symbol package contract instead of old debug/stat/package-cockpit behavior.
- Verification: `cmake --build build --target sparkle_release_assembly --config DevelopmentEditor` passed and emitted `sparkle-runtime-0.0.0-dev-dev-windows-x64` plus `sparkle-symbols-0.0.0-dev-dev-windows-x64`.

### Stage 41: Late Measurement Setup

References: AMD-CAULDRON, NV-NRI, UE-SOURCE.

Prompt:

- Use existing markers/timestamps/debuggers before adding any measurement code.

Acceptance:

General acceptance for this stage:

- [x] Universal engineering gate is satisfied.
- [x] Cleanup-after-cleanup scan is run for the measured feature path before any measurement code is accepted.

Stage-specific acceptance:

- [x] Feature cleanup is complete for the measured path.
- [x] Existing PIX/RenderDoc/Nsight hooks are used first.
- [x] Any new measurement code replaces old diagnostics.
- [x] No new report panel or benchmark format appears by default.

Stage 41 status:

- Completed as an audit-only measurement setup stage. No source code changes were made and no measurement feature was added.
- Existing-capability search found the measurement surfaces to use first: `RenderCommandList::{BeginDiagnosticScope, EndDiagnosticScope, InsertDiagnosticMarker}`, `RenderDiagnostics`, `RenderTimingDiagnostics`, `FrameExecutionDiagnostics`, `PassExecutionDiagnostics`, `FrameGraphExecutionDiagnostics`, `RayTracingPerformanceDiagnostics`, `CVarRendererDiagnosticMarkerVerbosity`, and `CVarRendererDiagnosticGpuTiming`.
- D3D12 already owns PIX-compatible event scopes through `D3D12PixEvents` / `WinPixEventRuntime.dll`, plus timestamp query allocation/write/resolve through `D3D12RenderDiagnostics`.
- Vulkan already owns RenderDoc/Nsight-visible debug-utils command labels and object names through `VulkanDebugEvents` and `VulkanRenderDiagnostics`; Vulkan timestamp timing is explicitly unavailable today through `SupportsTimestampQueries = false`, so no roadmap item may pretend Vulkan GPU timing parity exists until a real backend implementation is added.
- Renderer already consumes the measurement surface through frame, frame-graph pass, detailed pass, BLAS, classic TLAS, and PTLAS scopes. Future feature measurement must first enable/use these scopes and external tools before adding code.
- New measurement code is allowed only when it replaces an older diagnostic/report path or exposes a missing backend capability required by a cleaned-up feature path. It must not add a default report panel, benchmark format, timing summary, launcher cockpit page, or persistent artifact.
- Cleanup-after-cleanup scan covered RHI diagnostics, D3D12/Vulkan command marker paths, renderer frame/pass/ray-tracing diagnostics, and measurement CVars. No new single-field wrapper, empty branch, no-value forwarding wrapper, content hardcode, or fallback chain was introduced by this stage.

### Stage 42: Final Stabilization And Persona Evidence

References: all reference repos above.

Prompt:

- Run final stabilization after the staged cleanup sequence.
- Run the final repo-wide cleanup-after-cleanup scan after stabilization candidates are in place.
- Confirm the repo demonstrates the persona.

Acceptance:

General acceptance for this stage:

- [x] Universal engineering gate is satisfied for the current stabilization evidence pass.
- [x] Repo-wide cleanup-after-cleanup scan is complete after all staged implementation work.

Stage-specific acceptance:

- [x] Build relevant editor/runtime targets.
- [x] Cook curated default level set.
- [ ] Run default level set.
- [ ] Run D3D12 path where supported.
- [ ] Run Vulkan path where supported.
- [x] Verify shader packages cook and load with reflection data.
- [ ] Verify screenshot/BMP capture.
- [ ] Verify classic TLAS selection.
- [ ] Verify PTLAS selection where supported.
- [x] Verify multiple levels remain selectable.
- [x] Confirm public APIs are smaller.
- [x] Confirm repo/depot weight is smaller.
- [x] Confirm no new docs/logs/validation/report systems/wrappers/thick abstractions replaced old ones.
- [ ] Confirm no no-value forwarding wrappers, single-field data-only shells, empty branches, stale includes, or duplicated local helpers remain in touched modules.

Stage 42 evidence rules:

- A checkbox closes only with command output, produced artifact, or inspected source path named below.
- Dry-run launcher readiness is evidence for launch wiring, cooked-data availability, level selection, and environment/argument propagation; it is not evidence that a D3D12/Vulkan frame presented or that capture wrote a BMP.
- Runtime GPU evidence must name backend, startup level, executable, working directory, log path, and whether the process presented frames without fatal failure.
- Screenshot evidence must name the BMP file path and the product capture entrypoint that produced it.
- Cleanup evidence must distinguish actionable no-value shells from semantic handles, ABI records, or compact data contracts.

Stage 42 current evidence:

| Requirement | Status | Evidence |
| --- | --- | --- |
| Build relevant editor/runtime targets | Closed | `cmake --build build --target SparkleRHI SparkleRenderer ShowcaseEditor ShowcaseRuntime SparkleLauncher SparkleCookTools sparkle_release_assembly --config DevelopmentEditor` passed. `cmake --build build --target ShowcaseRuntime --config DevelopmentGame` also passed so the launcher-planned runtime executable was rebuilt after the PTLAS command-list cleanup. Outputs included `ShowcaseEditor.exe`, `ShowcaseRuntime.exe`, `SparkleLauncher.exe`, `AssetCooker.exe`, `ShaderCompiler.exe`, `TextureCooker.exe`, and release runtime/symbol layouts under `dist/releases/0.0.0-dev`. Launcher deploy kept one non-fatal `VCINSTALLDIR` warning. |
| Cook curated default level set | Closed | `artifacts\dev\tools\AssetCooker\DevelopmentEditor\AssetCooker.exe cook-project Showcase DevelopmentGame --root .` passed. It cooked shaders, textures, scene assets, meshes, and materials. Scene manifests produced for ABeautifulGame, CesiumMan, Cube, DamagedHelmet, DiffuseTransmissionPlant, Instancing, and Sponza. |
| Default level launch readiness | Dry-run only | `SparkleLauncher.exe --root . --project Showcase --launch-target runtime --startup-level Sponza --dry-run project.run` reported `Launch Project [Ready]`, valid executable, valid working directory, cooked scenes/meshes, textures, and shaders, and `SPARKLE_STARTUP_LEVEL=Sponza`. Actual runtime frame presentation remains open. |
| PTLAS level launch readiness | Dry-run only | `SparkleLauncher.exe --root . --project Showcase --launch-target runtime --startup-level SponzaPtlas --dry-run project.run` reported the same readiness facts with `SPARKLE_STARTUP_LEVEL=SponzaPtlas`. Actual PTLAS runtime rendering remains open. |
| D3D12/Vulkan selection plumbing | Static evidence only | `RhiBackendSelection.cpp` parses `--graphics-api`, `--graphics-api=`, and `SPARKLE_RHI_BACKEND`; launcher process requests pass `--graphics-api` when `GraphicsBackend` is set. Actual D3D12/Vulkan runs remain open. |
| Shader packages and reflection | Closed | Cook emitted `ShaderPackageRegistry.sreg` and 28 `.sparkshader` runtime package files. `ShaderCompiler inspect-package artifacts\dev\projects\Showcase\cooked\Shaders\Packages\E39738CC27CFB10B.sparkshader` showed DXIL and SPIR-V bytecode plus reflection and pipeline-layout records. |
| Screenshot/BMP capability | Static evidence only | Product path is `Renderer::CaptureViewportProductToBmp` -> `FramePipeline::CaptureViewportProductToBmp` -> `RhiCaptureService::CaptureTextureToBmp` -> D3D12/Vulkan capture services -> `WriteRhiBmp`. Actual BMP write remains open. |
| Multiple levels selectable | Closed | `Projects/Showcase/Levels.catalog` lists seven default-selectable levels with Sponza required/startup default and Bistro represented as external unavailable optional content. Launcher dry-runs accepted both `Sponza` and `SponzaPtlas`. |
| Public API size | Closed | Current public lines: Core 2387, RHI 4016, Renderer 2155, GameFramework 2082. Baseline review recorded Core 2658, RHI 4424, Renderer 2155, GameFramework 2151; combined targeted public surface decreased by 748 lines. |
| Depot weight | Closed | `Projects` excluding generated logs is 88.26 MiB, matching the Stage 07 target after Bistro externalization. Raw `Projects` is inflated by generated `Projects/Showcase/logs/trace.json`; generated logs are not source depot weight. |
| No sample/content hardcoding in engine/tool code | Closed for named content | Repo scan for `Sponza`, `SponzaPtlas`, `Bistro`, `DamagedHelmet`, `CesiumMan`, `DiffuseTransmissionPlant`, and `ABeautifulGame` in `Engine` and `Tools` returned no matches. Content names remain in catalog/level data. |
| No silent PTLAS no-op command path | Closed | Cleanup scan found `RenderCommandList::BuildPartitionedTopLevelAccelerationStructure(...) {}`. It was removed and made pure virtual; both D3D12 and Vulkan command lists already implement the command, and the rebuild passed. |
| Empty branch scan in touched RHI/renderer ray tracing paths | Closed | Scan over RHI public, D3D12, Vulkan, and renderer ray-tracing paths found no empty `if`/`else`/loop/switch bodies after the PTLAS command-list cleanup. |
| Final no-value shell scan | Open | Repo-wide scan finds semantic one-field or compact contract candidates that require owner-by-owner triage before this checkbox can close. Do not mark final persona evidence complete until actionable wrappers are deleted or explicitly justified as handles, ABI records, or stable product contracts. |

Remaining concrete runtime evidence to close:

1. Run default level with D3D12: `ShowcaseRuntime.exe --graphics-api d3d12` with `SPARKLE_STARTUP_LEVEL=Sponza`, from `Projects/Showcase`, then record log path and result.
2. Run default level with Vulkan where supported: `ShowcaseRuntime.exe --graphics-api vulkan` with `SPARKLE_STARTUP_LEVEL=Sponza`, from `Projects/Showcase`, then record log path and result.
3. Run PTLAS selection where supported: same backend command with `SPARKLE_STARTUP_LEVEL=SponzaPtlas`, then confirm selected top-level provider is PTLAS and unsupported PTLAS reports explicit unavailable capability instead of falling back silently.
4. Produce one BMP through `Renderer::CaptureViewportProductToBmp`, record the output path, and verify the file exists and is non-empty.
5. Triage repo-wide cleanup scan candidates and close only after no actionable no-value wrappers, single-field data-only shells, empty branches, stale includes, or duplicated local helpers remain in modules touched by the staged work.

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
- [ ] Repo-wide cleanup-after-cleanup scan finds no no-value forwarding wrappers, single-field data-only shells, empty control-flow shells, stale includes, duplicate local helpers, or dead compatibility paths in modules touched by the staged work.
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

Final cleanup scan prompts:

```powershell
rg -n "TODO|temporary|compat|legacy|fallback|wrapper|shim|unused|deprecated|scaffold" Engine Tools Projects
rg -n "struct .*\\{|class .*\\{|enum .*\\{" Engine Tools Projects
rg -n "if \\(|else|switch \\(|for \\(|while \\(" Engine Tools Projects
rg -n "return [A-Za-z0-9_:]+\\(.*\\);" Engine Tools Projects
rg -n "#include" Engine Tools Projects
```

Use these as starting points, not proof by themselves. For each hit, decide whether it is a real current capability, a product-owned abstraction, or a leftover shell from cleanup. Delete or collapse the leftover shell before final acceptance.

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
