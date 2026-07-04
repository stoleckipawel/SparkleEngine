# 02. MODIFY - Refactor Existing Systems

Status: staged implementation prompt
Date: 2026-07-04
Source set: `Docs/Architecture/00-Review`
Use this as: the primary "change existing code" plan

## Purpose

This document gathers every major system that should be modified rather than kept untouched, deleted outright, or expanded with new systems.

The modify path is for valuable systems whose ownership, API surface, default behavior, or feature shape is too broad.

Modify means:

- preserve the capability
- reduce code or public surface
- sharpen ownership
- keep the thinnest high-level layer that makes the engine easier to use and reason about
- remove diagnostics/logs/reports/scaffolding around the capability
- keep D3D12/Vulkan, shader ABI, cook, runtime, and editor workflows intact

Modify does not mean:

- add wrappers
- add thick abstraction layers
- add validation systems
- add logging
- add new documents
- add another abstraction because the current boundary is uncomfortable
- remove valuable capabilities

## Common Refactor Gate

Every implementation batch must answer:

1. Persona pillar: which persona capability is this developing?
2. Engine capability: what capability is preserved or improved?
3. Deletion delta: what code, public API, file, artifact, or depot weight is removed?
4. Backend impact: D3D12, Vulkan, or both?
5. Shader impact: shader source, offline cooked shader packages, reflection, cook output, or none?
6. Runtime impact: RHI, frame graph, renderer passes, GameFramework, launcher, cookers, or none?
7. Public/private impact: which public surface shrinks or remains justified?
8. Tooling impact: are debugger markers and screenshot capture preserved?
9. Content impact: is multi-level support preserved?
10. Separation impact: does the change preserve Core/RHI/Renderer/GameFramework/Tools ownership?
11. No-pollution check: no new docs, logs, validation systems, wrappers, thick abstraction layers, diagnostic panels, or future scaffolding.

If the batch cannot answer these, split it or move it to `03_ADD_MinimalMissingCapabilities.md` or `04_REMOVE_DeletionsAndCleanup.md`.

Modification bias:

- Remove bloat first.
- Prefer fewer files, fewer public headers, fewer settings, fewer branches, and fewer default artifacts.
- Keep a thin user-facing concept only when it makes the engine easier to operate without hiding the rendering path.
- A richer engine with less code is the signal of good engineering.

## Modification Order

Recommended order:

1. Content catalog and optional heavy content organization.
2. Screenshot/BMP capture ownership hardening.
3. Smoke/report/debug artifact cleanup around existing workflows.
4. Launcher slimming.
5. Public RHI/Renderer observation API narrowing.
6. Classic TLAS/PTLAS refactor.
7. Reference path tracing role cleanup.
8. Shader debug and duplication cleanup.
9. Core/GameFramework public surface reduction.
10. Feature hardening before measurement.
11. Late profiling and measurement only after feature cleanup.
12. Package contract cleanup.

## 1. Content And Level Organization

Persona pillar: product engineering discipline.

Current problem:

- `Projects` is about 2788.88 MB, mostly Showcase content.
- Heavy content dominates depot size more than source code.
- The engine must not become a one-level demo. Multi-level support is a preserved capability.

Target shape:

- multi-project and multi-level support remains first-class
- default level set is curated for build/cook/run/review
- heavy levels/assets are optional packs
- launcher/cooker can select available levels
- missing optional packs fail gracefully
- default workflows do not require multi-GB media

Allowed modifications:

- add or modify level/content catalog metadata only if it enables deletion or externalization of heavy content
- update launcher/cooker selection to use catalog data
- move heavy assets out of the core repo after catalog representation exists
- keep small default assets needed by curated levels

Do not:

- reduce Sparkle to one minimal level
- dump more uncataloged levels into the repo
- add content systems that do not remove depot weight

Target paths:

- `Projects/Showcase`
- `Projects/Showcase/Assets/Meshes/Bistro`
- large DDS/TGA/HDR assets
- launcher/cooker project selection code

Acceptance:

- [ ] Multiple Showcase levels remain selectable.
- [ ] Default level set cooks and launches without optional packs.
- [ ] Heavy optional packs are discoverable through one path.
- [ ] Core repo size drops materially.
- [ ] Source increase is limited to small catalog/resolver code.

## 2. Screenshot/BMP Capture Ownership

Persona pillar: debugging and tool fluency.

Current problem:

- Capture is important, but old smoke/ad hoc call sites can make it look like a diagnostic toy.
- Capture should not be deleted.

Target shape:

- screenshot/BMP capture is a preserved editor/tool capability
- capture has narrow ownership and low runtime cost
- smoke-only capture ownership is removed
- public API is justified by real editor/tool use

Allowed modifications:

- move capture call sites out of smoke paths
- make capture entrypoint explicit and product-owned
- keep BMP writer only if used by the hardened capture path
- keep D3D12/Vulkan readback behavior working

Do not:

- delete capture capability
- add a new capture framework
- add screenshot report artifacts
- expose broad public capture diagnostics

Target paths:

- `Engine/RHI/Public/Capture/RhiCaptureService.h`
- `Engine/RHI/Private/Capture/RhiBmpWriter.*`
- smoke/ad hoc capture call sites
- editor/launcher capture command paths if present

Acceptance:

- [ ] `CaptureTextureToBmp` remains only in the hardened capability.
- [ ] Smoke/ad hoc ownership is gone.
- [ ] Capture still works from the intended editor/tool path.
- [ ] Public API is smaller or clearly justified.

## 3. Launcher Workflow Shell

Persona pillar: product engineering discipline.

Current problem:

- `Tools/Launcher/SparkleLauncher` is large enough to be judged as an application.
- It includes build/cook/launch/package/clean/dependency/status/quality plus logs, operation catalogs, dry-run plans, and debug toggles.

Target shape:

- launcher is a small workflow shell
- first-class workflows are build, cook, run, clean, package if owned, dependency sync if actively used
- launcher does not preserve validation/report/debug scaffolding

Allowed modifications:

- remove operation/status UI that does not drive a current workflow
- remove shader debug artifact/stat toggles from normal GUI
- keep command paths that build, cook, run, clean, or package real outputs
- keep package UI only if package contract is real

Do not:

- add new panels
- add new logs
- add new report formats
- move every local workflow into the launcher

Target paths:

- `Tools/Launcher/SparkleLauncher/Private/Shell/LauncherShell.cpp`
- launcher option pages
- launcher operation/status pages
- shader debug toggles
- package workflow files

Acceptance:

- [ ] Launcher source decreases.
- [ ] One command path still builds/cooks/runs.
- [ ] Debug/diagnostic toggles are gone unless product-owned.
- [ ] No workflow is replaced by a new wrapper.

## 4. Public Renderer And RHI Observation APIs

Persona pillar: API taste and explicit graphics API ownership.

Current problem:

- Renderer and RHI expose memory, mesh, texture, descriptor, and diagnostic surfaces that look like permanent product API.
- Some observation is useful, but broad public snapshots raise maintenance cost.

Target shape:

- public API exposes behavior and capability
- editor-only observation moves behind private/editor ownership
- compact runtime pressure/status survives only if it drives runtime policy
- screenshot/BMP capture remains preserved and narrow
- debugger/profiler support remains preserved

Allowed modifications:

- shrink public diagnostic headers
- move editor-only data behind editor-private adapters
- collapse memory/descriptor snapshots into compact status if consumed
- keep RHI debug layer, object names, GPU events, timestamp queries, failure diagnostics

Do not:

- delete product capture
- add a diagnostics facade
- add public JSON dump/detail allocation APIs
- remove backend debug support

Target paths:

- `Engine/Renderer/Public/Renderer.h`
- `Engine/Renderer/Public/Diagnostics/*`
- `Engine/Renderer/Public/Meshes/*Diagnostics*`
- `Engine/Renderer/Public/Resources/Textures/*Diagnostics*`
- `Engine/RHI/Public/Diagnostics/*`
- `Engine/RHI/Public/Descriptors/RhiDescriptorService.h`
- editor panels consuming diagnostics

Acceptance:

- [ ] Public renderer/RHI lines decrease.
- [ ] Editor still shows only product-owned panels.
- [ ] Runtime policy still has the compact facts it needs.
- [ ] GPU markers/timestamps/debug-layer support remains.

## 5. Classic TLAS And PTLAS

Persona pillar: ray tracing depth.

Current problem:

- Classic TLAS and PTLAS are both valuable.
- PTLAS currently risks carrying planner metrics, diagnostic structs, future GPU-pack scaffolding, and CPU validation readbacks beyond the minimal product need.

Target shape:

- classic TLAS and PTLAS are equal product RT features
- users can select either where supported
- D3D12 and Vulkan capability is preserved
- PTLAS is minimized toward reference flow:
  - capability check
  - compact descriptor input
  - backend build/update
  - resource lifetime
  - trace usage

Allowed modifications:

- delete future GPU-pack placeholders
- remove PTLAS metrics/diagnostics not needed to build/update/trace
- delete CPU validation readback paths that are not product behavior
- collapse strategy code toward minimal selection and backend calls

Do not:

- remove PTLAS capability
- remove classic TLAS capability
- add a new acceleration structure abstraction
- make PTLAS D3D12-only if Vulkan support exists

Target paths:

- `Engine/Renderer/Private/RayTracing/Acceleration/RayTracingPartitionedTlasStrategy.*`
- `Engine/Renderer/Private/RayTracing/Acceleration/RayTracingPtlas*`
- `Engine/Renderer/Private/RayTracing/Diagnostics/RayTracingPtlas*`
- `Engine/RHI/Public/RayTracing/RhiPartitionedTlasDesc.h`
- D3D12/Vulkan PTLAS services

Acceptance:

- [ ] Classic TLAS renders.
- [ ] PTLAS renders where supported.
- [ ] Selection can choose classic TLAS or PTLAS.
- [ ] Future GPU-pack placeholders are gone.
- [ ] PTLAS code is smaller and more direct.

## 6. Reference Path Tracing

Persona pillar: rendering correctness.

Current problem:

- Reference path tracing can look like a second realtime renderer, debug reference, progressive renderer, and provider signal source at once.

Target shape:

- choose one clear role first: debug reference
- share BRDF/material/light policy with realtime path
- remove provider handoff expectations if unsupported
- remove guide outputs not consumed by product features

Allowed modifications:

- simplify reference settings
- delete dead provider hooks
- reduce reference-only buffers
- preserve enough path tracing to compare correctness

Do not:

- add RTXPT-style product path unless it replaces/deletes current complexity
- add new guide buffers without consumers
- add debug panels

Acceptance:

- [ ] Reference mode has one sentence product role.
- [ ] Dead provider handoff is gone.
- [ ] Reference buffers/settings are fewer or explicitly consumed.
- [ ] Material/light policy is shared with realtime path.

## 7. Shader Debug And Duplication

Persona pillar: shader/kernel craft.

Current problem:

- Shader pipeline is strong, but default debug bundles, stats CSV, and validation-oriented demos can make it feel like a lab.

Target shape:

- default shader cook writes runtime packages only
- debug artifacts are opt-in or removed
- demo shaders that do not serve renderer features are removed from product runtime
- duplication is reduced only when the replacement deletes materially more code

Allowed modifications:

- remove debug artifact writers from default cook
- remove launcher controls for debug artifacts
- remove HelloWorld/ComputeClear-style validation registrations from product runtime if they are only validation-oriented
- preserve package ABI and reflection

Do not:

- add a generator unless net code decreases
- weaken layout verification
- remove package inspection if actively useful

Acceptance:

- [ ] Default cook writes runtime shader packages only.
- [ ] Debug bundles are opt-in or gone.
- [ ] Shader ABI still works.
- [ ] Source line delta is negative or clearly justified.

## 8. Core And GameFramework Public Surface

Persona pillar: engine boundary discipline.

Current problem:

- Core is public-heavy.
- GameFramework may expose asset loader and scene-manifest implementation details too early.

Target shape:

- public headers represent stable runtime contract
- private helpers stay private
- aggregate/convenience headers do not grow
- renderer consumes GameFramework privately

Allowed modifications:

- move implementation-only types private
- remove unused utility APIs
- narrow asset loader and scene manifest public exposure
- keep level/scene/component/asset types that projects actually use

Do not:

- add replacement aggregate headers
- break cooked scene loading
- move renderer details into GameFramework

Acceptance:

- [ ] Public Core/GameFramework lines decrease.
- [ ] Cooked scene still loads.
- [ ] No replacement aggregate header appears.
- [ ] Build includes remain cleaner.

## 9. Feature Hardening Before Measurement

Persona pillar: feature ownership.

Current problem:

- Profiling too early will measure unstable feature shape.

Target shape:

- RT, GI/path tracing, post-processing, denoising, upscaling, frame graph, shaders, and passes are cleaned before measurement work
- existing markers/timestamps/debugger support remain
- no new profiling framework

Allowed modifications:

- simplify pass ownership
- reduce frame graph and shader-pass duplication
- trim provider diagnostics
- preserve screenshot capture

Do not:

- add benchmark/report systems
- add logs
- add validation systems
- add profiling panels

Acceptance:

- [ ] Feature-facing renderer/RHI code is smaller or simpler.
- [ ] Affected features remain buildable.
- [ ] Screenshot capture still works.
- [ ] Existing debug/profiler hooks remain.

## 10. Package Contract

Persona pillar: product engineering discipline.

Current problem:

- Package assembly exists, but package ownership is not crisp.

Target shape:

- only real packages are assembled
- runtime/editor/launcher/dev tools/symbols/optional content are intentional
- content pack is separate from runtime package

Allowed modifications:

- remove release manifest fields not consumed
- remove release note generation not used
- keep checksums/manifests only if a workflow consumes them

Do not:

- keep "review-only" package output unless explicitly owned
- add packaging code for future products

Acceptance:

- [ ] One package command produces only real outputs.
- [ ] Package output bytes or package/build code decrease.
- [ ] Content pack is not forced into runtime package.

## Done State

The modify plan is complete when:

- repo size is smaller
- public renderer/RHI/Core/GameFramework API is smaller
- launcher is workflow-focused
- cookers emit assets, not default reports
- shader cook emits runtime packages, not debug bundles
- classic TLAS and PTLAS are both product-owned
- PTLAS is smaller and functional on supported D3D12/Vulkan paths
- screenshot capture is preserved and hardened
- feature paths are cleaner before profiling
